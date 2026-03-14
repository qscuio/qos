#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/linux-1.0.0/userspace/src"
OUT="$ROOT/build/linux1/rootfs"
LINKER_SCRIPT="$ROOT/scripts/linux1/userspace.ld"
TOOLPREFIX="${TOOLPREFIX:-i686-linux-gnu-}"
AS="${AS:-${TOOLPREFIX}as}"
LD="${LD:-${TOOLPREFIX}ld}"

mkdir -p "$OUT/sbin" "$OUT/bin"

[[ -f "$LINKER_SCRIPT" ]] || {
  echo "ERROR:build-linux1-userspace:missing linker script at $LINKER_SCRIPT"
  exit 1
}

build_one() {
  local src="$1"
  local out="$2"
  local obj="${out}.o"
  if ! "$AS" --32 -I "$SRC" -o "$obj" "$src"; then
    echo "ERROR:build-linux1-userspace:assemble failed for $src"
    exit 1
  fi
  if ! "$LD" -m elf_i386 -static -nostdlib -T "$LINKER_SCRIPT" -o "$out" "$obj"; then
    echo "ERROR:build-linux1-userspace:link failed for $src"
    exit 1
  fi
  rm -f "$obj"
}

build_one "$SRC/init.S" "$OUT/sbin/init"
build_one "$SRC/sh.S" "$OUT/bin/sh"
build_one "$SRC/echo.S" "$OUT/bin/echo"

echo "OK:build-linux1-userspace"
