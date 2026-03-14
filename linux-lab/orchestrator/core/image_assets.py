from __future__ import annotations

from pathlib import Path


def resolve_mirror_url(*, image_release: str, arch: str, region: str | None) -> str | None:
    if region is None:
        return None
    if region == "cn":
        if arch == "arm64":
            return "https://mirrors.tuna.tsinghua.edu.cn/ubuntu-ports/"
        return "https://mirrors.tuna.tsinghua.edu.cn/ubuntu/"
    if region == "sg":
        if arch == "arm64":
            return "https://mirror.sg.gs/ubuntu-ports/"
        return "http://mirror.nus.edu.sg/archive.ubuntu.com/ubuntu/"
    raise ValueError(f"unsupported mirror region for runtime assets: {region}")


def release_asset_dir(linux_lab_root: Path, image_release: str) -> Path:
    return linux_lab_root / "images" / "releases" / image_release


def release_asset_paths(linux_lab_root: Path, image_release: str) -> dict[str, Path]:
    release_dir = release_asset_dir(linux_lab_root, image_release)
    return {
        "release_dir": release_dir,
        "netcfg_path": release_dir / "01-netcfg.yaml",
        "sources_list_path": release_dir / "sources.list",
    }
