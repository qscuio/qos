#!/usr/bin/env bash
set -euo pipefail

BRIDGE_ONE=${LINUX_LAB_BRIDGE_ONE:-br1}
BRIDGE_TWO=${LINUX_LAB_BRIDGE_TWO:-br2}
TAP_ONE=${LINUX_LAB_TAP_ONE:-tap1}
TAP_TWO=${LINUX_LAB_TAP_TWO:-tap2}

sudo ip link set "$BRIDGE_ONE" down || true
sudo ip link set "$BRIDGE_TWO" down || true
sudo ip link set "$TAP_ONE" down || true
sudo ip link set "$TAP_TWO" down || true
sudo tunctl -d "$TAP_ONE" || true
sudo tunctl -d "$TAP_TWO" || true
sudo brctl delbr "$BRIDGE_ONE" || true
sudo brctl delbr "$BRIDGE_TWO" || true
