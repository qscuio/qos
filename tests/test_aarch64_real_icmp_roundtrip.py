from __future__ import annotations

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ICMP_PROBE_PAYLOAD = b"QOSICMPRT"


def _run_smoke(impl: str, pcap: Path) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env["QOS_PCAP_PATH"] = str(pcap)
    return subprocess.run(
        ["make", "-C", f"{impl}-os", "ARCH=aarch64", "smoke"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        env=env,
    )


def _assert_roundtrip_ok(impl: str) -> None:
    pcap = ROOT / f"{impl}-os/build/aarch64/virtio-icmp-roundtrip.pcap"
    pcap.parent.mkdir(parents=True, exist_ok=True)
    if pcap.exists():
        pcap.unlink()

    result = _run_smoke(impl, pcap)
    assert result.returncode == 0, f"stderr:\n{result.stderr}\nstdout:\n{result.stdout}"

    log = ROOT / f"{impl}-os/build/aarch64/smoke.log"
    text = log.read_text(encoding="utf-8", errors="replace")
    assert (
        "icmp_real=roundtrip_ok" in text or "icmp_real=roundtrip_skip" in text
    ), f"missing roundtrip marker in {log}\n{text}"

    assert pcap.is_file(), f"expected pcap capture at {pcap}"
    raw = pcap.read_bytes()
    assert ICMP_PROBE_PAYLOAD in raw, f"expected ICMP probe payload in pcap {pcap}"


def test_rust_aarch64_real_icmp_roundtrip() -> None:
    _assert_roundtrip_ok("rust")


def test_c_aarch64_real_icmp_roundtrip() -> None:
    _assert_roundtrip_ok("c")
