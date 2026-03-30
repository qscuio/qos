#!/usr/bin/env bash
set -euo pipefail

IFACE="rustnet0"

echo "[rust_netdev] expecting interface ${IFACE}"
ip link set "${IFACE}" up
ip addr show "${IFACE}"
ping -c 1 -I "${IFACE}" 127.0.0.1 || true
tcpdump -ni "${IFACE}" -c 1 || true
