from __future__ import annotations

import importlib.util
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "linux-lab" / "orchestrator" / "core" / "example_catalog.py"
CATALOG_ROOT = ROOT / "linux-lab" / "catalog" / "examples"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
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
