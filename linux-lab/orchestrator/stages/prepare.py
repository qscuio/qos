from __future__ import annotations

from orchestrator.core.stages import StageDefinition, prepare_executor


STAGE = StageDefinition(
    name="prepare",
    depends_on=("fetch",),
    consumes=("resolved-request", "source-plan"),
    produces=("prepare-report",),
    supports_delegate=False,
    supports_placeholder=False,
    executor=prepare_executor("prepare"),
)
