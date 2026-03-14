#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMG="${1:-$ROOT/build/linux1/images/linux1-disk.img}"
MEM_MB="${LINUX1_QEMU_MEM_MB:-64}"

fail() {
  local reason="$1"
  local code="${2:-1}"
  echo "ERROR:run-linux1-curses:${reason}"
  exit "$code"
}

require_cmd() {
  local cmd="$1"
  command -v "$cmd" >/dev/null 2>&1 || fail "missing tool '$cmd'" 2
}

[[ -f "$IMG" ]] || fail "missing disk image at $IMG" 2

require_cmd qemu-system-i386
require_cmd script

# LILO is configured for COM1, so keep a live serial backend attached while
# rendering the guest VGA text console through curses.
exec qemu-system-i386 \
  -drive "file=${IMG},format=raw,if=ide" \
  -serial pty \
  -monitor none \
  -display curses \
  -no-reboot \
  -no-shutdown \
  -m "$MEM_MB"
