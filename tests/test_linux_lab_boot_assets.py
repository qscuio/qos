from __future__ import annotations

import importlib.util
import os
import subprocess
import sys
from pathlib import Path
from types import ModuleType, SimpleNamespace


ROOT = Path(__file__).resolve().parents[1]
LINUX_LAB_ROOT = ROOT / "linux-lab"
SCRIPTS_DIR = LINUX_LAB_ROOT / "scripts"
BUILD_IMAGE_STAGE = LINUX_LAB_ROOT / "orchestrator" / "stages" / "build_image.py"
BOOT_STAGE = LINUX_LAB_ROOT / "orchestrator" / "stages" / "boot.py"
STATE_MODULE = LINUX_LAB_ROOT / "orchestrator" / "core" / "state.py"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
    if str(LINUX_LAB_ROOT) not in sys.path:
        sys.path.insert(0, str(LINUX_LAB_ROOT))
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _make_request(tmp_path: Path) -> object:
    return SimpleNamespace(
        kernel_version="6.18.4",
        arch="x86_64",
        image_release="noble",
        profiles=["default-lab"],
        mirror_region="sg",
        compat_mode=False,
        legacy_args={},
        request_fingerprint="liveboot123",
        artifact_root=tmp_path / "request",
        command="run",
        dry_run=False,
        stop_after=None,
    )


def test_runtime_scripts_are_ported() -> None:
    expected = [
        SCRIPTS_DIR / "create-image.sh",
        SCRIPTS_DIR / "boot.sh",
        SCRIPTS_DIR / "connect",
        SCRIPTS_DIR / "gdb",
        SCRIPTS_DIR / "up.sh",
        SCRIPTS_DIR / "down.sh",
    ]
    missing = [str(path) for path in expected if not path.is_file()]
    assert missing == [], f"missing runtime helper scripts: {missing}"
    not_executable = [str(path) for path in expected if not os.access(path, os.X_OK)]
    assert not_executable == [], f"non-executable helper scripts: {not_executable}"


def test_build_image_stage_executes_helper_script_in_mock_mode(tmp_path: Path, monkeypatch) -> None:
    build_image_mod = _load_module("linux_lab_build_image_live", BUILD_IMAGE_STAGE)
    state_mod = _load_module("linux_lab_state_image_live", STATE_MODULE)

    request = _make_request(tmp_path)
    request_root = Path(request.artifact_root)
    state_mod.ensure_request_dirs(request_root)
    monkeypatch.setenv("LINUX_LAB_IMAGE_MODE", "mock")

    result = build_image_mod.STAGE.executor(request, SimpleNamespace(), request_root)

    image_path = Path(request.artifact_root) / "images" / f"{request.image_release}.img"
    chroot_dir = Path(request.artifact_root) / "chroot"

    assert result["status"] == "succeeded"
    assert image_path.is_file()
    assert chroot_dir.is_dir()
    assert Path(result["metadata"]["script_path"]).name == "create-image.sh"
    assert "create-image.sh" in (request_root / "logs" / "build-image.log").read_text(encoding="utf-8")
    debugfs = subprocess.run(
        ["debugfs", "-R", "cat /linux-lab-image.env", str(image_path)],
        check=True,
        capture_output=True,
        text=True,
    )
    assert "mode=mock" in debugfs.stdout


def test_boot_stage_executes_helper_script_in_print_mode(tmp_path: Path, monkeypatch) -> None:
    boot_mod = _load_module("linux_lab_boot_live", BOOT_STAGE)
    state_mod = _load_module("linux_lab_state_boot_live", STATE_MODULE)

    request = _make_request(tmp_path)
    request_root = Path(request.artifact_root)
    state_mod.ensure_request_dirs(request_root)
    monkeypatch.setenv("LINUX_LAB_BOOT_MODE", "print")

    kernel_image = (
        Path(request.artifact_root)
        / "workspace"
        / "build"
        / "arch"
        / request.arch
        / "boot"
        / "bzImage"
    )
    kernel_image.parent.mkdir(parents=True, exist_ok=True)
    kernel_image.write_text("kernel\n", encoding="utf-8")
    image_path = Path(request.artifact_root) / "images" / f"{request.image_release}.img"
    image_path.parent.mkdir(parents=True, exist_ok=True)
    image_path.write_text("image\n", encoding="utf-8")

    manifests = SimpleNamespace(arches={"x86_64": SimpleNamespace(kernel_image_name="bzImage")})
    result = boot_mod.STAGE.executor(request, manifests, request_root)

    log_text = (request_root / "logs" / "boot.log").read_text(encoding="utf-8")

    assert result["status"] == "succeeded"
    assert Path(result["metadata"]["script_path"]).name == "boot.sh"
    assert "qemu-system-x86_64" in log_text
    assert "up.sh" in log_text
    assert "down.sh" in log_text


def test_boot_helper_fails_when_wrapped_command_exits_immediately(tmp_path: Path) -> None:
    log_path = tmp_path / "boot.log"
    pidfile = tmp_path / "qemu.pid"

    result = subprocess.run(
        [str(SCRIPTS_DIR / "boot.sh"), str(log_path), str(pidfile), "sh", "-c", "exit 42"],
        capture_output=True,
        text=True,
        env={**os.environ, "LINUX_LAB_BOOT_MODE": "exec", "LINUX_LAB_BOOT_STARTUP_WAIT": "0.1"},
    )

    assert result.returncode == 42
    assert not pidfile.exists()


def test_gdb_helper_defaults_to_latest_linux_lab_request_root(tmp_path: Path) -> None:
    workdir = tmp_path / "cwd"
    request_root = workdir / "build" / "linux-lab" / "requests" / "sample" / "workspace" / "build"
    request_root.mkdir(parents=True, exist_ok=True)
    (request_root / "vmlinux").write_text("elf\n", encoding="utf-8")

    fakebin = tmp_path / "bin"
    fakebin.mkdir()
    fake_gdb = fakebin / "gdb-multiarch"
    fake_gdb.write_text("#!/usr/bin/env bash\nprintf '%s\\n' \"$1\"\n", encoding="utf-8")
    fake_gdb.chmod(0o755)

    result = subprocess.run(
        [str(SCRIPTS_DIR / "gdb")],
        cwd=workdir,
        capture_output=True,
        text=True,
        env={**os.environ, "PATH": f"{fakebin}:{os.environ['PATH']}"},
    )

    assert result.returncode == 0, result.stderr
    assert result.stdout.strip().endswith("build/linux-lab/requests/sample/workspace/build/vmlinux")
