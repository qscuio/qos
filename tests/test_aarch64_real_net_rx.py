from __future__ import annotations

import os
import socket
import subprocess
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REAL_RX_GUEST_PORT = 40002
REAL_RX_PAYLOAD = b"QOSREALRX"


def _spam_udp(host_port: int, stop: threading.Event) -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        deadline = time.time() + 5.0
        while time.time() < deadline and not stop.is_set():
            sock.sendto(REAL_RX_PAYLOAD, ("127.0.0.1", host_port))
            time.sleep(0.05)
    finally:
        sock.close()


def _run_smoke_with_rx_probe(
    impl: str, host_port: int, pcap: Path
) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env["QOS_HOSTFWD_UDP_HOST_PORT"] = str(host_port)
    env["QOS_HOSTFWD_UDP_GUEST_PORT"] = str(REAL_RX_GUEST_PORT)
    env["QOS_PCAP_PATH"] = str(pcap)
    cmd = ["make", "-C", f"{impl}-os", "ARCH=aarch64", "smoke"]
    return subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True, env=env)


def test_rust_aarch64_smoke_emits_real_virtio_rx_frame() -> None:
    host_port = 41000
    pcap = ROOT / "rust-os/build/aarch64/virtio-net-rx-smoke.pcap"
    pcap.parent.mkdir(parents=True, exist_ok=True)
    if pcap.exists():
        pcap.unlink()

    stop = threading.Event()
    sender = threading.Thread(target=_spam_udp, args=(host_port, stop), daemon=True)
    sender.start()
    try:
        result = _run_smoke_with_rx_probe("rust", host_port, pcap)
    finally:
        stop.set()
        sender.join(timeout=1.0)

    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    log = ROOT / "rust-os/build/aarch64/smoke.log"
    text = log.read_text(encoding="utf-8", errors="replace")
    assert "net_rx=real_ok" in text, f"missing real net rx marker in {log}\n{text}"

    assert pcap.is_file(), f"expected pcap capture at {pcap}"
    raw = pcap.read_bytes()
    assert len(raw) > 64, f"expected captured traffic in pcap {pcap}"


def test_c_aarch64_smoke_emits_real_virtio_rx_frame() -> None:
    host_port = 41001
    pcap = ROOT / "c-os/build/aarch64/virtio-net-rx-smoke.pcap"
    pcap.parent.mkdir(parents=True, exist_ok=True)
    if pcap.exists():
        pcap.unlink()

    stop = threading.Event()
    sender = threading.Thread(target=_spam_udp, args=(host_port, stop), daemon=True)
    sender.start()
    try:
        result = _run_smoke_with_rx_probe("c", host_port, pcap)
    finally:
        stop.set()
        sender.join(timeout=1.0)

    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    log = ROOT / "c-os/build/aarch64/smoke.log"
    text = log.read_text(encoding="utf-8", errors="replace")
    assert "net_rx=real_ok" in text, f"missing real net rx marker in {log}\n{text}"

    assert pcap.is_file(), f"expected pcap capture at {pcap}"
    raw = pcap.read_bytes()
    assert len(raw) > 64, f"expected captured traffic in pcap {pcap}"
