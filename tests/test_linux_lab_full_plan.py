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


def test_run_dry_run_exercises_full_linux_lab_stage_graph() -> None:
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
    request = json.loads((request_root / "request.json").read_text(encoding="utf-8"))

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
    missing = [name for name in expected if not (request_root / "state" / name).is_file()]
    assert missing == [], f"missing state files: {missing}"

    boot_state = json.loads((request_root / "state" / "boot.json").read_text(encoding="utf-8"))
    assert request["command"] == "run"
    assert request["dry_run"] is True
    assert boot_state["status"] == "dry-run"
    assert boot_state["metadata"]["qemu_command"][0] == "qemu-system-x86_64"
