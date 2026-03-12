from __future__ import annotations

import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("impl", ["c", "rust"])
def test_x86_guest_runs_init_then_shell_and_executes_commands(impl: str) -> None:
    build = subprocess.run(
        ["make", "-C", f"{impl}-os", "ARCH=x86_64", "build"],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    assert build.returncode == 0, f"stderr:\n{build.stderr}\nstdout:\n{build.stdout}"

    disk = ROOT / f"{impl}-os" / "build" / "x86_64" / "disk.img"
    script = "\n".join(
        [
            "help",
            "ps",
            "echo qos-init-shell",
            "ping 10.0.2.2",
            "wget http://10.0.2.2:8080/",
            "exit",
            "",
        ]
    )
    run = subprocess.run(
        ["timeout", "4s", "./qemu/x86_64.sh", str(disk)],
        cwd=ROOT,
        text=True,
        capture_output=True,
        input=script,
    )
    assert run.returncode in (0, 124), f"stderr:\n{run.stderr}\nstdout:\n{run.stdout}"

    text = f"{run.stdout}\n{run.stderr}"
    assert f"QOS kernel started impl={impl} arch=x86_64" in text, text
    assert "init[1]: starting /sbin/init" in text, text
    assert "init[1]: exec /bin/sh" in text, text
    assert "qos-sh:/>" in text, text
    assert "PID PPID STATE CMD" in text, text
    assert "qos-init-shell" in text, text
    assert "PING 10.0.2.2" in text, text
    assert "64 bytes from 10.0.2.2" in text, text
    assert "HTTP/1.0 200 OK" in text, text
