from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BIN_LINUX_LAB = ROOT / "linux-lab" / "bin" / "linux-lab"
REQUESTS_ROOT = ROOT / "build" / "linux-lab" / "requests"


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _latest_request_root() -> Path:
    request_dirs = sorted([path for path in REQUESTS_ROOT.iterdir() if path.is_dir()])
    assert request_dirs, f"no request directories in {REQUESTS_ROOT}"
    return request_dirs[-1]


def setup_function() -> None:
    shutil.rmtree(ROOT / "build" / "linux-lab", ignore_errors=True)


def test_plan_writes_request_and_placeholder_stage_state() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "plan",
            "--kernel",
            "6.18.4",
            "--arch",
            "x86_64",
            "--image",
            "noble",
            "--profile",
            "default-lab",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    request_json = request_root / "request.json"
    validate_json = request_root / "state" / "validate.json"
    boot_json = request_root / "state" / "boot.json"

    assert request_json.is_file(), f"missing {request_json}"
    assert validate_json.is_file(), f"missing {validate_json}"
    assert boot_json.is_file(), f"missing {boot_json}"

    request = json.loads(request_json.read_text(encoding="utf-8"))
    validate_state = json.loads(validate_json.read_text(encoding="utf-8"))
    boot_state = json.loads(boot_json.read_text(encoding="utf-8"))

    assert request["kernel_version"] == "6.18.4"
    assert request["profiles"] == ["debug", "bpf", "rust", "samples", "debug-tools"]
    assert validate_state["stage"] == "validate"
    assert validate_state["status"] == "succeeded"
    assert boot_state["stage"] == "boot"
    assert boot_state["status"] == "placeholder"


def test_plan_writes_all_stage_state_files() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "plan",
            "--kernel",
            "6.9.8",
            "--arch",
            "arm64",
            "--image",
            "jammy",
            "--profile",
            "debug",
            "--profile",
            "rust",
            "--mirror",
            "sg",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    state_dir = request_root / "state"
    expected = [
        "validate.json",
        "fetch.json",
        "prepare.json",
        "patch.json",
        "configure.json",
        "build-kernel.json",
        "build-tools.json",
        "build-examples.json",
        "build-image.json",
        "boot.json",
    ]

    missing = [name for name in expected if not (state_dir / name).is_file()]
    assert missing == [], f"missing state files: {missing}"
