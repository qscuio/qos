from __future__ import annotations

import importlib.util
import sys
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "linux-lab" / "orchestrator" / "core" / "example_catalog.py"
CATALOG_ROOT = ROOT / "linux-lab" / "catalog" / "examples"
EXAMPLES_MODULE_PATH = ROOT / "linux-lab" / "orchestrator" / "core" / "examples.py"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
    linux_lab_root = ROOT / "linux-lab"
    if str(linux_lab_root) not in sys.path:
        sys.path.insert(0, str(linux_lab_root))
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_curated_examples_have_catalog_entries() -> None:
    expected = [
        CATALOG_ROOT / "modules-debug.yaml",
        CATALOG_ROOT / "simple.yaml",
        CATALOG_ROOT / "ioctl.yaml",
        CATALOG_ROOT / "userspace-app.yaml",
        CATALOG_ROOT / "rust_learn.yaml",
        CATALOG_ROOT / "bpf-learn.yaml",
    ]
    missing = [str(path) for path in expected if not path.is_file()]
    assert missing == [], f"missing curated example catalog entries: {missing}"


def test_catalog_loader_exposes_required_fields_and_enabled_curated_subset() -> None:
    module = _load_module("linux_lab_example_catalog", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)

    curated_keys = {
        "bpf-learn",
        "ioctl",
        "modules-debug",
        "rust_learn",
        "simple",
        "userspace-app",
    }
    assert curated_keys.issubset(catalog)
    required = {"key", "kind", "category", "source", "origin", "build_mode", "enabled"}
    for item in catalog.values():
        assert required.issubset(item), f"catalog entry missing required fields: {item}"
    for key in curated_keys:
        assert catalog[key]["enabled"] is True


def test_catalog_loader_rejects_duplicate_keys(tmp_path: Path) -> None:
    module = _load_module("linux_lab_example_catalog_dupe", MODULE_PATH)
    root = tmp_path / "catalog"
    root.mkdir()
    first = root / "first.yaml"
    second = root / "second.yaml"
    body = "\n".join(
        [
            'key: "duplicate"',
            'kind: "module"',
            'category: "core"',
            'source: "linux-lab/examples/modules/simple"',
            'origin: "../qulk/modules/simple"',
            'build_mode: "kbuild-module"',
            "enabled: true",
            "",
        ]
    )
    first.write_text(body, encoding="utf-8")
    second.write_text(body, encoding="utf-8")

    try:
        module.load_example_catalog(root)
    except ValueError as exc:
        assert "duplicate" in str(exc)
    else:
        raise AssertionError("expected duplicate catalog key to fail")


def test_catalog_covers_all_qulk_module_roots() -> None:
    module = _load_module("linux_lab_example_catalog_full", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)
    covered_roots = {Path(item["origin"]).name for item in catalog.values()}
    expected_roots = {path.name for path in (ROOT / ".." / "qulk" / "modules").iterdir() if path.is_dir()}

    missing = sorted(expected_roots.difference(covered_roots))
    assert missing == [], f"missing qulk module roots from catalog: {missing}"
    for item in catalog.values():
        source_path = ROOT / item["source"]
        assert source_path.exists(), f"catalog source path does not exist: {source_path}"


def test_custom_make_entries_are_explicitly_wired_and_enabled() -> None:
    module = _load_module("linux_lab_example_catalog_custom_make", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)

    custom_entries = sorted(
        (item["key"], item.get("build_commands"))
        for item in catalog.values()
        if item["build_mode"] == "custom-make"
    )
    custom_keys = [key for key, _commands in custom_entries]

    assert custom_keys == [
        "LiME",
        "afxdp",
        "bds_lkm_ftrace",
        "kernel-hook-framework",
        "linux_kernel_hacking",
        "mm-uffd",
        "sBPF",
        "xdp_ipv6_filter",
    ]
    for key, commands in custom_entries:
        assert catalog[key]["enabled"] is True
        assert commands, f"{key} is missing explicit build_commands"
    assert catalog["rust"]["enabled"] is True
    assert catalog["rust"]["build_mode"] == "kernel-tree-rust"


def test_example_planner_picks_up_newly_enabled_catalog_entries(tmp_path: Path) -> None:
    labroot = tmp_path / "labroot"
    catalog_root = labroot / "catalog" / "examples"
    catalog_root.mkdir(parents=True)
    module_dir = labroot / "examples" / "modules" / "core" / "hello"
    module_dir.mkdir(parents=True)
    (catalog_root / "hello.yaml").write_text(
        "\n".join(
            [
                'key: "hello"',
                'kind: "module"',
                'category: "core"',
                'source: "linux-lab/examples/modules/core/hello"',
                'origin: "../qulk/modules/hello"',
                'build_mode: "kbuild-module"',
                "enabled: true",
                "",
            ]
        ),
        encoding="utf-8",
    )

    module = _load_module("linux_lab_examples_catalog_enabled", EXAMPLES_MODULE_PATH)
    plan = module.resolve_example_plan(
        example_groups=["modules-core"],
        linux_lab_root=labroot,
        build_root=tmp_path / "build",
    )

    assert [entry["key"] for entry in plan[0]["entries"]] == ["hello"]


