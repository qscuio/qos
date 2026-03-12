#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <impl> <output-initramfs>" >&2
  exit 2
fi

out_initramfs="$2"
out_dir="$(dirname "$out_initramfs")"
mkdir -p "$out_dir"

tmp_file="$(mktemp)"
trap 'rm -f "$tmp_file"' EXIT

dd if=/dev/zero of="$tmp_file" bs=512 count=1 status=none
printf '070701' | dd if=/dev/stdin of="$tmp_file" bs=1 conv=notrunc status=none
mv "$tmp_file" "$out_initramfs"
