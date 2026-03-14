from __future__ import annotations

from orchestrator.core.stages import StageDefinition, plan_result_executor


STAGE = StageDefinition(
    name="patch",
    depends_on=("prepare",),
    consumes=("resolved-request", "source-plan", "prepare-report"),
    produces=("patch-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=plan_result_executor(
        "patch",
        ("resolved-request", "source-plan", "prepare-report"),
        ("patch-plan",),
        "patch: placeholder patch metadata emitted",
    ),
)
