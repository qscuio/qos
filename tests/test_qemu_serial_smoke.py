from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def test_test_all_uses_real_qemu_serial_markers() -> None:
    result = _run(["make", "test-all"])
    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    logs = {
        ("c", "x86_64"): ROOT / "c-os/build/x86_64/smoke.log",
        ("c", "aarch64"): ROOT / "c-os/build/aarch64/smoke.log",
        ("rust", "x86_64"): ROOT / "rust-os/build/x86_64/smoke.log",
        ("rust", "aarch64"): ROOT / "rust-os/build/aarch64/smoke.log",
    }

    for (impl, arch), log_path in logs.items():
        assert log_path.is_file(), f"missing smoke log: {log_path}"
        text = log_path.read_text(encoding="utf-8", errors="replace")

        marker = f"QOS kernel started impl={impl} arch={arch}"
        assert marker in text, f"missing serial marker {marker!r} in {log_path}\n{text}"
        assert "qemu=" not in text, f"synthetic smoke log detected in {log_path}\n{text}"
