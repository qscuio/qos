#!/usr/bin/env bash
set -euo pipefail

BRIDGE_ONE=${LINUX_LAB_BRIDGE_ONE:-br1}
BRIDGE_TWO=${LINUX_LAB_BRIDGE_TWO:-br2}
TAP_ONE=${LINUX_LAB_TAP_ONE:-tap1}
TAP_TWO=${LINUX_LAB_TAP_TWO:-tap2}
TUNCTL_BIN=${LINUX_LAB_TUNCTL_BIN:-tunctl}
BRCTL_BIN=${LINUX_LAB_BRCTL_BIN:-brctl}
IP_BIN=${LINUX_LAB_IP_BIN:-ip}

sudo "$IP_BIN" link set "$BRIDGE_ONE" down || true
sudo "$IP_BIN" link set "$BRIDGE_TWO" down || true
sudo "$IP_BIN" link set "$TAP_ONE" down || true
sudo "$IP_BIN" link set "$TAP_TWO" down || true
sudo "$TUNCTL_BIN" -d "$TAP_ONE" || true
sudo "$TUNCTL_BIN" -d "$TAP_TWO" || true
sudo "$BRCTL_BIN" delbr "$BRIDGE_ONE" || true
sudo "$BRCTL_BIN" delbr "$BRIDGE_TWO" || true
