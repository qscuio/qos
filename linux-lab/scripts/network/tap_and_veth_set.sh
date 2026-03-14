#!/usr/bin/env bash
set -euo pipefail

TAP_ONE=${LINUX_LAB_TAP_VETH_TAP_ONE:-tap110}
TAP_TWO=${LINUX_LAB_TAP_VETH_TAP_TWO:-tap111}
VETH_ONE=${LINUX_LAB_TAP_VETH_ONE:-veth110}
VETH_TWO=${LINUX_LAB_TAP_VETH_TWO:-veth111}
BRIDGE_ONE=${LINUX_LAB_TAP_VETH_BRIDGE_ONE:-br110}
BRIDGE_TWO=${LINUX_LAB_TAP_VETH_BRIDGE_TWO:-br111}
TAP_ONE_ADDR=${LINUX_LAB_TAP_VETH_TAP_ONE_ADDR:-192.168.100.1/24}
TAP_TWO_ADDR=${LINUX_LAB_TAP_VETH_TAP_TWO_ADDR:-192.168.100.2/24}
VETH_ONE_ADDR=${LINUX_LAB_TAP_VETH_ONE_ADDR:-192.168.100.3/24}
VETH_TWO_ADDR=${LINUX_LAB_TAP_VETH_TWO_ADDR:-192.168.100.4/24}
IP_BIN=${LINUX_LAB_IP_BIN:-ip}
BRCTL_BIN=${LINUX_LAB_BRCTL_BIN:-brctl}

sudo "$IP_BIN" tuntap add dev "$TAP_ONE" mode tap
sudo "$IP_BIN" tuntap add dev "$TAP_TWO" mode tap
sudo "$IP_BIN" link set "$TAP_ONE" up
sudo "$IP_BIN" link set "$TAP_TWO" up
sudo "$IP_BIN" link add "$VETH_ONE" type veth peer name "$VETH_TWO"
sudo "$IP_BIN" link set "$VETH_ONE" up
sudo "$IP_BIN" link set "$VETH_TWO" up
sudo "$BRCTL_BIN" addbr "$BRIDGE_ONE"
sudo "$BRCTL_BIN" addbr "$BRIDGE_TWO"
sudo "$BRCTL_BIN" stp "$BRIDGE_ONE" off
sudo "$BRCTL_BIN" stp "$BRIDGE_TWO" off
sudo "$BRCTL_BIN" addif "$BRIDGE_ONE" "$TAP_ONE"
sudo "$BRCTL_BIN" addif "$BRIDGE_ONE" "$VETH_ONE"
sudo "$BRCTL_BIN" addif "$BRIDGE_TWO" "$TAP_TWO"
sudo "$BRCTL_BIN" addif "$BRIDGE_TWO" "$VETH_TWO"
sudo "$IP_BIN" link set "$BRIDGE_ONE" up
sudo "$IP_BIN" link set "$BRIDGE_TWO" up
sudo "$IP_BIN" addr add "$TAP_ONE_ADDR" dev "$TAP_ONE"
sudo "$IP_BIN" addr add "$TAP_TWO_ADDR" dev "$TAP_TWO"
sudo "$IP_BIN" addr add "$VETH_ONE_ADDR" dev "$VETH_ONE"
sudo "$IP_BIN" addr add "$VETH_TWO_ADDR" dev "$VETH_TWO"
