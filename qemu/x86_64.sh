#!/usr/bin/env bash
set -euo pipefail

dry_run=0
if [[ "${1:-}" == "--dry-run" ]]; then
  dry_run=1
  shift
fi

disk="${1:-qemu/configs/disk.img}"

cmd=(
  qemu-system-x86_64
  -drive "file=${disk},format=raw,if=ide"
  -device e1000,netdev=net0
  -netdev user,id=net0
  -serial stdio
  -monitor none
  -display none
  -no-reboot
  -no-shutdown
  -m 256M
)

if [[ "$dry_run" -eq 1 ]]; then
  printf '%q ' "${cmd[@]}"
  printf '\n'
  exit 0
fi

exec "${cmd[@]}"
