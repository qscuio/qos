from __future__ import annotations

import importlib.util
from pathlib import Path
from types import ModuleType

import yaml


ROOT = Path(__file__).resolve().parents[1]
IMAGE_ASSETS_MODULE = ROOT / "linux-lab" / "orchestrator" / "core" / "image_assets.py"
QEMU_MODULE = ROOT / "linux-lab" / "orchestrator" / "core" / "qemu.py"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _read_yaml(path: Path) -> dict:
    data = yaml.safe_load(path.read_text(encoding="utf-8"))
    assert isinstance(data, dict), f"{path} must decode to a mapping"
    return data


def test_release_assets_are_ported() -> None:
    expected = [
        ROOT / "linux-lab" / "images" / "releases" / "noble" / "01-netcfg.yaml",
        ROOT / "linux-lab" / "images" / "releases" / "noble" / "sources.list",
        ROOT / "linux-lab" / "images" / "releases" / "jammy" / "01-netcfg.yaml",
        ROOT / "linux-lab" / "images" / "releases" / "jammy" / "sources.list",
    ]
    missing = [str(path) for path in expected if not path.is_file()]
    assert missing == [], f"missing release assets: {missing}"


def test_image_manifests_reference_runtime_assets() -> None:
    noble = _read_yaml(ROOT / "linux-lab" / "manifests" / "images" / "noble.yaml")
    jammy = _read_yaml(ROOT / "linux-lab" / "manifests" / "images" / "jammy.yaml")

    assert noble["recipe_ref"] == "linux-lab/images/releases/noble"
    assert jammy["recipe_ref"] == "linux-lab/images/releases/jammy"


def test_image_asset_helper_resolves_region_specific_mirror() -> None:
    module = _load_module("linux_lab_image_assets", IMAGE_ASSETS_MODULE)

    assert module.resolve_mirror_url(image_release="noble", arch="x86_64", region="sg").endswith("/ubuntu/")
    assert module.resolve_mirror_url(image_release="noble", arch="arm64", region="sg").endswith("/ubuntu-ports/")
    assert module.resolve_mirror_url(image_release="jammy", arch="x86_64", region="cn").startswith("https://mirrors.tuna")


def test_qemu_helper_builds_boot_commands_for_both_arches() -> None:
    module = _load_module("linux_lab_qemu", QEMU_MODULE)

    x86 = module.build_qemu_command(
        arch="x86_64",
        kernel_image=Path("build/linux-lab/kernel/arch/x86_64/boot/bzImage"),
        disk_image=Path("build/linux-lab/images/noble.img"),
        shared_dir=Path("."),
    )
    arm = module.build_qemu_command(
        arch="arm64",
        kernel_image=Path("build/linux-lab/kernel/arch/arm64/boot/Image.gz"),
        disk_image=Path("build/linux-lab/images/jammy.img"),
        shared_dir=Path("."),
    )

    assert x86[0] == "qemu-system-x86_64"
    assert "root=/dev/sda" in " ".join(x86)
    assert "console=ttyS0" in " ".join(x86)
    assert arm[0] == "qemu-system-aarch64"
    assert "root=/dev/vda" in " ".join(arm)
    assert "console=ttyAMA0" in " ".join(arm)
