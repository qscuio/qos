#!/usr/bin/env bash
set -euo pipefail

BRIDGE_ONE=${LINUX_LAB_BRIDGE_ONE:-br1}
BRIDGE_TWO=${LINUX_LAB_BRIDGE_TWO:-br2}
TAP_ONE=${LINUX_LAB_TAP_ONE:-tap1}
TAP_TWO=${LINUX_LAB_TAP_TWO:-tap2}

sudo brctl addbr "$BRIDGE_ONE"
sudo brctl addbr "$BRIDGE_TWO"
sudo brctl stp "$BRIDGE_ONE" off
sudo brctl stp "$BRIDGE_TWO" off
sudo tunctl -t "$TAP_ONE" -u root
sudo tunctl -t "$TAP_TWO" -u root
sudo brctl addif "$BRIDGE_ONE" "$TAP_ONE"
sudo brctl addif "$BRIDGE_TWO" "$TAP_TWO"
sudo ip address add 10.10.10.254/24 dev "$BRIDGE_ONE"
sudo ip address add 10.20.20.254/24 dev "$BRIDGE_TWO"
sudo ip link set "$BRIDGE_ONE" up
sudo ip link set "$BRIDGE_TWO" up
sudo ip link set "$TAP_ONE" up
sudo ip link set "$TAP_TWO" up
