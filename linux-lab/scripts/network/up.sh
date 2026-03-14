#!/usr/bin/env bash
set -euo pipefail

BRIDGE_ONE=${LINUX_LAB_BRIDGE_ONE:-br1}
BRIDGE_TWO=${LINUX_LAB_BRIDGE_TWO:-br2}
TAP_ONE=${LINUX_LAB_TAP_ONE:-tap1}
TAP_TWO=${LINUX_LAB_TAP_TWO:-tap2}
BRCTL_BIN=${LINUX_LAB_BRCTL_BIN:-brctl}
TUNCTL_BIN=${LINUX_LAB_TUNCTL_BIN:-tunctl}
IP_BIN=${LINUX_LAB_IP_BIN:-ip}

sudo "$BRCTL_BIN" addbr "$BRIDGE_ONE"
sudo "$BRCTL_BIN" addbr "$BRIDGE_TWO"
sudo "$BRCTL_BIN" stp "$BRIDGE_ONE" off
sudo "$BRCTL_BIN" stp "$BRIDGE_TWO" off
sudo "$TUNCTL_BIN" -t "$TAP_ONE" -u root
sudo "$TUNCTL_BIN" -t "$TAP_TWO" -u root
sudo "$BRCTL_BIN" addif "$BRIDGE_ONE" "$TAP_ONE"
sudo "$BRCTL_BIN" addif "$BRIDGE_TWO" "$TAP_TWO"
sudo "$IP_BIN" address add 10.10.10.254/24 dev "$BRIDGE_ONE"
sudo "$IP_BIN" address add 10.20.20.254/24 dev "$BRIDGE_TWO"
sudo "$IP_BIN" link set "$BRIDGE_ONE" up
sudo "$IP_BIN" link set "$BRIDGE_TWO" up
sudo "$IP_BIN" link set "$TAP_ONE" up
sudo "$IP_BIN" link set "$TAP_TWO" up
