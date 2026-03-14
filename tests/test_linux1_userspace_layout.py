import os
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ROOTFS = ROOT / "build" / "linux1" / "rootfs"
BINARIES = [
    ROOTFS / "sbin" / "init",
    ROOTFS / "bin" / "sh",
    ROOTFS / "bin" / "echo",
]


def _program_headers(path: Path) -> list[str]:
    timeout_sec = int(os.environ.get("LINUX1_BUILD_TIMEOUT_SEC", "1800"))
    readelf = subprocess.run(
        ["readelf", "-l", str(path)],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_sec,
    )
    assert readelf.returncode == 0, readelf.stdout.decode(errors="replace")
    return readelf.stdout.decode(errors="replace").splitlines()


def test_linux1_userspace_segments_are_linux10_elf_loader_friendly():
    timeout_sec = int(os.environ.get("LINUX1_BUILD_TIMEOUT_SEC", "1800"))
    result = subprocess.run(
        ["bash", "scripts/build_linux1_userspace.sh"],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_sec,
    )
    assert result.returncode == 0, result.stdout.decode(errors="replace")

    for binary in BINARIES:
        assert binary.exists(), f"missing userspace binary: {binary}"
        load_segments = []
        for line in _program_headers(binary):
            stripped = line.strip()
            if not stripped.startswith("LOAD"):
                continue
            parts = stripped.split()
            load_segments.append(
                {
                    "offset": int(parts[1], 16),
                    "vaddr": int(parts[2], 16),
                    "filesz": int(parts[4], 16),
                }
            )

        assert len(load_segments) == 1, (
            f"{binary} should use a single PT_LOAD segment so Linux 1.0's ELF "
            f"loader does not trip over segment-tail padzero() assumptions: "
            f"{load_segments}"
        )

        segment = load_segments[0]
        assert (segment["vaddr"] + segment["filesz"]) % 0x1000 == 0, (
            f"{binary} must end its file-backed load segment on a page boundary; "
            f"got vaddr=0x{segment['vaddr']:x} filesz=0x{segment['filesz']:x}"
        )
