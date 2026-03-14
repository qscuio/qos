#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMG="${1:-$ROOT/build/linux1/images/linux1-disk.img}"
LOG_DIR="$ROOT/build/linux1/logs"
LOG_FILE="${LINUX1_BOOT_LOG:-$LOG_DIR/boot.log}"
TIMEOUT_SEC="${LINUX1_BOOT_TIMEOUT_SEC:-45}"
QEMU_STDOUT="$LOG_DIR/qemu-pty.log"
QEMU_STDERR="$LOG_DIR/qemu.stderr.log"
SERIAL_LOG="$LOG_DIR/serial.log"
SCREEN_PPM="$LOG_DIR/boot-screen.ppm"
SCREEN_PNG="$LOG_DIR/boot-screen.png"
OCR_PNG="$LOG_DIR/boot-screen-ocr.png"
OCR_FULL_PNG="$LOG_DIR/boot-screen-ocr-full.png"
MONITOR_SOCK="$LOG_DIR/qemu-monitor.sock"

fail() {
  local reason="$1"
  local code="${2:-1}"
  echo "ERROR:run-linux1-qemu:${reason}"
  exit "$code"
}

require_cmd() {
  local cmd="$1"
  command -v "$cmd" >/dev/null 2>&1 || fail "missing tool '$cmd'" 2
}

[[ -f "$IMG" ]] || fail "missing disk image at $IMG" 2
mkdir -p "$LOG_DIR"

require_cmd qemu-system-i386
require_cmd nc
require_cmd python3
require_cmd tesseract

rm -f "$LOG_FILE" "$QEMU_STDOUT" "$QEMU_STDERR" "$SERIAL_LOG" \
  "$SCREEN_PPM" "$SCREEN_PNG" "$OCR_PNG" "$OCR_FULL_PNG" "$MONITOR_SOCK"
touch "$QEMU_STDOUT" "$QEMU_STDERR"

qemu-system-i386 \
  -drive "file=${IMG},format=raw,if=ide" \
  -serial pty \
  -monitor "unix:${MONITOR_SOCK},server,nowait" \
  -display none \
  -no-reboot \
  -no-shutdown \
  -m 64M \
  >"$QEMU_STDOUT" 2>"$QEMU_STDERR" &
qemu_pid=$!

cleanup() {
  set +e
  if [[ -n "${serial_pid:-}" ]]; then
    kill "$serial_pid" >/dev/null 2>&1 || true
  fi
  kill "$qemu_pid" >/dev/null 2>&1 || true
}
trap cleanup EXIT

for _ in $(seq 1 200); do
  pty_path="$(
    sed -n 's#char device redirected to \([^ ]*\).*#\1#p' "$QEMU_STDOUT" 2>/dev/null | tail -n 1
  )"
  if [[ -n "${pty_path:-}" && -e "$pty_path" && -S "$MONITOR_SOCK" ]]; then
    break
  fi
  sleep 0.1
done

[[ -n "${pty_path:-}" && -e "$pty_path" ]] || fail "qemu did not expose a serial PTY"
[[ -S "$MONITOR_SOCK" ]] || fail "qemu monitor socket did not become ready"

set +e
timeout "${TIMEOUT_SEC}s" cat "$pty_path" >"$SERIAL_LOG" &
serial_pid=$!
set -e

sleep "$TIMEOUT_SEC"
printf 'screendump %s\nquit\n' "$SCREEN_PPM" | nc -U "$MONITOR_SOCK" >/dev/null 2>&1 || true

set +e
wait "$qemu_pid"
qemu_status=$?
wait "$serial_pid"
serial_status=$?
set -e

if [[ ! -f "$SCREEN_PPM" ]]; then
  fail "monitor screendump failed"
fi

python3 - "$SCREEN_PPM" "$SCREEN_PNG" "$OCR_PNG" "$OCR_FULL_PNG" "$SERIAL_LOG" "$LOG_FILE" <<'PY'
import pathlib
import re
import subprocess
import sys

from PIL import Image

ppm_path = pathlib.Path(sys.argv[1])
png_path = pathlib.Path(sys.argv[2])
ocr_png_path = pathlib.Path(sys.argv[3])
ocr_full_png_path = pathlib.Path(sys.argv[4])
serial_log_path = pathlib.Path(sys.argv[5])
log_path = pathlib.Path(sys.argv[6])

img = Image.open(ppm_path).convert("L")
img.save(png_path)

full_ocr_img = img.resize((img.size[0] * 2, img.size[1] * 2), Image.NEAREST)
full_ocr_img = full_ocr_img.point(lambda p: 255 if p > 128 else 0)
full_ocr_img.save(ocr_full_png_path)

# OCR is more reliable on an enlarged thresholded crop of the bottom rows.
w, h = img.size
ocr_img = img.crop((0, max(0, h - 120), w, h)).resize((w * 2, 240), Image.NEAREST)
ocr_img = ocr_img.point(lambda p: 255 if p > 128 else 0)
ocr_img.save(ocr_png_path)

ocr_full = subprocess.run(
    ["tesseract", str(ocr_full_png_path), "stdout", "--psm", "6"],
    stdout=subprocess.PIPE,
    stderr=subprocess.DEVNULL,
    text=True,
    check=True,
).stdout
ocr_bottom = subprocess.run(
    ["tesseract", str(ocr_png_path), "stdout", "--psm", "6"],
    stdout=subprocess.PIPE,
    stderr=subprocess.DEVNULL,
    text=True,
    check=True,
).stdout

serial = serial_log_path.read_text(errors="replace") if serial_log_path.exists() else ""
ocr = f"{ocr_full}\n{ocr_bottom}"
ocr_lower = ocr.lower()

markers = []
if "lilo" in serial.lower():
    markers.append("LILO")
if re.search(r"linux version 1\.0", ocr_lower):
    markers.append("Linux version 1.0.0")
if re.search(r"linux[1il][-_ ]init[:;]?\s*start", ocr_lower):
    markers.append("linux1-init: start")
if re.search(r"linux[1il][-_ ]sh", ocr_lower):
    markers.append("linux1-sh# ")

parts = []
if serial.strip():
    parts.append(serial.strip())
if markers:
    parts.extend(markers)
parts.append("--- OCR ---")
parts.append(ocr.strip())
log_path.write_text("\n".join(parts).strip() + "\n")

required = [
    "LILO",
    "Linux version 1.0.0",
    "linux1-init: start",
    "linux1-sh# ",
]
missing = [marker for marker in required if marker not in markers]
if missing:
    print("\n".join(missing))
    raise SystemExit(3)
PY
ocr_status=$?

trap - EXIT

if [[ "$qemu_status" -ne 0 && "$qemu_status" -ne 124 && "$qemu_status" -ne 143 ]]; then
  fail "qemu exited with status ${qemu_status}"
fi
if [[ "$serial_status" -ne 0 && "$serial_status" -ne 1 && "$serial_status" -ne 124 && "$serial_status" -ne 143 ]]; then
  fail "serial capture exited with status ${serial_status}"
fi
if [[ "$ocr_status" -ne 0 ]]; then
  sed -n '1,200p' "$LOG_FILE" >&2 || true
  fail "boot marker check failed"
fi

echo "OK:run-linux1-qemu"
