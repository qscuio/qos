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


def _script() -> str:
    return (
        "mkdir /tmp/work\n"
        "cd /tmp/work\n"
        "pwd\n"
        "echo \"hello qos\" > /tmp/work/msg\n"
        "cat /tmp/work/msg\n"
        "echo 'pipe data' | cat\n"
        "export PATH=/bin:/usr/bin\n"
        "unset PATH\n"
        "exit\n"
    )


def _path_script() -> str:
    return (
        "export PATH=/sbin\n"
        "ls\n"
        "/bin/ls\n"
        "export PATH=/bin:/usr/bin\n"
        "ls\n"
        "unset PATH\n"
        "ls\n"
        "exit\n"
    )


def test_c_shell_supports_quotes_redirection_pipes_and_builtins() -> None:
    shell = _build_c_shell()
    run = _run([str(shell)], input_text=_script())
    assert run.returncode == 0, f"shell failed:\n{run.stderr}\nstdout:\n{run.stdout}"
    text = run.stdout
    assert "/tmp/work" in text
    assert "hello qos" in text
    assert "pipe data" in text
    assert "set PATH" in text
    assert "unset PATH" in text
    assert "qos-sh:/>" in text
    assert "qos-sh:/tmp/work>" in text


def test_rust_shell_supports_quotes_redirection_pipes_and_builtins() -> None:
    run = _run(
        ["cargo", "run", "--quiet", "-p", "qos-userspace"],
        cwd=ROOT / "rust-os",
        input_text=_script(),
    )
    assert run.returncode == 0, f"shell failed:\n{run.stderr}\nstdout:\n{run.stdout}"
    text = run.stdout
    assert "/tmp/work" in text
    assert "hello qos" in text
    assert "pipe data" in text
    assert "set PATH" in text
    assert "unset PATH" in text
    assert "qos-sh:/>" in text
    assert "qos-sh:/tmp/work>" in text


def test_c_shell_resolves_commands_via_path() -> None:
    shell = _build_c_shell()
    run = _run([str(shell)], input_text=_path_script())
    assert run.returncode == 0, f"shell failed:\n{run.stderr}\nstdout:\n{run.stdout}"
    text = run.stdout
    assert "set PATH=/sbin" in text
    assert text.count("unknown command: ls") >= 2
    assert "set PATH=/bin:/usr/bin" in text
    assert "/tmp" in text


def test_rust_shell_resolves_commands_via_path() -> None:
    run = _run(
        ["cargo", "run", "--quiet", "-p", "qos-userspace"],
        cwd=ROOT / "rust-os",
        input_text=_path_script(),
    )
    assert run.returncode == 0, f"shell failed:\n{run.stderr}\nstdout:\n{run.stdout}"
    text = run.stdout
    assert "set PATH=/sbin" in text
    assert text.count("unknown command: ls") >= 2
    assert "set PATH=/bin:/usr/bin" in text
    assert "/tmp" in text
