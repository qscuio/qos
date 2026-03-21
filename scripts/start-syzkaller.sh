#!/usr/bin/env bash
# start-syzkaller.sh - Launch syzkaller fuzzer against a running linux-lab arm64 VM.
#
# Prerequisites:
#   1. linux-lab network must be up:   linux-lab/scripts/up.sh
#   2. A fuzzing kernel must be built:
#        python3 -m orchestrator --kernel 6.18.4 --arch arm64 --image noble \
#            --profile debug --profile fuzzing run
#   3. The VM must be booted and reachable at 10.10.10.1.
#   4. syzkaller must be built:  cd build/syzkaller/src && make TARGETOS=linux TARGETARCH=arm64
#
# Usage: ./scripts/start-syzkaller.sh [--dry-run]
set -euo pipefail

QOS_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$QOS_ROOT"

GUEST_HOST=${LINUX_LAB_GUEST_HOST:-10.10.10.1}
GUEST_USER=${LINUX_LAB_GUEST_USER:-qwert}
SSH_KEY_DIR="$QOS_ROOT/syzkaller/ssh"
SSH_KEY="$SSH_KEY_DIR/id_rsa"
SYZ_SRC="$QOS_ROOT/build/syzkaller/src"
SYZ_MANAGER="$SYZ_SRC/bin/syz-manager"
SYZ_CFG="$QOS_ROOT/build/syzkaller/arm64.cfg"
WORKDIR="$QOS_ROOT/build/syzkaller/workdir"
KERNEL_OBJ_LINK="$QOS_ROOT/build/syzkaller/kernel_obj"
DRY_RUN=false

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        *) echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

# --- 1. Check syzkaller is built ---
if [ ! -x "$SYZ_MANAGER" ]; then
    echo "ERROR: syz-manager not found at $SYZ_MANAGER" >&2
    echo "Build syzkaller first:" >&2
    echo "  cd build/syzkaller/src && make TARGETOS=linux TARGETARCH=arm64" >&2
    exit 1
fi

# --- 2. Generate SSH keypair if missing ---
mkdir -p "$SSH_KEY_DIR"
if [ ! -f "$SSH_KEY" ]; then
    echo "Generating syzkaller SSH key pair..."
    ssh-keygen -t rsa -b 4096 -f "$SSH_KEY" -N '' -C 'syzkaller@linux-lab'
    chmod 600 "$SSH_KEY"
    echo ""
    echo "IMPORTANT: Add the public key to the guest's authorized_keys:"
    echo "  ssh ${GUEST_USER}@${GUEST_HOST} 'mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys' < ${SSH_KEY}.pub"
    echo ""
    echo "Then re-run this script."
    exit 0
fi
chmod 600 "$SSH_KEY"

# --- 3. Verify SSH connectivity ---
echo "Checking SSH connectivity to ${GUEST_USER}@${GUEST_HOST}..."
if ! ssh -i "$SSH_KEY" -o StrictHostKeyChecking=no -o ConnectTimeout=5 \
        "${GUEST_USER}@${GUEST_HOST}" 'uname -r' >/dev/null 2>&1; then
    echo "ERROR: Cannot SSH to ${GUEST_USER}@${GUEST_HOST}" >&2
    echo "Ensure the linux-lab VM is running and the SSH key is installed." >&2
    echo "  Add key:  ssh ${GUEST_USER}@${GUEST_HOST} 'cat >> ~/.ssh/authorized_keys' < ${SSH_KEY}.pub" >&2
    exit 1
fi
KERNEL_RELEASE=$(ssh -i "$SSH_KEY" -o StrictHostKeyChecking=no \
    "${GUEST_USER}@${GUEST_HOST}" 'uname -r' 2>/dev/null)
echo "Guest kernel: $KERNEL_RELEASE"

# --- 4. Resolve kernel_obj symlink ---
# Match the running guest's kernel release to the correct build tree by reading
# include/config/kernel.release from each request directory, rather than using
# modification timestamps (which would point at the wrong tree if multiple
# arm64 kernels have been built).
MATCHED_KERNEL_OBJ=""
REQUESTS_DIR="$QOS_ROOT/build/linux-lab/requests"
if [ -d "$REQUESTS_DIR" ]; then
    while IFS= read -r release_file; do
        build_dir=$(dirname "$release_file")
        release=$(cat "$release_file" 2>/dev/null)
        if [ "$release" = "$KERNEL_RELEASE" ]; then
            MATCHED_KERNEL_OBJ="$build_dir"
            break
        fi
    done < <(find "$REQUESTS_DIR" -maxdepth 4 -name "kernel.release" \
                  -path "*/include/config/kernel.release" 2>/dev/null)
fi

if [ -n "$MATCHED_KERNEL_OBJ" ] && [ -d "$MATCHED_KERNEL_OBJ" ]; then
    echo "Using kernel_obj: $MATCHED_KERNEL_OBJ (matches uname -r: $KERNEL_RELEASE)"
    ln -sfn "$MATCHED_KERNEL_OBJ" "$KERNEL_OBJ_LINK"
else
    echo "WARNING: No build tree found for kernel release '$KERNEL_RELEASE' under $REQUESTS_DIR" >&2
    echo "Crash symbolization will be unavailable." >&2
    echo "Build first: python3 -m orchestrator --kernel 6.18.4 --arch arm64 --image noble --profile debug --profile fuzzing run" >&2
    mkdir -p "$KERNEL_OBJ_LINK"
fi

# --- 5. Prepare workdir ---
mkdir -p "$WORKDIR"

# --- 6. Generate syz-manager config from current variables ---
cat > "$SYZ_CFG" <<CFGEOF
{
    "name": "linux-lab-arm64",
    "target": "linux/arm64",
    "http": "0.0.0.0:56741",
    "workdir": "$WORKDIR",
    "syzkaller": "$SYZ_SRC",
    "kernel_obj": "$KERNEL_OBJ_LINK",
    "sshkey": "$SSH_KEY",
    "ssh_user": "$GUEST_USER",
    "sandbox": "none",
    "procs": 4,
    "type": "isolated",
    "vm": {
        "targets": ["$GUEST_HOST"],
        "target_dir": "/tmp/syzkaller-fuzzer",
        "target_reboot": false
    }
}
CFGEOF

# --- 7. Launch syz-manager ---
echo ""
echo "Starting syz-manager..."
echo "  Config:  $SYZ_CFG"
echo "  Workdir: $WORKDIR"
echo "  Web UI:  http://localhost:56741"
echo ""

if [ "$DRY_RUN" = true ]; then
    echo "[dry-run] Would execute:"
    echo "  $SYZ_MANAGER -config=$SYZ_CFG"
    exit 0
fi

exec "$SYZ_MANAGER" -config="$SYZ_CFG"
