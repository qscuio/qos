from __future__ import annotations

import hashlib
import importlib.util
import io
import sys
import tarfile
import threading
import time
from pathlib import Path
from types import ModuleType, SimpleNamespace


ROOT = Path(__file__).resolve().parents[1]
LINUX_LAB_ROOT = ROOT / "linux-lab"
FETCH_STAGE = ROOT / "linux-lab" / "orchestrator" / "stages" / "fetch.py"
PATCH_STAGE = ROOT / "linux-lab" / "orchestrator" / "stages" / "patch.py"
CONFIG_STAGE = ROOT / "linux-lab" / "orchestrator" / "stages" / "configure.py"
BUILD_STAGE = ROOT / "linux-lab" / "orchestrator" / "stages" / "build_kernel.py"
STAGES_MODULE = ROOT / "linux-lab" / "orchestrator" / "core" / "stages.py"
STATE_MODULE = ROOT / "linux-lab" / "orchestrator" / "core" / "state.py"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
    if str(LINUX_LAB_ROOT) not in sys.path:
        sys.path.insert(0, str(LINUX_LAB_ROOT))
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def _make_fake_kernel_archive(path: Path, version: str) -> str:
    makefile = """olddefconfig:
\tmkdir -p $(O)
\ttest -f $(O)/.config

bzImage:
\tmkdir -p $(O)/arch/x86_64/boot
\tprintf 'fake kernel\\n' > $(O)/arch/x86_64/boot/bzImage

modules:
\tmkdir -p $(O)/lib/modules
\tprintf 'fake modules\\n' > $(O)/lib/modules/modules.builtin
"""
    files = {
        "Makefile": makefile,
        "README": "before\n",
    }
    with tarfile.open(path, "w:xz") as tar:
        for relative, content in files.items():
            data = content.encode("utf-8")
            info = tarfile.TarInfo(name=f"linux-{version}/{relative}")
            info.size = len(data)
            tar.addfile(info, io.BytesIO(data))
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _make_patch(path: Path, version: str) -> None:
    path.write_text(
        f"""diff -urNa kernel/linux-{version}/README kernel/linux-{version}/README
--- kernel/linux-{version}/README\t2026-03-14 00:00:00.000000000 +0000
+++ kernel/linux-{version}/README\t2026-03-14 00:00:00.000000000 +0000
@@ -1 +1 @@
-before
+after
""",
        encoding="utf-8",
    )


def _make_request(tmp_path: Path, version: str) -> object:
    return SimpleNamespace(
        kernel_version=version,
        arch="x86_64",
        image_release="noble",
        profiles=["debug"],
        mirror_region=None,
        compat_mode=False,
        legacy_args={},
        request_fingerprint="liveexec123456",
        artifact_root=tmp_path / "request",
        command="run",
        dry_run=False,
        stop_after="build-kernel",
    )


def _make_manifests(archive: Path, sha256: str, patch_path: Path, fragment_path: Path) -> object:
    return SimpleNamespace(
        kernels={
            "1.2.3": SimpleNamespace(
                source_url=archive.as_uri(),
                source_sha256=sha256,
                patch_set=str(patch_path),
                config_family="modern",
                tool_groups=[],
                example_groups=[],
            )
        },
        arches={
            "x86_64": SimpleNamespace(
                toolchain_prefix="",
                kernel_image_name="bzImage",
                qemu_machine="pc",
                qemu_cpu="qemu64",
                console="ttyS0",
                supported_images=["noble"],
            )
        },
        profiles={
            "debug": SimpleNamespace(
                fragment_ref=str(fragment_path),
                tool_groups=[],
                example_groups=[],
                host_tools=[],
            )
        },
    )


