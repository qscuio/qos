#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <input-bin-dir> <output-header>" >&2
  exit 2
fi

in_bin_dir="$1"
out_header="$2"
out_dir="$(dirname "$out_header")"

cmds=(echo ps ping ip wget ls cat touch edit)

mkdir -p "$out_dir"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

header_tmp="$tmp_dir/cmd_elves.h"
cat > "$header_tmp" <<'HEADER'
#ifndef QOS_AARCH64_CMD_ELVES_H
#define QOS_AARCH64_CMD_ELVES_H

#include <stddef.h>
#include <stdint.h>

HEADER

for cmd in "${cmds[@]}"; do
  elf="$in_bin_dir/$cmd"

  if [[ ! -f "$elf" ]]; then
    echo "missing command binary: $elf" >&2
    exit 1
  fi

  {
    printf 'static const uint8_t qos_cmd_%s_elf[] = {\n' "$cmd"
    bytes_on_line=0
    while read -r -a row; do
      for b in "${row[@]}"; do
        if (( bytes_on_line % 12 == 0 )); then
          printf '    '
        fi
        printf '0x%s,' "$b"
        bytes_on_line=$((bytes_on_line + 1))
        if (( bytes_on_line % 12 == 0 )); then
          printf '\n'
        else
          printf ' '
        fi
      done
    done < <(od -An -tx1 -v "$elf")
    if (( bytes_on_line % 12 != 0 )); then
      printf '\n'
    fi
    printf '};\n\n'
  } >> "$header_tmp"
done

cat >> "$header_tmp" <<'FOOTER'
#endif /* QOS_AARCH64_CMD_ELVES_H */
FOOTER

mv "$header_tmp" "$out_header"
