#!/usr/bin/env bash
set -euo pipefail

TAP_ONE=${LINUX_LAB_TAP_VETH_TAP_ONE:-tap110}
TAP_TWO=${LINUX_LAB_TAP_VETH_TAP_TWO:-tap111}
VETH_ONE=${LINUX_LAB_TAP_VETH_ONE:-veth110}
VETH_TWO=${LINUX_LAB_TAP_VETH_TWO:-veth111}
BRIDGE_ONE=${LINUX_LAB_TAP_VETH_BRIDGE_ONE:-br110}
BRIDGE_TWO=${LINUX_LAB_TAP_VETH_BRIDGE_TWO:-br111}
IP_BIN=${LINUX_LAB_IP_BIN:-ip}
BRCTL_BIN=${LINUX_LAB_BRCTL_BIN:-brctl}

sudo "$IP_BIN" link set "$BRIDGE_ONE" down || true
sudo "$IP_BIN" link set "$BRIDGE_TWO" down || true
sudo "$BRCTL_BIN" delbr "$BRIDGE_ONE" || true
sudo "$BRCTL_BIN" delbr "$BRIDGE_TWO" || true
sudo "$IP_BIN" link set "$TAP_ONE" down || true
sudo "$IP_BIN" link set "$TAP_TWO" down || true
sudo "$IP_BIN" tuntap del dev "$TAP_ONE" mode tap || true
sudo "$IP_BIN" tuntap del dev "$TAP_TWO" mode tap || true
sudo "$IP_BIN" link set "$VETH_ONE" down || true
sudo "$IP_BIN" link set "$VETH_TWO" down || true
sudo "$IP_BIN" link delete "$VETH_ONE" || true
