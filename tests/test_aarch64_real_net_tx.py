from __future__ import annotations

import os
import socket
import subprocess
import threading
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REAL_UDP_PORT = 40000


def test_rust_aarch64_smoke_emits_real_virtio_tx_frame() -> None:
    received: list[bytes] = []
    ready = threading.Event()

    def _udp_listener() -> None:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sock.bind(("127.0.0.1", REAL_UDP_PORT))
            sock.settimeout(5.0)
            ready.set()
            data, _ = sock.recvfrom(4096)
            received.append(data)
        except TimeoutError:
            pass
        finally:
            sock.close()

    listener = threading.Thread(target=_udp_listener, daemon=True)
    listener.start()
    assert ready.wait(timeout=2.0), "udp listener did not start"

    pcap = ROOT / "rust-os/build/aarch64/virtio-net-smoke.pcap"
    pcap.parent.mkdir(parents=True, exist_ok=True)
    if pcap.exists():
        pcap.unlink()

    env = os.environ.copy()
    env["QOS_PCAP_PATH"] = str(pcap)
    result = subprocess.run(
        ["make", "-C", "rust-os", "ARCH=aarch64", "smoke"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        env=env,
    )
    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    log = ROOT / "rust-os/build/aarch64/smoke.log"
    text = log.read_text(encoding="utf-8", errors="replace")
    assert "net_tx=real_ok" in text, f"missing real net tx marker in {log}\n{text}"

    assert pcap.is_file(), f"expected pcap capture at {pcap}"
    raw = pcap.read_bytes()
    assert b"QOSREALNET" in raw, f"expected probe payload in pcap {pcap}"

    listener.join(timeout=6.0)
    assert received, "expected host UDP listener to receive guest payload"
    assert received[0] == b"QOSREALNET", f"unexpected guest payload: {received[0]!r}"


def test_c_aarch64_smoke_emits_real_virtio_tx_frame() -> None:
    received: list[bytes] = []
    ready = threading.Event()

    def _udp_listener() -> None:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sock.bind(("127.0.0.1", REAL_UDP_PORT))
            sock.settimeout(5.0)
            ready.set()
            data, _ = sock.recvfrom(4096)
            received.append(data)
        except TimeoutError:
            pass
        finally:
            sock.close()

    listener = threading.Thread(target=_udp_listener, daemon=True)
    listener.start()
    assert ready.wait(timeout=2.0), "udp listener did not start"

    pcap = ROOT / "c-os/build/aarch64/virtio-net-smoke.pcap"
    pcap.parent.mkdir(parents=True, exist_ok=True)
    if pcap.exists():
        pcap.unlink()

    env = os.environ.copy()
    env["QOS_PCAP_PATH"] = str(pcap)
    result = subprocess.run(
        ["make", "-C", "c-os", "ARCH=aarch64", "smoke"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        env=env,
    )
    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    log = ROOT / "c-os/build/aarch64/smoke.log"
    text = log.read_text(encoding="utf-8", errors="replace")
    assert "net_tx=real_ok" in text, f"missing real net tx marker in {log}\n{text}"

    assert pcap.is_file(), f"expected pcap capture at {pcap}"
    raw = pcap.read_bytes()
    assert b"QOSREALNET" in raw, f"expected probe payload in pcap {pcap}"

    listener.join(timeout=6.0)
    assert received, "expected host UDP listener to receive guest payload"
    assert received[0] == b"QOSREALNET", f"unexpected guest payload: {received[0]!r}"