def test_example_planner_respects_catalog_group_membership(tmp_path: Path) -> None:
    labroot = tmp_path / "labroot"
    catalog_root = labroot / "catalog" / "examples"
    catalog_root.mkdir(parents=True)
    for relative in (
        "examples/modules/core/simple",
        "examples/modules/core/hello",
    ):
        (labroot / relative).mkdir(parents=True)

    (catalog_root / "simple.yaml").write_text(
        "\n".join(
            [
                'key: "simple"',
                'kind: "module"',
                'category: "core"',
                'source: "linux-lab/examples/modules/core/simple"',
                'origin: "../qulk/modules/simple"',
                'build_mode: "kbuild-module"',
                "groups:",
                '  - "modules-core"',
                '  - "modules-all"',
                "enabled: true",
                "",
            ]
        ),
        encoding="utf-8",
    )
    (catalog_root / "hello.yaml").write_text(
        "\n".join(
            [
                'key: "hello"',
                'kind: "module"',
                'category: "core"',
                'source: "linux-lab/examples/modules/core/hello"',
                'origin: "../qulk/modules/hello"',
                'build_mode: "kbuild-module"',
                "groups:",
                '  - "modules-all"',
                "enabled: true",
                "",
            ]
        ),
        encoding="utf-8",
    )

    module = _load_module("linux_lab_examples_catalog_groups", EXAMPLES_MODULE_PATH)

    core_plan = module.resolve_example_plan(
        example_groups=["modules-core"],
        linux_lab_root=labroot,
        build_root=tmp_path / "build",
    )
    full_plan = module.resolve_example_plan(
        example_groups=["modules-all"],
        linux_lab_root=labroot,
        build_root=tmp_path / "build",
    )

    assert [entry["key"] for entry in core_plan[0]["entries"]] == ["simple"]
    assert [entry["key"] for entry in full_plan[0]["entries"]] == ["simple", "hello"]


def test_mm_experiments_catalog_entries_are_valid_and_enabled() -> None:
    module = _load_module("linux_lab_example_catalog_mm", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)

    mm_keys = {"mm-anon-fault", "mm-cow-fault", "mm-zero-page", "mm-uffd"}
    assert mm_keys.issubset(catalog), f"missing mm catalog entries: {mm_keys - catalog.keys()}"

    for key in mm_keys:
        entry = catalog[key]
        assert entry["enabled"] is True
        assert entry["kind"] == "userspace"
        assert entry["category"] == "memory"
        assert entry["build_mode"] in {"gcc-userspace", "custom-make"}
        assert "mm-experiments" in entry.get("groups", [])


def test_mm_experiments_group_is_registered_in_example_planner() -> None:
    module = _load_module("linux_lab_examples_mm_reg", EXAMPLES_MODULE_PATH)
    assert "mm-experiments" in module.EXAMPLE_GROUP_ORDER
    assert module.GROUP_KIND_MAP.get("mm-experiments") == "userspace"
    assert "mm-experiments" in module.GROUP_ENTRY_PRIORITY


def test_mm_probe_catalog_entry_is_valid() -> None:
    module = _load_module("linux_lab_example_catalog_mm_probe", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)
    assert "mm-probe" in catalog
    entry = catalog["mm-probe"]
    assert entry["enabled"] is True
    assert entry["kind"] == "module"
    assert entry["category"] == "memory"
    assert entry["origin"] == "qos-native"
    assert entry["build_mode"] == "kbuild-module"
    assert entry["source"] == "linux-lab/experiments/mm/mm_probe"
    assert "mm-probe" in entry.get("groups", [])


def test_mm_probe_group_is_registered_in_example_planner() -> None:
    module = _load_module("linux_lab_examples_mm_probe_reg", EXAMPLES_MODULE_PATH)
    assert "mm-probe" in module.EXAMPLE_GROUP_ORDER
    assert module.GROUP_KIND_MAP.get("mm-probe") == "module"
    assert "mm-probe" in module.GROUP_ENTRY_PRIORITY


def test_mm_va_to_pa_catalog_entry_is_valid() -> None:
    module = _load_module("linux_lab_example_catalog_mm_va_to_pa", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)
    assert "mm-va-to-pa" in catalog
    entry = catalog["mm-va-to-pa"]
    assert entry["enabled"] is True
    assert entry["kind"] == "userspace"
    assert entry["category"] == "memory"
    assert entry["origin"] == "qos-native"
    assert entry["build_mode"] == "gcc-userspace"
    assert entry["source"] == "linux-lab/experiments/mm/va_to_pa"
    assert "mm-experiments" in entry.get("groups", [])
