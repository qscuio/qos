#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$ROOT/build/linux1/images"
WORK_DIR="$ROOT/build/linux1/work/disk"
IMG="${LINUX1_DISK_IMAGE:-$OUT_DIR/linux1-disk.img}"
MNT="$WORK_DIR/mnt"
NBD_DEV="${LINUX1_NBD_DEV:-/dev/nbd0}"
PDEV="${NBD_DEV}p1"
DISK_SIZE_MB="${LINUX1_DISK_SIZE_MB:-64}"
PART_START="${LINUX1_PART_START_SECTOR:-2048}"
HEADS="${LINUX1_HEADS:-16}"
SECTORS="${LINUX1_SECTORS:-63}"

KERNEL="$ROOT/build/linux1/kernel/vmlinuz-1.0.0"
ROOTFS="$ROOT/build/linux1/rootfs"
LILO_BIN="$ROOT/build/linux1/lilo/lilo"
LILO_BOOT="$ROOT/build/linux1/lilo/boot.b"

fail() {
  local reason="$1"
  local code="${2:-1}"
  echo "ERROR:mk-linux1-disk:${reason}"
  exit "$code"
}

cleanup() {
  set +e
  if mountpoint -q "$MNT"; then
    sudo umount "$MNT"
  fi
  sudo qemu-nbd --disconnect "$NBD_DEV" >/dev/null 2>&1 || true
}
trap cleanup EXIT

require_cmd() {
  local cmd="$1"
  command -v "$cmd" >/dev/null 2>&1 || fail "missing tool '$cmd'" 2
}

require_cmd sudo
require_cmd qemu-nbd
require_cmd sfdisk
require_cmd mkfs.ext2
require_cmd partprobe

[[ -f "$KERNEL" ]] || fail "missing kernel image at $KERNEL" 2
[[ -x "$LILO_BIN" ]] || fail "missing lilo binary at $LILO_BIN" 2
[[ -f "$LILO_BOOT" ]] || fail "missing lilo boot sector at $LILO_BOOT" 2
[[ -d "$ROOTFS" ]] || fail "missing staged rootfs at $ROOTFS" 2

mkdir -p "$OUT_DIR" "$WORK_DIR" "$MNT"

truncate -s "${DISK_SIZE_MB}M" "$IMG"

sudo modprobe nbd max_part=8 || true
sudo qemu-nbd --disconnect "$NBD_DEV" >/dev/null 2>&1 || true
sudo qemu-nbd --connect="$NBD_DEV" --format=raw "$IMG" || fail "qemu-nbd connect failed"

for _ in $(seq 1 50); do
  size_bytes="$(sudo blockdev --getsize64 "$NBD_DEV" 2>/dev/null || echo 0)"
  if [[ "$size_bytes" -gt 0 ]]; then
    break
  fi
  sleep 0.1
done
[[ "${size_bytes:-0}" -gt 0 ]] || fail "nbd device did not become ready"

total_sectors=$(( size_bytes / 512 ))
cylinders=$(( total_sectors / (HEADS * SECTORS) ))
if [[ "$cylinders" -lt 1 ]]; then
  cylinders=1
fi

printf 'label: dos\nunit: sectors\n%s1 : start=%s, type=83, bootable\n' "$NBD_DEV" "$PART_START" | \
  sudo sfdisk "$NBD_DEV" >/dev/null || fail "partitioning failed"

# Modern sfdisk writes DOS CHS fields using 255/63 translation and no longer
# exposes geometry override flags. LILO must see partition CHS that matches the
# BIOS geometry configured below, or the MBR can stall at "LI".
sudo python3 - "$NBD_DEV" "$HEADS" "$SECTORS" <<'PY'
import sys


def encode_chs(lba: int, heads: int, sectors: int) -> bytes:
    cyl, rem = divmod(lba, heads * sectors)
    head, sec0 = divmod(rem, sectors)
    sec = sec0 + 1
    if cyl > 1023:
        return bytes((254, 255, 255))
    return bytes((head, sec | ((cyl >> 2) & 0xC0), cyl & 0xFF))


dev = sys.argv[1]
heads = int(sys.argv[2])
sectors = int(sys.argv[3])

with open(dev, "r+b", buffering=0) as disk:
    mbr = bytearray(disk.read(512))
    part = 0x1BE
    start_lba = int.from_bytes(mbr[part + 8 : part + 12], "little")
    size = int.from_bytes(mbr[part + 12 : part + 16], "little")
    if not start_lba or not size:
        raise SystemExit("partition entry missing LBA/size")
    end_lba = start_lba + size - 1
    mbr[part + 1 : part + 4] = encode_chs(start_lba, heads, sectors)
    mbr[part + 5 : part + 8] = encode_chs(end_lba, heads, sectors)
    disk.seek(0)
    disk.write(mbr)
PY

sudo partprobe "$NBD_DEV" || true
for _ in $(seq 1 50); do
  if [[ -b "$PDEV" ]]; then
    break
  fi
  sleep 0.1
done
[[ -b "$PDEV" ]] || fail "partition device missing ($PDEV)"

# Linux 1.0.0 expects early ext2 on-disk format (revision 0 static superblock).
sudo mkfs.ext2 -F -q -b 1024 -I 128 -O none -r 0 "$PDEV" || fail "mkfs.ext2 failed"

sudo mount "$PDEV" "$MNT" || fail "mount failed"

sudo mkdir -p "$MNT/boot" "$MNT/dev" "$MNT/etc"
sudo cp -a "$ROOTFS/." "$MNT/"
sudo cp "$KERNEL" "$MNT/boot/vmlinuz-1.0.0"
sudo cp "$KERNEL" "$MNT/boot/vmlinuz"
sudo cp "$LILO_BOOT" "$MNT/boot/boot.b"

sudo mknod -m 600 "$MNT/dev/console" c 5 1 2>/dev/null || true
sudo mknod -m 666 "$MNT/dev/null" c 1 3 2>/dev/null || true
sudo mknod -m 620 "$MNT/dev/tty1" c 4 1 2>/dev/null || true
sudo mknod -m 666 "$MNT/dev/ttyS0" c 4 64 2>/dev/null || true

LILO_CONF="$WORK_DIR/lilo.conf"
cat > "$LILO_CONF" <<CONF
boot=$NBD_DEV
map=$MNT/boot/map
install=$MNT/boot/boot.b
prompt
timeout=50
serial=0,9600n8
disk=$NBD_DEV
  bios=0x80
  sectors=$SECTORS
  heads=$HEADS
  cylinders=$cylinders
  partition=$PDEV
    start=$PART_START
image=$MNT/boot/vmlinuz-1.0.0
  label=linux
  root=0x301
  read-only
  append="console=ttyS0"
CONF

if ! sudo "$LILO_BIN" -v -v -C "$LILO_CONF"; then
  fail "lilo install failed"
fi

sync
sudo umount "$MNT"
sudo qemu-nbd --disconnect "$NBD_DEV" >/dev/null 2>&1 || true
trap - EXIT

echo "OK:mk-linux1-disk"
