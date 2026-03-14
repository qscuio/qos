from __future__ import annotations

from pathlib import Path

from orchestrator.core.kernel_assets import fetch_kernel_archive
from orchestrator.core.stages import StageDefinition, make_stage_result


def _execute(request, manifests, request_root: Path) -> dict:
    kernel = manifests.kernels[request.kernel_version]
    log_path = request_root / "logs" / "fetch.log"
    cache_archive = request_root.parents[1] / "cache" / f"linux-{request.kernel_version}.tar.xz"
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"
    elif request.command == "run":
        archive_path = fetch_kernel_archive(
            version=request.kernel_version,
            source_url=kernel.source_url,
            source_sha256=kernel.source_sha256,
            cache_dir=request_root.parents[1] / "cache",
        )
        status = "succeeded"
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text(f"fetch: downloaded {archive_path}\n", encoding="utf-8")
    else:
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text("fetch: resolved source archive metadata\n", encoding="utf-8")
    return make_stage_result(
        stage="fetch",
        status=status,
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request",),
        outputs=("source-plan",),
        log_path=log_path,
        metadata={
            "source_url": kernel.source_url,
            "source_sha256": kernel.source_sha256,
            "cache_archive": str(cache_archive),
            "archive_path": str(cache_archive),
        },
    )


STAGE = StageDefinition(
    name="fetch",
    depends_on=(),
    consumes=("resolved-request",),
    produces=("source-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=_execute,
)
