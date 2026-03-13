#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <output-bin-dir>" >&2
  exit 2
fi

out_bin_dir="$1"
root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
src_dir="$root_dir/tools/aarch64-c-probe/cmd-src"
cc="${X86_64_CC:-x86_64-linux-gnu-gcc}"
ld="${X86_64_LD:-x86_64-linux-gnu-ld}"

cmds=(echo ps ping ip wget ls cat touch edit insmod rmmod wqdemo)

mkdir -p "$out_bin_dir"

if ! command -v "$cc" >/dev/null 2>&1; then
  echo "missing x86_64 compiler: $cc" >&2
  exit 1
fi
if ! command -v "$ld" >/dev/null 2>&1; then
  echo "missing x86_64 linker: $ld" >&2
  exit 1
fi

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

cflags=(
  -m64
  -std=c11
  -ffreestanding
  -fno-builtin
  -fdata-sections
  -ffunction-sections
  -fno-stack-protector
  -fno-asynchronous-unwind-tables
  -fno-unwind-tables
  -fno-exceptions
)

for cmd in "${cmds[@]}"; do
  src="$src_dir/$cmd.c"
  obj="$tmp_dir/$cmd.o"
  elf="$out_bin_dir/$cmd"

  if [[ ! -f "$src" ]]; then
    echo "missing command source: $src" >&2
    exit 1
  fi

  "$cc" -c "${cflags[@]}" -o "$obj" "$src"
  "$ld" -m elf_x86_64 -nostdlib --gc-sections --strip-all "$obj" -o "$elf"
done
