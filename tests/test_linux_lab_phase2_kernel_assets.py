from __future__ import annotations

import hashlib
import importlib.util
import io
import tarfile
from pathlib import Path
from types import ModuleType

import pytest


ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "linux-lab" / "orchestrator" / "core" / "kernel_assets.py"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _make_tarball(path: Path, root_name: str, files: dict[str, str]) -> str:
    with tarfile.open(path, "w:xz") as tar:
        for relative, content in files.items():
            data = content.encode("utf-8")
            info = tarfile.TarInfo(name=f"{root_name}/{relative}")
            info.size = len(data)
            tar.addfile(info, io.BytesIO(data))
    return hashlib.sha256(path.read_bytes()).hexdigest()


def test_fetch_kernel_archive_copies_local_source_and_verifies_sha256(tmp_path: Path) -> None:
    module = _load_module("linux_lab_kernel_assets", MODULE_PATH)
    archive = tmp_path / "linux-1.2.3.tar.xz"
    sha256 = _make_tarball(archive, "linux-1.2.3", {"README": "hello\n"})

    fetched = module.fetch_kernel_archive(
        version="1.2.3",
        source_url=archive.as_uri(),
        source_sha256=sha256,
        cache_dir=tmp_path / "cache",
    )

    assert fetched.is_file()
    assert fetched.read_bytes() == archive.read_bytes()


def test_fetch_kernel_archive_rejects_sha256_mismatch(tmp_path: Path) -> None:
    module = _load_module("linux_lab_kernel_assets", MODULE_PATH)
    archive = tmp_path / "linux-1.2.3.tar.xz"
    _make_tarball(archive, "linux-1.2.3", {"README": "hello\n"})

    with pytest.raises(ValueError):
        module.fetch_kernel_archive(
            version="1.2.3",
            source_url=archive.as_uri(),
            source_sha256="deadbeef",
            cache_dir=tmp_path / "cache",
        )


def test_extract_kernel_archive_returns_extracted_tree(tmp_path: Path) -> None:
    module = _load_module("linux_lab_kernel_assets", MODULE_PATH)
    archive = tmp_path / "linux-1.2.3.tar.xz"
    sha256 = _make_tarball(archive, "linux-1.2.3", {"README": "hello\n"})
    fetched = module.fetch_kernel_archive(
        version="1.2.3",
        source_url=archive.as_uri(),
        source_sha256=sha256,
        cache_dir=tmp_path / "cache",
    )

    tree = module.extract_kernel_archive(
        version="1.2.3",
        archive_path=fetched,
        workspace_root=tmp_path / "workspace",
    )

    assert tree == tmp_path / "workspace" / "kernel" / "linux-1.2.3"
    assert (tree / "README").read_text(encoding="utf-8") == "hello\n"


def test_apply_kernel_patch_updates_extracted_tree(tmp_path: Path) -> None:
    module = _load_module("linux_lab_kernel_assets", MODULE_PATH)
    root = tmp_path / "workspace"
    target = root / "kernel" / "linux-1.2.3"
    target.mkdir(parents=True)
    sample = target / "sample.txt"
    sample.write_text("before\n", encoding="utf-8")

    patch_path = tmp_path / "kernel.patch"
    patch_path.write_text(
        """diff -urNa kernel/linux-1.2.3/sample.txt kernel/linux-1.2.3/sample.txt
--- kernel/linux-1.2.3/sample.txt\t2026-03-14 00:00:00.000000000 +0000
+++ kernel/linux-1.2.3/sample.txt\t2026-03-14 00:00:00.000000000 +0000
@@ -1 +1 @@
-before
+after
""",
        encoding="utf-8",
    )

    module.apply_kernel_patch(workspace_root=root, patch_path=patch_path)

    assert sample.read_text(encoding="utf-8") == "after\n"


def test_merge_kernel_config_applies_fragments_in_order(tmp_path: Path) -> None:
    module = _load_module("linux_lab_kernel_assets", MODULE_PATH)
    base = tmp_path / "defconfig"
    fragment_a = tmp_path / "a.conf"
    fragment_b = tmp_path / "b.conf"
    output = tmp_path / ".config"

    base.write_text("CONFIG_ALPHA=y\nCONFIG_BETA=m\n", encoding="utf-8")
    fragment_a.write_text("# CONFIG_BETA is not set\nCONFIG_GAMMA=y\n", encoding="utf-8")
    fragment_b.write_text("CONFIG_GAMMA=m\nCONFIG_DELTA=y\n", encoding="utf-8")

    module.merge_kernel_config(base, [fragment_a, fragment_b], output)

    text = output.read_text(encoding="utf-8")
    assert "CONFIG_ALPHA=y" in text
    assert "# CONFIG_BETA is not set" in text
    assert "CONFIG_GAMMA=m" in text
    assert "CONFIG_DELTA=y" in text
