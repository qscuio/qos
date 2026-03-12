from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=(cwd or ROOT), text=True, capture_output=True)


def test_qemu_launchers_match_arch_device_contracts() -> None:
    x86 = _run(["bash", "qemu/x86_64.sh", "--dry-run", "/tmp/qos-x86.img"])
    assert x86.returncode == 0, f"stderr:\n{x86.stderr}\nstdout:\n{x86.stdout}"
    x86_cmd = x86.stdout
    assert "qemu-system-x86_64" in x86_cmd
    assert "-drive" in x86_cmd and "file=/tmp/qos-x86.img" in x86_cmd and "if=ide" in x86_cmd
    assert "-device" in x86_cmd and "e1000" in x86_cmd and "netdev=net0" in x86_cmd
    assert "-netdev" in x86_cmd and "user" in x86_cmd and "id=net0" in x86_cmd

    arm = _run(
        [
            "bash",
            "qemu/aarch64.sh",
            "--dry-run",
            "/tmp/qos-arm.img",
            "/tmp/qos-kernel.elf",
            "/tmp/qos-initrd.cpio",
        ]
    )
    assert arm.returncode == 0, f"stderr:\n{arm.stderr}\nstdout:\n{arm.stdout}"
    arm_cmd = arm.stdout
    assert "qemu-system-aarch64" in arm_cmd
    assert "-machine virt" in arm_cmd
    assert "-cpu cortex-a57" in arm_cmd
    assert "-device" in arm_cmd and "virtio-blk-device" in arm_cmd and "drive=disk0" in arm_cmd
    assert "-device" in arm_cmd and "virtio-net-device" in arm_cmd and "netdev=net0" in arm_cmd
    assert "-initrd /tmp/qos-initrd.cpio" in arm_cmd


def test_smoke_logs_emit_arch_boundary_markers() -> None:
    result = _run(["make", "test-all"])
    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    x86_logs = [
        ROOT / "c-os/build/x86_64/smoke.log",
        ROOT / "rust-os/build/x86_64/smoke.log",
    ]
    for log in x86_logs:
        text = log.read_text(encoding="utf-8", errors="replace")
        assert "paging=pae" in text
        assert "long_mode=1" in text
        assert "kernel_va_high=1" in text

    aarch64_logs = [
        ROOT / "c-os/build/aarch64/smoke.log",
        ROOT / "rust-os/build/aarch64/smoke.log",
    ]
    for log in aarch64_logs:
        text = log.read_text(encoding="utf-8", errors="replace")
        assert "el=el1" in text
        assert "mmu=on" in text
        assert "ttbr_split=user-kernel" in text
        assert "virtio_discovery=dtb" in text
