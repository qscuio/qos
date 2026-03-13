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
dtb_mode="${QOS_AARCH64_DTB:-auto}"
dtb_path="${QOS_AARCH64_DTB_PATH:-/tmp/qos-aarch64-virt.dtb}"
netdev_arg="user,id=net0"

if [[ -n "$hostfwd_host_port" && -n "$hostfwd_guest_port" ]]; then
  netdev_arg="${netdev_arg},hostfwd=udp::${hostfwd_host_port}-:${hostfwd_guest_port}"
fi

if [[ "$dtb_mode" != "off" ]]; then
  if [[ "$dtb_mode" != "auto" ]]; then
    dtb_path="$dtb_mode"
  elif [[ "$dry_run" -eq 0 ]]; then
    rm -f "$dtb_path"
    qemu-system-aarch64 \
      -machine "virt,dumpdtb=${dtb_path}" \
      -cpu cortex-a57 \
      -m 256M \
      -nographic \
      -monitor none \
      -serial none \
      -display none >/dev/null 2>&1 || true
  fi
  if [[ "$dry_run" -eq 0 && ! -f "$dtb_path" ]]; then
    echo "failed to prepare aarch64 dtb at $dtb_path" >&2
    exit 1
  fi
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

if [[ "$dtb_mode" != "off" && -f "$dtb_path" ]]; then
  cmd+=( -dtb "$dtb_path" )
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
