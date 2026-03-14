#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KDIR="$ROOT/linux-1.0.0"
OUT="$ROOT/build/linux1/kernel"
TOOLPREFIX="${TOOLPREFIX:-i686-linux-gnu-}"

mkdir -p "$OUT"

if [[ ! -d "$KDIR" ]]; then
  echo "ERROR:build-kernel:missing kernel tree at $KDIR"
  exit 2
fi

if [[ ! -x "$(command -v as86 || true)" ]] || [[ ! -x "$(command -v ld86 || true)" ]]; then
  echo "ERROR:build-kernel:as86/ld86 missing (install bin86)"
  exit 2
fi

# Seed default config non-interactively when needed.
if [[ ! -f "$KDIR/.config" ]]; then
  set +o pipefail
  yes '' | script -qefc "make -C \"$KDIR\" config" /dev/null >/dev/null
  cfg_status=$?
  set -o pipefail
  if [[ $cfg_status -ne 0 ]]; then
    echo "ERROR:build-kernel:non-interactive config failed"
    exit 1
  fi
fi

MAKE_ARGS=(
  -C "$KDIR"
  CC="${TOOLPREFIX}gcc -D__KERNEL__ -fno-pie -no-pie -fcommon -fno-stack-protector -I$KDIR/include -I$KDIR/net/inet"
  # Linux 1.0.0 assumes toolchains that prefix C symbols with '_' (a.out ABI).
  CFLAGS="-Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -pipe -march=i486 -fcf-protection=none -std=gnu89 -fgnu89-inline -fleading-underscore -fno-builtin"
  AS="${TOOLPREFIX}as"
  LD="${TOOLPREFIX}ld"
  AR="${TOOLPREFIX}ar"
  OBJCOPY="${TOOLPREFIX}objcopy"
  STRIP="${TOOLPREFIX}strip"
  HOSTCC="gcc"
  HOSTCFLAGS="-O2 -I$KDIR/include"
  AS86="as86 -0 -a"
  LD86="ld86 -0"
)

if ! make "${MAKE_ARGS[@]}" clean; then
  echo "ERROR:build-kernel:linux 1.0.0 clean failed"
  exit 1
fi

if ! make "${MAKE_ARGS[@]}" depend; then
  echo "ERROR:build-kernel:linux 1.0.0 depend generation failed"
  exit 1
fi

if ! make "${MAKE_ARGS[@]}" zImage; then
  echo "ERROR:build-kernel:linux 1.0.0 zImage build failed"
  exit 1
fi

cp "$KDIR/zImage" "$OUT/vmlinuz-1.0.0"
echo "OK:build-kernel"
