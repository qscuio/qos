#!/usr/bin/env bash
set -euo pipefail

DEVICE="/dev/rust-workqueue"
HELPER="/home/qwert/linux-lab/examples/rust/rust_learn/user/test_workqueue"

echo "[rust_workqueue] expecting ${DEVICE}"
test -c "${DEVICE}"
"${HELPER}"
echo "[rust_workqueue] dmesg tail:"
dmesg | tail -20
