#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 && $# -ne 5 ]]; then
  echo "usage: $0 <c|rust> <x86_64|aarch64> <artifact-path> <log-file> [initrd-path]" >&2
  exit 2
fi

impl="$1"
arch="$2"
artifact="$3"
log_file="$4"
initrd_path="${5:-}"

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
timeout_sec="${SMOKE_TIMEOUT_SEC:-3}"
lock_retry_count="${SMOKE_LOCK_RETRY_COUNT:-5}"
lock_retry_sleep_sec="${SMOKE_LOCK_RETRY_SLEEP_SEC:-1}"

mkdir -p "$(dirname "$log_file")"

disk_path="$root_dir/qemu/configs/${impl}-${arch}.disk.img"
mkdir -p "$(dirname "$disk_path")"
if [[ ! -f "$disk_path" ]]; then
  truncate -s 64M "$disk_path"
fi

case "$arch" in
  x86_64)
    qemu_cmd=("$root_dir/qemu/x86_64.sh" "$artifact")
    ;;
  aarch64)
    if [[ -n "$initrd_path" ]]; then
      qemu_cmd=("$root_dir/qemu/aarch64.sh" "$disk_path" "$artifact" "$initrd_path")
    else
      qemu_cmd=("$root_dir/qemu/aarch64.sh" "$disk_path" "$artifact")
    fi
    ;;
  *)
    echo "unsupported arch: $arch" >&2
    exit 2
    ;;
esac

status=1
attempt=1
while [[ "$attempt" -le "$lock_retry_count" ]]; do
  set +e
  timeout "${timeout_sec}s" "${qemu_cmd[@]}" > "$log_file" 2>&1
  status=$?
  set -e

  if [[ "$status" -eq 0 || "$status" -eq 124 ]]; then
    break
  fi

  if grep -q 'Failed to get "write" lock' "$log_file"; then
    if [[ "$attempt" -lt "$lock_retry_count" ]]; then
      sleep "$lock_retry_sleep_sec"
      attempt=$((attempt + 1))
      continue
    fi
  fi
  break
done

if [[ "$status" -ne 0 && "$status" -ne 124 ]]; then
  echo "SMOKE FAIL: ${impl}/${arch} (qemu exit=$status)" >&2
  sed -n '1,120p' "$log_file" >&2 || true
  exit 1
fi

marker="QOS kernel started impl=${impl} arch=${arch}"
if ! grep -q "$marker" "$log_file"; then
  echo "SMOKE FAIL: ${impl}/${arch} (missing marker)" >&2
  sed -n '1,120p' "$log_file" >&2 || true
  exit 1
fi

echo "SMOKE PASS: ${impl}/${arch}"
