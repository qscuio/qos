#!/usr/bin/env bash
set -euo pipefail

DEVICE="/dev/rust-platform"
HELPER="/home/qwert/linux-lab/examples/rust/rust_learn/user/test_platform"
SYSFS_ROOT="/sys/bus/platform/drivers/rust-platform-demo"

echo "[rust_platform] expecting ${DEVICE}"
test -e "${DEVICE}"
if [ -d "${SYSFS_ROOT}" ]; then
  echo "rust-platform-demo.0" > "${SYSFS_ROOT}/bind" 2>/dev/null || true
fi
"${HELPER}"
echo "[rust_platform] dmesg tail:"
dmesg | tail -20
