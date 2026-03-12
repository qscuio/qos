from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def test_make_target_delegation_matrix() -> None:
    matrix = [
        ["make", "c", "x86_64"],
        ["make", "c", "aarch64"],
        ["make", "rust", "x86_64"],
        ["make", "rust", "aarch64"],
    ]

    for cmd in matrix:
        result = _run(cmd)
        assert result.returncode == 0, (
            f"command failed: {' '.join(cmd)}\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )


def test_make_test_all_runs_four_smoke_checks() -> None:
    result = _run(["make", "test-all"])
    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    expected = (
        "SMOKE PASS: c/x86_64",
        "SMOKE PASS: c/aarch64",
        "SMOKE PASS: rust/x86_64",
        "SMOKE PASS: rust/aarch64",
    )

    for marker in expected:
        assert marker in result.stdout, f"missing marker {marker!r}\n{result.stdout}"
