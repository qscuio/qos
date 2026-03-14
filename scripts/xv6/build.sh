#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT/xv6"
make \
  TOOLPREFIX=i686-linux-gnu- \
  CC='i686-linux-gnu-gcc -Wno-error=array-bounds -Wno-error=infinite-recursion' \
  xv6.img fs.img
