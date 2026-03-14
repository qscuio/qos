#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_ARCHIVE="$ROOT/build/linux1/sources/lilo-20.tar.gz"
WORK="$ROOT/build/linux1/work/lilo"
OUT="$ROOT/build/linux1/lilo"

if [[ ! -f "$SRC_ARCHIVE" ]]; then
  echo "ERROR:build-lilo:missing source archive $SRC_ARCHIVE"
  exit 2
fi

rm -rf "$WORK"
mkdir -p "$WORK" "$OUT"
tar -xf "$SRC_ARCHIVE" -C "$WORK"

srcdir="$(find "$WORK" -mindepth 1 -maxdepth 1 -type d | head -n1)"
if [[ -z "$srcdir" ]]; then
  echo "ERROR:build-lilo:could not locate extracted source directory"
  exit 2
fi

if compgen -G "$ROOT/linux-1.0.0/patches/lilo/*.patch" > /dev/null; then
  for p in "$ROOT"/linux-1.0.0/patches/lilo/*.patch; do
    patch -d "$srcdir" -p1 < "$p"
  done
fi

if ! make -C "$srcdir"; then
  echo "ERROR:build-lilo:lilo build failed"
  exit 1
fi

if [[ ! -x "$srcdir/lilo" ]]; then
  echo "ERROR:build-lilo:expected lilo binary not found"
  exit 2
fi

cp "$srcdir/lilo" "$OUT/lilo"
cp "$srcdir/boot.b" "$OUT/boot.b"
cp -a "$srcdir"/README* "$OUT"/ 2>/dev/null || true

echo "OK:build-lilo"
