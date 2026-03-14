from __future__ import annotations

import hashlib
import shutil
import subprocess
import tarfile
from pathlib import Path
from urllib.parse import unquote, urlsplit
from urllib.request import url2pathname, urlopen


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def fetch_kernel_archive(version: str, source_url: str, source_sha256: str, cache_dir: Path) -> Path:
    cache_dir.mkdir(parents=True, exist_ok=True)
    archive_path = cache_dir / f"linux-{version}.tar.xz"

    parsed = urlsplit(source_url)
    if parsed.scheme == "file":
        source_path = Path(url2pathname(unquote(parsed.path)))
        shutil.copyfile(source_path, archive_path)
    else:
        with urlopen(source_url) as response, archive_path.open("wb") as destination:
            shutil.copyfileobj(response, destination)

    digest = _sha256(archive_path)
    if digest != source_sha256:
        raise ValueError(
            f"sha256 mismatch for linux-{version}.tar.xz: expected {source_sha256}, got {digest}"
        )
    return archive_path


def extract_kernel_archive(version: str, archive_path: Path, workspace_root: Path) -> Path:
    kernel_root = workspace_root / "kernel"
    source_tree = kernel_root / f"linux-{version}"
    if source_tree.exists():
        shutil.rmtree(source_tree)
    kernel_root.mkdir(parents=True, exist_ok=True)
    with tarfile.open(archive_path, "r:*") as tar:
        tar.extractall(kernel_root, filter="data")
    return source_tree


def apply_kernel_patch(workspace_root: Path, patch_path: Path) -> None:
    subprocess.run(
        ["patch", "-p0", "-i", str(patch_path)],
        cwd=workspace_root,
        check=True,
        capture_output=True,
        text=True,
    )


def _parse_config_lines(text: str) -> list[tuple[str, str]]:
    entries: list[tuple[str, str]] = []
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("CONFIG_") and "=" in line:
            key = line.split("=", 1)[0]
            entries.append((key, line))
            continue
        if line.startswith("# CONFIG_") and line.endswith(" is not set"):
            key = line[len("# ") :].split(" ", 1)[0]
            entries.append((key, line))
    return entries


def merge_kernel_config(base_defconfig: Path, fragment_paths: list[Path], output_path: Path) -> Path:
    merged: dict[str, str] = {}
    order: list[str] = []

    for path in [base_defconfig, *fragment_paths]:
        for key, line in _parse_config_lines(path.read_text(encoding="utf-8")):
            if key not in merged:
                order.append(key)
            merged[key] = line

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(merged[key] for key in order) + "\n", encoding="utf-8")
    return output_path


def resolve_profile_fragment_paths(manifests, profiles: list[str], linux_lab_root: Path) -> list[Path]:
    resolved: list[Path] = []
    seen: set[Path] = set()
    repo_root = linux_lab_root.parent if linux_lab_root.name == "linux-lab" else linux_lab_root

    for profile_key in profiles:
        profile = manifests.profiles[profile_key]
        if not profile.fragment_ref:
            continue
        path = Path(profile.fragment_ref)
        if not path.is_absolute():
            path = repo_root / path
        if path not in seen:
            seen.add(path)
            resolved.append(path)
    return resolved
