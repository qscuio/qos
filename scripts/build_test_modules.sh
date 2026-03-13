#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <x86_64|aarch64> <output-rootfs-dir>" >&2
  exit 2
fi

arch="$1"
out_rootfs="$2"
root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
src_dir="$root_dir/tools/test-modules"

case "$arch" in
  x86_64)
    cc="${X86_64_CC:-x86_64-linux-gnu-gcc}"
    ;;
  aarch64)
    cc="${AARCH64_CC:-aarch64-linux-gnu-gcc}"
    ;;
  *)
    echo "unsupported arch: $arch" >&2
    exit 2
    ;;
esac

if ! command -v "$cc" >/dev/null 2>&1; then
  echo "missing compiler: $cc" >&2
  exit 1
fi

lib_dir="$out_rootfs/lib"
mod_dir="$lib_dir/modules"
lib_out="$lib_dir/libqos_test.so"
mod_out="$mod_dir/qos_test.ko"

mkdir -p "$lib_dir" "$mod_dir"

cflags=(
  -std=c11
  -O2
  -fPIC
  -ffreestanding
  -fno-builtin
  -fvisibility=hidden
)

"$cc" "${cflags[@]}" -shared -nostdlib -Wl,--build-id=none -Wl,-soname,libqos_test.so \
  -o "$lib_out" "$src_dir/qos_testlib.c"
"$cc" "${cflags[@]}" -shared -nostdlib -Wl,--build-id=none -Wl,-soname,qos_test.ko \
  -o "$mod_out" "$src_dir/qos_testmod.c"
