#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 3 ]; then
    echo "Usage: $0 LOG_PATH PIDFILE QEMU_COMMAND..." >&2
    exit 1
fi

LOG_PATH=$1
PIDFILE=$2
shift 2

MODE=${LINUX_LAB_BOOT_MODE:-exec}
STARTUP_WAIT=${LINUX_LAB_BOOT_STARTUP_WAIT:-1}

mkdir -p "$(dirname "$LOG_PATH")" "$(dirname "$PIDFILE")"

if [ "$MODE" = "print" ]; then
    printf '%s\n' "$*"
    exit 0
fi

if ! command -v "$1" >/dev/null 2>&1; then
    echo "missing command: $1" >&2
    exit 127
fi

"$@" >>"$LOG_PATH" 2>&1 &
BOOT_PID=$!
sleep "$STARTUP_WAIT"
if ! kill -0 "$BOOT_PID" 2>/dev/null; then
    wait "$BOOT_PID"
    exit $?
fi
echo "$BOOT_PID" >"$PIDFILE"
