from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=(cwd or ROOT),
        text=True,
        capture_output=True,
    )


def _build_c_app(name: str) -> Path:
    src = ROOT / "c-os/userspace" / f"{name}.c"
    out = ROOT / "c-os/build" / f"{name}"
    out.parent.mkdir(parents=True, exist_ok=True)
    build = _run(["gcc", "-std=c11", "-O2", "-o", str(out), str(src)], cwd=ROOT)
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}"
    return out


def _run_c_app(name: str, *app_args: str) -> str:
    app = _build_c_app(name)
    result = _run([str(app), *app_args], cwd=ROOT)
    assert result.returncode == 0, f"app failed:\n{result.stderr}\nstdout:\n{result.stdout}"
    return result.stdout


def _run_net_app(bin_name: str, *app_args: str) -> str:
    result = _run(
        [
            "cargo",
            "run",
            "--quiet",
            "-p",
            "qos-userspace",
            "--bin",
            bin_name,
            "--",
            *app_args,
        ],
        cwd=ROOT / "rust-os",
    )
    assert result.returncode == 0, f"cargo run failed:\n{result.stderr}\nstdout:\n{result.stdout}"
    return result.stdout


def test_rust_qos_ping_binary_reachable_and_unreachable() -> None:
    ok = _run_net_app("qos-ping", "10.0.2.2")
    assert "64 bytes from 10.0.2.2" in ok

    bad = _run_net_app("qos-ping", "10.0.2.99")
    assert "Destination Host Unreachable" in bad


def test_rust_qos_wget_binary_fetches_gateway_http() -> None:
    out = _run_net_app("qos-wget", "http://10.0.2.2:8080/")
    assert "HTTP/1.0 200 OK" in out
    assert "qos" in out


def test_c_ping_binary_reachable_and_unreachable() -> None:
    ok = _run_c_app("ping", "10.0.2.2")
    assert "64 bytes from 10.0.2.2" in ok

    bad = _run_c_app("ping", "10.0.2.99")
    assert "Destination Host Unreachable" in bad


def test_c_wget_binary_fetches_gateway_http() -> None:
    out = _run_c_app("wget", "http://10.0.2.2:8080/")
    assert "HTTP/1.0 200 OK" in out
    assert "qos" in out
