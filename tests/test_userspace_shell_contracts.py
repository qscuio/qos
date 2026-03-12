from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str], cwd: Path | None = None, input_text: str | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=(cwd or ROOT),
        text=True,
        input=input_text,
        capture_output=True,
    )


def _build_c_shell() -> Path:
    src = ROOT / "c-os/userspace/shell.c"
    out = ROOT / "c-os/build/qos-sh-c"
    out.parent.mkdir(parents=True, exist_ok=True)
    build = _run(["gcc", "-std=c11", "-O2", "-o", str(out), str(src)])
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}"
    return out


def test_c_shell_supports_core_program_contracts() -> None:
    shell = _build_c_shell()
    run = _run(
        [str(shell)],
        input_text=(
            "ls\n"
            "cat /proc/meminfo\n"
            "mkdir /tmp/work\n"
            "rm /tmp/work\n"
            "ps\n"
            "ip addr\n"
            "ip route\n"
            "ping 10.0.2.2\n"
            "wget http://10.0.2.2:8080/\n"
            "exit\n"
        ),
    )
    assert run.returncode == 0, f"shell failed:\n{run.stderr}\nstdout:\n{run.stdout}"
    text = run.stdout
    assert "proc" in text
    assert "MemTotal:" in text
    assert "created /tmp/work" in text
    assert "removed /tmp/work" in text
    assert "PID" in text
    assert "inet 10.0.2.15/24" in text
    assert "default via 10.0.2.2" in text
    assert "PING 10.0.2.2" in text
    assert "HTTP/1.0 200 OK" in text


def _run_rust_once(command: str) -> str:
    result = _run(
        ["cargo", "run", "--quiet", "-p", "qos-userspace", "--", "--once", command],
        cwd=ROOT / "rust-os",
    )
    assert result.returncode == 0, f"cargo run failed:\n{result.stderr}\nstdout:\n{result.stdout}"
    return result.stdout


def test_rust_userspace_once_mode_supports_core_program_contracts() -> None:
    assert "proc" in _run_rust_once("ls")
    assert "MemTotal:" in _run_rust_once("cat /proc/meminfo")
    assert "created /tmp/work" in _run_rust_once("mkdir /tmp/work")
    assert "removed /tmp" in _run_rust_once("rm /tmp")
    assert "PID" in _run_rust_once("ps")
    assert "inet 10.0.2.15/24" in _run_rust_once("ip addr")
    assert "default via 10.0.2.2" in _run_rust_once("ip route")
    assert "PING 10.0.2.2" in _run_rust_once("ping 10.0.2.2")
    assert "HTTP/1.0 200 OK" in _run_rust_once("wget http://10.0.2.2:8080/")


def test_shell_ping_reports_unreachable_for_unknown_host() -> None:
    shell = _build_c_shell()
    c_run = _run([str(shell)], input_text="ping 10.0.2.99\nexit\n")
    assert c_run.returncode == 0, f"shell failed:\n{c_run.stderr}\nstdout:\n{c_run.stdout}"
    assert "Destination Host Unreachable" in c_run.stdout

    rust_out = _run_rust_once("ping 10.0.2.99")
    assert "Destination Host Unreachable" in rust_out
