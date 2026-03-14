import os
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DISK = ROOT / "build" / "linux1" / "images" / "linux1-disk.img"
PART_START = int(os.environ.get("LINUX1_PART_START_SECTOR", "2048"))
HEADS = int(os.environ.get("LINUX1_HEADS", "16"))
SECTORS = int(os.environ.get("LINUX1_SECTORS", "63"))


def run_step(*args: str) -> None:
    timeout_sec = int(os.environ.get("LINUX1_BUILD_TIMEOUT_SEC", "1800"))
    result = subprocess.run(
        list(args),
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_sec,
    )
    assert result.returncode == 0, result.stdout.decode(errors="replace")


def decode_chs(entry: bytes) -> tuple[int, int, int]:
    head = entry[1]
    sector = entry[2] & 0x3F
    cylinder = ((entry[2] & 0xC0) << 2) | entry[3]
    return cylinder, head, sector


def lba_to_chs(lba: int) -> tuple[int, int, int]:
    cylinder, rem = divmod(lba, HEADS * SECTORS)
    head, sector0 = divmod(rem, SECTORS)
    return cylinder, head, sector0 + 1


def test_linux1_disk_partition_chs_matches_lilo_geometry():
    run_step("bash", "scripts/linux1/fetch_sources.sh")
    run_step("bash", "scripts/linux1/build_kernel.sh")
    run_step("bash", "scripts/linux1/build_userspace.sh")
    run_step("bash", "scripts/linux1/build_lilo.sh")
    run_step("bash", "scripts/linux1/mk_disk.sh")

    assert DISK.exists(), f"missing disk image: {DISK}"

    with DISK.open("rb") as image:
        image.seek(0x1BE)
        partition = image.read(16)

    start_lba = int.from_bytes(partition[8:12], "little")
    actual = decode_chs(partition)
    expected = lba_to_chs(start_lba)

    assert start_lba == PART_START, (
        f"unexpected partition start LBA: got {start_lba}, expected {PART_START}"
    )
    assert actual == expected, (
        "MBR partition CHS must match the geometry passed to LILO; "
        f"got CHS={actual} for LBA {start_lba}, expected {expected} "
        f"with HEADS={HEADS} SECTORS={SECTORS}"
    )
