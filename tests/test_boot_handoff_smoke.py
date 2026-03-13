from __future__ import annotations

import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def test_smoke_logs_include_stage_and_handoff_markers() -> None:
    result = _run(["make", "test-all"])
    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    cases = {
        (
            "c",
            "x86_64",
        ): (
            "stage=stage2",
            "boot_info_addr=0x7000",
            "handoff=boot_info",
            "entry=kernel_main",
            "abi=stack_ptr",
            "a20=enabled",
            "kernel_load=elf",
            "kernel_magic=ELF",
            "kernel_entry_nonzero=1",
            "kernel_phdr_nonzero=1",
            "mode_transition=pmode",
            "pmode_serial=32bit",
            "mmap_source=e820",
            "mmap_first_type=usable",
            "mmap_first_len_nonzero=1",
            "initramfs_load=cpio",
            "acpi_rsdp_addr=0",
            "dtb_addr=0",
        ),
        (
            "rust",
            "x86_64",
        ): (
            "stage=stage2",
            "boot_info_addr=0x7000",
            "handoff=boot_info",
            "entry=kernel_main",
            "abi=stack_ptr",
            "a20=enabled",
            "kernel_load=elf",
            "kernel_magic=ELF",
            "kernel_entry_nonzero=1",
            "kernel_phdr_nonzero=1",
            "mode_transition=pmode",
            "pmode_serial=32bit",
            "mmap_source=e820",
            "mmap_first_type=usable",
            "mmap_first_len_nonzero=1",
            "initramfs_load=cpio",
            "acpi_rsdp_addr=0",
            "dtb_addr=0",
        ),
        ("c", "aarch64"): (
            "handoff=boot_info",
            "entry=kernel_main",
            "abi=x0",
            "dtb_addr_nonzero=1",
            "dtb_magic=ok",
            "dtb_handoff=",
            "mmap_source=dtb",
            "mmap_count=",
            "mmap_len_nonzero=1",
            "initramfs_source=",
            "initramfs_size_nonzero=1",
            "irq_timer=ok",
            "icmp_echo=gateway_ok",
            "net_tx=real_ok",
        ),
        ("rust", "aarch64"): (
            "handoff=boot_info",
            "entry=kernel_main",
            "abi=x0",
            "dtb_addr_nonzero=1",
            "dtb_magic=ok",
            "dtb_handoff=",
            "mmap_source=dtb",
            "mmap_count=",
            "mmap_len_nonzero=1",
            "initramfs_source=",
            "initramfs_size_nonzero=1",
            "irq_timer=ok",
            "icmp_echo=gateway_ok",
            "net_tx=real_ok",
        ),
    }

    for (impl, arch), required_markers in cases.items():
        log = ROOT / impl.replace("rust", "rust-os").replace("c", "c-os") / "build" / arch / "smoke.log"
        text = log.read_text(encoding="utf-8", errors="replace")
        for required in required_markers:
            assert required in text, f"missing {required!r} in {log}\n{text}"

        if arch == "x86_64":
            match = re.search(r"mmap_count=0x([0-9A-Fa-f]{4})", text)
            assert match, f"missing hex mmap_count in {log}\n{text}"
            count = int(match.group(1), 16)
            assert count > 0, f"expected mmap_count > 0 in {log}, got {count}\n{text}"
            disk_match = re.search(r"disk_load=(lba_ext|chs)", text)
            assert disk_match, f"missing disk_load marker in {log}\n{text}"
        if arch == "aarch64":
            match = re.search(r"mmap_count=([0-9]+)", text)
            assert match, f"missing decimal mmap_count in {log}\n{text}"
            count = int(match.group(1))
            assert count >= 3, f"expected mmap_count >= 3 in {log}, got {count}\n{text}"
            dtb_handoff = re.search(r"dtb_handoff=(x0|scan)", text)
            assert dtb_handoff, f"expected dtb_handoff=(x0|scan) in {log}\n{text}"
            initramfs_source = re.search(r"initramfs_source=(dtb|scan)", text)
            assert initramfs_source, f"expected initramfs_source=(dtb|scan) in {log}\n{text}"
