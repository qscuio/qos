from __future__ import annotations

import os
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPTS_DIR = ROOT / "linux-lab" / "scripts"


def test_runtime_and_network_script_surface_is_ported() -> None:
    expected = [
        SCRIPTS_DIR / "runtime" / "connect",
        SCRIPTS_DIR / "runtime" / "guest_run",
        SCRIPTS_DIR / "runtime" / "guest_insmod",
        SCRIPTS_DIR / "runtime" / "guest_rmmods",
        SCRIPTS_DIR / "runtime" / "monitor",
        SCRIPTS_DIR / "runtime" / "reboot",
        SCRIPTS_DIR / "runtime" / "qmp",
        SCRIPTS_DIR / "network" / "up.sh",
        SCRIPTS_DIR / "network" / "down.sh",
        SCRIPTS_DIR / "network" / "tap_and_veth_set.sh",
        SCRIPTS_DIR / "network" / "tap_and_veth_unset.sh",
        SCRIPTS_DIR / "guest_run",
        SCRIPTS_DIR / "guest_insmod",
        SCRIPTS_DIR / "guest_rmmods",
        SCRIPTS_DIR / "monitor",
        SCRIPTS_DIR / "reboot",
        SCRIPTS_DIR / "qmp",
    ]
    missing = [str(path) for path in expected if not path.is_file()]
    assert missing == [], f"missing parity helper scripts: {missing}"
    not_executable = [str(path) for path in expected if not os.access(path, os.X_OK)]
    assert not_executable == [], f"non-executable parity helper scripts: {not_executable}"


def test_top_level_runtime_wrappers_delegate_to_structured_scripts() -> None:
    wrappers = {
        "connect": "runtime/connect",
        "guest_run": "runtime/guest_run",
        "guest_insmod": "runtime/guest_insmod",
        "guest_rmmods": "runtime/guest_rmmods",
        "monitor": "runtime/monitor",
        "reboot": "runtime/reboot",
        "qmp": "runtime/qmp",
    }
    for name, target in wrappers.items():
        text = (SCRIPTS_DIR / name).read_text(encoding="utf-8")
        assert target in text, f"{name} does not delegate to {target}"


def test_runtime_connect_respects_environment_overrides(tmp_path: Path) -> None:
    fakebin = tmp_path / "bin"
    fakebin.mkdir()
    ssh = fakebin / "ssh"
    ssh.write_text(
        "#!/usr/bin/env bash\nprintf '%s\\n' \"$*\"\n",
        encoding="utf-8",
    )
    ssh.chmod(0o755)

    result = subprocess.run(
        [str(SCRIPTS_DIR / "runtime" / "connect"), "uname", "-a"],
        capture_output=True,
        text=True,
        env={
            **os.environ,
            "PATH": f"{fakebin}:{os.environ['PATH']}",
            "LINUX_LAB_GUEST_HOST": "192.0.2.10",
            "LINUX_LAB_GUEST_USER": "lab",
            "LINUX_LAB_SSH_KEY": "/tmp/test.key",
        },
    )

    assert result.returncode == 0, result.stderr
    assert "-i /tmp/test.key lab@192.0.2.10 -o StrictHostKeyChecking=no uname -a" in result.stdout.strip()


def test_runtime_qmp_respects_environment_overrides(tmp_path: Path) -> None:
    fakebin = tmp_path / "bin"
    fakebin.mkdir()
    nc = fakebin / "nc"
    nc.write_text(
        "#!/usr/bin/env bash\ncat > \"$LINUX_LAB_QMP_CAPTURE\"\nprintf '%s\\n' \"$*\" >> \"$LINUX_LAB_QMP_CAPTURE\"\n",
        encoding="utf-8",
    )
    nc.chmod(0o755)
    capture = tmp_path / "qmp.txt"

    result = subprocess.run(
        [str(SCRIPTS_DIR / "runtime" / "qmp"), '{"execute":"query-status"}'],
        capture_output=True,
        text=True,
        env={
            **os.environ,
            "PATH": f"{fakebin}:{os.environ['PATH']}",
            "LINUX_LAB_QMP_HOST": "127.0.0.2",
            "LINUX_LAB_QMP_PORT": "24567",
            "LINUX_LAB_QMP_CAPTURE": str(capture),
        },
    )

    assert result.returncode == 0, result.stderr
    text = capture.read_text(encoding="utf-8")
    assert '{"execute":"qmp_capabilities"}' in text
    assert '{"execute":"query-status"}' in text
    assert "127.0.0.2 24567" in text


def test_runtime_monitor_respects_environment_overrides(tmp_path: Path) -> None:
    fakebin = tmp_path / "bin"
    fakebin.mkdir()
    telnet = fakebin / "telnet"
    telnet.write_text(
        "#!/usr/bin/env bash\nprintf '%s\\n' \"$*\"\n",
        encoding="utf-8",
    )
    telnet.chmod(0o755)

    result = subprocess.run(
        [str(SCRIPTS_DIR / "runtime" / "monitor")],
        capture_output=True,
        text=True,
        env={
            **os.environ,
            "PATH": f"{fakebin}:{os.environ['PATH']}",
            "LINUX_LAB_MONITOR_HOST": "127.0.0.3",
            "LINUX_LAB_MONITOR_PORT": "32345",
        },
    )

    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == "127.0.0.3 32345"
