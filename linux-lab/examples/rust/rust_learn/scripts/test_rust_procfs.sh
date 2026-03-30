#!/usr/bin/env bash
set -euo pipefail

ROOT="/proc/rust_procfs"

echo "[rust_procfs] expecting ${ROOT}"
test -d "${ROOT}"
echo "echo_mode=off" > "${ROOT}/config"
echo "[rust_procfs] stats:"
cat "${ROOT}/stats"
echo "[rust_procfs] config:"
cat "${ROOT}/config"
