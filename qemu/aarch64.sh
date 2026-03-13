#!/usr/bin/env bash
set -euo pipefail

dry_run=0
if [[ "${1:-}" == "--dry-run" ]]; then
  dry_run=1
  shift
fi

disk="${1:-qemu/configs/disk.img}"
kernel="${2:-boot.elf}"
initrd="${3:-}"
pcap_path="${QOS_PCAP_PATH:-}"
hostfwd_host_port="${QOS_HOSTFWD_UDP_HOST_PORT:-}"
hostfwd_guest_port="${QOS_HOSTFWD_UDP_GUEST_PORT:-}"
mapwatch_log="${QOS_MAPWATCH_LOG:-/tmp/qos-mapwatch.log}"
netdev_arg="user,id=net0"

if [[ -n "$hostfwd_host_port" && -n "$hostfwd_guest_port" ]]; then
  netdev_arg="${netdev_arg},hostfwd=udp::${hostfwd_host_port}-:${hostfwd_guest_port}"
fi

cmd=(
  qemu-system-aarch64
  -machine virt
  -cpu cortex-a57
  -kernel "$kernel"
  -drive "if=none,file=${disk},format=raw,id=disk0"
  -device virtio-blk-device,drive=disk0
  -netdev "$netdev_arg"
  -device virtio-net-device,netdev=net0
  -serial stdio
  -monitor none
  -display none
  -no-reboot
  -no-shutdown
  -m 256M
)

if [[ -n "$initrd" ]]; then
  cmd+=( -initrd "$initrd" )
fi

if [[ -n "$pcap_path" ]]; then
  mkdir -p "$(dirname "$pcap_path")"
  cmd+=( -object "filter-dump,id=f0,netdev=net0,file=${pcap_path}" )
fi

if [[ "$mapwatch_log" != "off" ]]; then
  if [[ "$dry_run" -eq 0 ]]; then
    mkdir -p "$(dirname "$mapwatch_log")"
    : > "$mapwatch_log"
  fi
  cmd+=(
    -chardev "file,id=mapwatch,path=${mapwatch_log},append=off"
    -semihosting-config "enable=on,target=native,chardev=mapwatch"
  )
fi

if [[ "$dry_run" -eq 1 ]]; then
  printf '%q ' "${cmd[@]}"
  printf '\n'
  exit 0
fi

exec "${cmd[@]}"
