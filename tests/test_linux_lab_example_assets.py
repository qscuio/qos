from __future__ import annotations

import importlib.util
import json
import shutil
import subprocess
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[1]
BIN_LINUX_LAB = ROOT / "linux-lab" / "bin" / "linux-lab"
REQUESTS_ROOT = ROOT / "build" / "linux-lab" / "requests"
MODULE_PATH = ROOT / "linux-lab" / "orchestrator" / "core" / "examples.py"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
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


def test_example_helper_resolves_build_commands() -> None:
    module = _load_module("linux_lab_examples", MODULE_PATH)
    plan = module.resolve_example_plan(
        example_groups=["modules-core", "userspace-core", "rust-core", "bpf-core"],
        linux_lab_root=ROOT / "linux-lab",
        build_root=Path("build/linux-lab/request/workspace/build"),
    )

    groups = [item["group"] for item in plan]
    assert groups == ["modules-core", "userspace-core", "rust-core", "bpf-core"]
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
    assert [item["group"] for item in example_state["metadata"]["example_plans"]] == [
        "modules-core",
        "userspace-core",
        "rust-core",
        "bpf-core",
    ]
