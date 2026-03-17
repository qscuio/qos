#!/usr/bin/env bash
# qemu-mm-walk.sh — translate a guest VA to PA via the QEMU monitor,
# then read 4 bytes from the physical address.
#
# Usage: ./scripts/qemu-mm-walk.sh <va>
#   va: guest virtual address (hex with 0x prefix, e.g. 0x55a3f2b01000)
#
# Requires: socat OR nc with Unix socket support
# Requires: QEMU started with -monitor unix:/tmp/qemu-monitor.sock,server,nowait

set -euo pipefail

SOCK="/tmp/qemu-monitor.sock"
VA="${1:-}"

if [[ -z "$VA" ]]; then
    echo "Usage: $0 <va>"
    echo "  va: guest virtual address (e.g. 0x55a3f2b01000)"
    exit 1
fi

# Verify socket exists
if [[ ! -S "$SOCK" ]]; then
    echo "Error: QEMU monitor socket not found at $SOCK"
    echo ""
    echo "Start QEMU with:"
    echo "  -monitor unix:${SOCK},server,nowait"
    exit 1
fi

# Pick a monitor client tool
if command -v socat &>/dev/null; then
    send_cmd() { printf '%s\n' "$1" | socat - "UNIX-CONNECT:${SOCK}" 2>/dev/null; }
elif nc -U /dev/null 2>/dev/null; then
    send_cmd() { printf '%s\n' "$1" | nc -U "$SOCK" 2>/dev/null; }
else
    echo "Error: neither socat nor nc (with -U) found"
    echo "  Install socat: apt install socat"
    exit 1
fi

echo "==> gva2gpa $VA"
gva_output=$(send_cmd "gva2gpa $VA")
echo "$gva_output"

# Parse PA from "0x<va> -> 0x<pa>" line
PA=$(echo "$gva_output" | grep -oP '-> \K0x[0-9a-fA-F]+' || true)
if [[ -z "$PA" ]]; then
    echo "gva2gpa failed — page is not present (not yet faulted in)"
    exit 1
fi

echo ""
echo "==> xp /4xb $PA"
send_cmd "xp /4xb $PA"
