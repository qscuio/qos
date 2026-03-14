from __future__ import annotations

from pathlib import Path

from orchestrator.core.stages import StageDefinition, make_stage_result


def _execute(request, manifests, request_root: Path) -> dict:
    kernel = manifests.kernels[request.kernel_version]
    log_path = request_root / "logs" / "fetch.log"
    log_path.write_text("fetch: resolved source archive metadata\n", encoding="utf-8")
    return make_stage_result(
        stage="fetch",
        status="placeholder",
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request",),
        outputs=("source-plan",),
        log_path=log_path,
        metadata={
            "source_url": kernel.source_url,
            "source_sha256": kernel.source_sha256,
            "cache_archive": f"build/linux-lab/cache/linux-{request.kernel_version}.tar.xz",
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
