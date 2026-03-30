#!/usr/bin/env bash
set -euo pipefail

DEVICE="/dev/rust-slab"
HELPER="/home/qwert/linux-lab/examples/rust/rust_learn/user/test_slab"

echo "[rust_slab] expecting ${DEVICE}"
test -c "${DEVICE}"
"${HELPER}"
echo "[rust_slab] slabinfo excerpt:"
grep -i rust /proc/slabinfo || true
