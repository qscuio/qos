from __future__ import annotations

import json
from dataclasses import asdict, is_dataclass
from datetime import UTC, datetime
from pathlib import Path
from typing import Any


def iso_now() -> str:
    return datetime.now(UTC).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def ensure_request_dirs(request_root: Path) -> None:
    (request_root / "state").mkdir(parents=True, exist_ok=True)
    (request_root / "logs").mkdir(parents=True, exist_ok=True)


def _json_default(value: Any) -> Any:
    if is_dataclass(value):
        return asdict(value)
    if isinstance(value, Path):
        return str(value)
    raise TypeError(f"unsupported JSON value: {value!r}")


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True, default=_json_default) + "\n", encoding="utf-8")


def touch_log(path: Path, line: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(line + "\n", encoding="utf-8")


def request_to_dict(request) -> dict[str, Any]:
    return {
        "kernel_version": request.kernel_version,
        "arch": request.arch,
        "image_release": request.image_release,
        "profiles": request.profiles,
        "mirror_region": request.mirror_region,
        "compat_mode": request.compat_mode,
        "legacy_args": request.legacy_args,
        "request_fingerprint": request.request_fingerprint,
        "artifact_root": str(request.artifact_root),
    }
