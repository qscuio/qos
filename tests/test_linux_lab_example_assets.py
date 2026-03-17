from __future__ import annotations

import importlib.util
import json
import shutil
import subprocess
import sys
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[1]
BIN_LINUX_LAB = ROOT / "linux-lab" / "bin" / "linux-lab"
REQUESTS_ROOT = ROOT / "build" / "linux-lab" / "requests"
MODULE_PATH = ROOT / "linux-lab" / "orchestrator" / "core" / "examples.py"
CATALOG_MODULE_PATH = ROOT / "linux-lab" / "orchestrator" / "core" / "example_catalog.py"
LINUX_LAB_ROOT = ROOT / "linux-lab"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
    if str(LINUX_LAB_ROOT) not in sys.path:
        sys.path.insert(0, str(LINUX_LAB_ROOT))
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _latest_request_root() -> Path:
    request_dirs = sorted([path for path in REQUESTS_ROOT.iterdir() if path.is_dir()])
    assert request_dirs, f"no request directories in {REQUESTS_ROOT}"
    return request_dirs[-1]


def setup_function() -> None:
    shutil.rmtree(ROOT / "build" / "linux-lab", ignore_errors=True)


def test_curated_examples_are_ported() -> None:
    expected = [
        ROOT / "linux-lab" / "examples" / "modules" / "debug" / "Makefile",
        ROOT / "linux-lab" / "examples" / "modules" / "simple" / "Makefile",
        ROOT / "linux-lab" / "examples" / "modules" / "ioctl" / "Makefile",
        ROOT / "linux-lab" / "examples" / "userspace" / "app" / "poll.c",
        ROOT / "linux-lab" / "examples" / "bpf" / "learn" / "xdp.c",
        ROOT / "linux-lab" / "examples" / "rust" / "rust_learn" / "hello_rust.rs",
        ROOT / "linux-lab" / "examples" / "rust" / "rust_learn" / "user" / "test_hello.rs",
    ]
    missing = [str(path) for path in expected if not path.is_file()]
    assert missing == [], f"missing curated examples: {missing}"


def test_curated_examples_are_enabled_in_catalog() -> None:
    module = _load_module("linux_lab_example_catalog_assets", CATALOG_MODULE_PATH)
    catalog = module.load_example_catalog(ROOT / "linux-lab" / "catalog" / "examples")

    assert catalog["modules-debug"]["enabled"] is True
    assert catalog["simple"]["enabled"] is True
    assert catalog["ioctl"]["enabled"] is True
    assert catalog["userspace-app"]["enabled"] is True
    assert catalog["rust_learn"]["enabled"] is True
    assert catalog["bpf-learn"]["enabled"] is True


def test_example_helper_resolves_build_commands() -> None:
    module = _load_module("linux_lab_examples", MODULE_PATH)
    plan = module.resolve_example_plan(
        example_groups=["modules-core", "userspace-core", "rust-core", "bpf-core"],
        linux_lab_root=ROOT / "linux-lab",
        build_root=Path("build/linux-lab/request/workspace/build"),
    )

    groups = [item["group"] for item in plan]
    assert groups == ["modules-core", "userspace-core", "rust-core", "bpf-core"]
    assert [entry["key"] for entry in plan[0]["entries"]] == ["modules-debug", "simple", "ioctl"]
    assert [entry["key"] for entry in plan[1]["entries"]] == ["userspace-app"]
    assert [entry["key"] for entry in plan[2]["entries"]] == ["rust_learn"]
    assert [entry["key"] for entry in plan[3]["entries"]] == ["bpf-learn"]
    assert plan[0]["commands"][0][0:2] == ["make", "-C"]
    assert plan[1]["commands"][0][0] == "gcc"
    assert plan[2]["commands"][0][0:2] == ["make", "-C"]
    assert plan[3]["commands"][0][0] == "clang"


def test_run_dry_run_emits_example_metadata() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "run",
            "--kernel",
            "6.18.4",
            "--arch",
            "x86_64",
            "--image",
            "noble",
            "--profile",
            "default-lab",
            "--dry-run",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    example_state = json.loads((request_root / "state" / "build-examples.json").read_text(encoding="utf-8"))
    assert example_state["status"] == "dry-run"
    example_plans = example_state["metadata"]["example_plans"]
    assert [item["group"] for item in example_plans] == [
        "modules-core",
        "userspace-core",
        "rust-core",
        "bpf-core",
        "mm-experiments",
        "mm-probe",
    ]
    rust_plan = next(item for item in example_plans if item["group"] == "rust-core")
    assert [entry["key"] for entry in rust_plan["entries"]] == ["rust_learn"]


def test_full_samples_profile_emits_broad_example_metadata() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "run",
            "--kernel",
            "6.18.4",
            "--arch",
            "x86_64",
            "--image",
            "noble",
            "--profile",
            "full-samples",
            "--dry-run",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    example_state = json.loads((request_root / "state" / "build-examples.json").read_text(encoding="utf-8"))
    example_plans = example_state["metadata"]["example_plans"]
    assert [item["group"] for item in example_plans] == [
        "modules-all",
        "userspace-all",
        "rust-all",
        "bpf-all",
    ]
    assert len(example_plans[0]["entries"]) > 3
    assert any(entry["key"] == "hello" for entry in example_plans[0]["entries"])
    assert any(entry["key"] == "rust" for entry in example_plans[2]["entries"])
    assert any(entry["key"] == "afxdp" for entry in example_plans[3]["entries"])


def test_minimal_profile_omits_optional_example_metadata() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "run",
            "--kernel",
            "6.18.4",
            "--arch",
            "arm64",
            "--image",
            "noble",
            "--profile",
            "minimal",
            "--dry-run",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    example_state = json.loads((request_root / "state" / "build-examples.json").read_text(encoding="utf-8"))
    assert example_state["status"] == "dry-run"
    assert example_state["metadata"]["example_groups"] == []
    assert example_state["metadata"]["example_plans"] == []


def test_mm_experiments_entries_appear_in_default_lab_dry_run() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "run",
            "--kernel", "6.18.4",
            "--arch", "x86_64",
            "--image", "noble",
            "--profile", "default-lab",
            "--dry-run",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    example_state = json.loads(
        (request_root / "state" / "build-examples.json").read_text(encoding="utf-8")
    )
    mm_plan = next(
        (item for item in example_state["metadata"]["example_plans"]
         if item["group"] == "mm-experiments"),
        None,
    )
    assert mm_plan is not None, "mm-experiments group missing from default-lab dry-run"
    mm_keys = {entry["key"] for entry in mm_plan["entries"]}
    assert mm_keys == {"mm-anon-fault", "mm-cow-fault", "mm-zero-page", "mm-uffd"}


def test_mm_experiment_source_files_exist() -> None:
    expected = [
        ROOT / "linux-lab" / "experiments" / "mm" / "anon_fault" / "anon_fault.c",
        ROOT / "linux-lab" / "experiments" / "mm" / "cow_fault" / "cow_fault.c",
        ROOT / "linux-lab" / "experiments" / "mm" / "zero_page" / "zero_page.c",
        ROOT / "linux-lab" / "experiments" / "mm" / "uffd" / "uffd.c",
    ]
    missing = [str(p) for p in expected if not p.is_file()]
    assert missing == [], f"missing mm experiment source files: {missing}"
