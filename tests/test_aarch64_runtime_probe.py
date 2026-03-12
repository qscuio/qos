from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def test_rust_aarch64_smoke_reports_kernel_icmp_probe_marker() -> None:
    result = _run(["make", "test-all"])
    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    log = ROOT / "rust-os/build/aarch64/smoke.log"
    text = log.read_text(encoding="utf-8", errors="replace")
    assert "icmp_echo=gateway_ok" in text, f"missing icmp probe marker in {log}\n{text}"


def test_c_aarch64_smoke_reports_kernel_icmp_probe_marker() -> None:
    result = _run(["make", "-C", "c-os", "ARCH=aarch64", "smoke"])
    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    log = ROOT / "c-os/build/aarch64/smoke.log"
    text = log.read_text(encoding="utf-8", errors="replace")
    assert "icmp_echo=gateway_ok" in text, f"missing icmp probe marker in {log}\n{text}"
