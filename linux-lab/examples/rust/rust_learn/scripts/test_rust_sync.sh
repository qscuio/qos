#!/usr/bin/env bash
set -euo pipefail

DEVICE="/dev/rust-sync"
HELPER="/home/qwert/linux-lab/examples/rust/rust_learn/user/test_sync"

echo "[rust_sync] expecting ${DEVICE}"
test -c "${DEVICE}"
"${HELPER}"
echo "[rust_sync] dmesg tail:"
dmesg | tail -20
