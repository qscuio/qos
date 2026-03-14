import os
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
KDIR = ROOT / "linux-1.0.0"
ZSYSTEM = KDIR / "tools" / "zSystem"


def test_linux1_kernel_entrypoint_layout():
    timeout_sec = int(os.environ.get("LINUX1_BUILD_TIMEOUT_SEC", "1800"))
    result = subprocess.run(
        ["bash", "scripts/linux1/build_kernel.sh"],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_sec,
    )
    assert result.returncode == 0, result.stdout.decode(errors="replace")
    assert ZSYSTEM.exists(), f"missing kernel ELF: {ZSYSTEM}"

    nm = subprocess.run(
        ["nm", "-n", str(ZSYSTEM)],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_sec,
    )
    assert nm.returncode == 0, nm.stdout.decode(errors="replace")

    startup_addr = None
    for line in nm.stdout.decode(errors="replace").splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == "startup_32":
            startup_addr = parts[0]
            break

    assert startup_addr == "00100000", (
        "startup_32 must be linked at 0x00100000 so the decompressor jump "
        f"lands in boot/head.S; got {startup_addr!r}"
    )


def test_linux1_schedule_switch_uses_stable_far_pointer():
    timeout_sec = int(os.environ.get("LINUX1_BUILD_TIMEOUT_SEC", "1800"))
    result = subprocess.run(
        ["bash", "scripts/linux1/build_kernel.sh"],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_sec,
    )
    assert result.returncode == 0, result.stdout.decode(errors="replace")

    disasm = subprocess.run(
        [
            "gdb-multiarch",
            "-q",
            str(ZSYSTEM),
            "-ex",
            "set disassembly-flavor intel",
            "-ex",
            "disassemble _schedule,+420",
            "-ex",
            "quit",
        ],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_sec,
    )
    assert disasm.returncode == 0, disasm.stdout.decode(errors="replace")

    text = disasm.stdout.decode(errors="replace")
    assert "jmp    FWORD PTR [ecx+" not in text, (
        "switch_to() must not build the far jump operand through %ecx: "
        "xchg %%ecx,_current rewrites %%ecx to the old task pointer before ljmp.\n"
        f"{text}"
    )