def test_live_run_executes_fetch_patch_configure_and_build(tmp_path: Path) -> None:
    fetch_mod = _load_module("linux_lab_fetch_stage_live", FETCH_STAGE)
    patch_mod = _load_module("linux_lab_patch_stage_live", PATCH_STAGE)
    config_mod = _load_module("linux_lab_config_stage_live", CONFIG_STAGE)
    build_mod = _load_module("linux_lab_build_stage_live", BUILD_STAGE)
    state_mod = _load_module("linux_lab_state_stage_live", STATE_MODULE)

    archive = tmp_path / "linux-1.2.3.tar.xz"
    patch_path = tmp_path / "kernel.patch"
    fragment_path = tmp_path / "debug.conf"
    sha256 = _make_fake_kernel_archive(archive, "1.2.3")
    _make_patch(patch_path, "1.2.3")
    fragment_path.write_text("CONFIG_TEST_FEATURE=y\n", encoding="utf-8")

    request = _make_request(tmp_path, "1.2.3")
    manifests = _make_manifests(archive, sha256, patch_path, fragment_path)
    request_root = Path(request.artifact_root)
    state_mod.ensure_request_dirs(request_root)

    fetch_result = fetch_mod.STAGE.executor(request, manifests, request_root)
    patch_result = patch_mod.STAGE.executor(request, manifests, request_root)
    config_result = config_mod.STAGE.executor(request, manifests, request_root)
    build_result = build_mod.STAGE.executor(request, manifests, request_root)

    assert fetch_result["status"] == "succeeded"
    assert Path(fetch_result["metadata"]["archive_path"]).is_file()

    source_tree = Path(patch_result["metadata"]["source_tree"])
    assert patch_result["status"] == "succeeded"
    assert source_tree.is_dir()
    assert (source_tree / "README").read_text(encoding="utf-8") == "after\n"

    output_config = Path(config_result["metadata"]["output_config"])
    assert config_result["status"] == "succeeded"
    assert output_config.is_file()
    config_text = output_config.read_text(encoding="utf-8")
    assert "CONFIG_TEST_FEATURE=y" in config_text
    assert "CONFIG_KGDB=y" in config_text

    kernel_image = Path(build_result["metadata"]["kernel_image_path"])
    assert build_result["status"] == "succeeded"
    assert kernel_image.is_file()
    assert kernel_image.read_text(encoding="utf-8") == "fake kernel\n"


def test_run_stage_command_streams_output_before_process_exit(tmp_path: Path) -> None:
    stages_mod = _load_module("linux_lab_core_stages_live", STAGES_MODULE)
    script = tmp_path / "emit.sh"
    script.write_text(
        "#!/usr/bin/env bash\nprintf 'start\\n'\nsleep 0.3\nprintf 'finish\\n'\n",
        encoding="utf-8",
    )
    script.chmod(0o755)
    log_path = tmp_path / "stream.log"
    errors: list[BaseException] = []

    def _runner() -> None:
        try:
            stages_mod.run_stage_command([str(script)], cwd=tmp_path, log_path=log_path)
        except BaseException as exc:  # pragma: no cover - surfaced by assertion below
            errors.append(exc)

    thread = threading.Thread(target=_runner)
    thread.start()

    saw_start = False
    deadline = time.time() + 1.0
    while time.time() < deadline:
        if log_path.is_file() and "start" in log_path.read_text(encoding="utf-8"):
            saw_start = True
            break
        time.sleep(0.05)

    thread.join()

    assert errors == []
    assert saw_start
    assert "finish" in log_path.read_text(encoding="utf-8")


def test_live_run_uses_request_root_for_relative_artifact_paths(tmp_path: Path, monkeypatch) -> None:
    fetch_mod = _load_module("linux_lab_fetch_stage_relative", FETCH_STAGE)
    patch_mod = _load_module("linux_lab_patch_stage_relative", PATCH_STAGE)
    config_mod = _load_module("linux_lab_config_stage_relative", CONFIG_STAGE)
    build_mod = _load_module("linux_lab_build_stage_relative", BUILD_STAGE)
    state_mod = _load_module("linux_lab_state_stage_relative", STATE_MODULE)

    monkeypatch.chdir(tmp_path)
    archive = tmp_path / "linux-1.2.3.tar.xz"
    patch_path = tmp_path / "kernel.patch"
    fragment_path = tmp_path / "debug.conf"
    sha256 = _make_fake_kernel_archive(archive, "1.2.3")
    _make_patch(patch_path, "1.2.3")
    fragment_path.write_text("CONFIG_TEST_FEATURE=y\n", encoding="utf-8")

    request = _make_request(tmp_path, "1.2.3")
    request.artifact_root = Path("request-rel")
    manifests = _make_manifests(archive, sha256, patch_path, fragment_path)
    request_root = tmp_path / "request-rel"
    state_mod.ensure_request_dirs(request_root)

    fetch_mod.STAGE.executor(request, manifests, request_root)
    patch_mod.STAGE.executor(request, manifests, request_root)
    config_mod.STAGE.executor(request, manifests, request_root)
    build_result = build_mod.STAGE.executor(request, manifests, request_root)

    kernel_image = request_root / "workspace" / "build" / "arch" / "x86_64" / "boot" / "bzImage"
    nested_images = list((request_root / "workspace" / "kernel").glob("**/request-rel/workspace/build/arch/x86_64/boot/bzImage"))

    assert build_result["status"] == "succeeded"
    assert kernel_image.is_file()
    assert nested_images == []
