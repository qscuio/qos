from __future__ import annotations

from orchestrator.core.stages import StageDefinition, plan_result_executor


STAGE = StageDefinition(
    name="configure",
    depends_on=("patch",),
    consumes=("resolved-request", "patch-plan"),
    produces=("config-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=plan_result_executor(
        "configure",
        ("resolved-request", "patch-plan"),
        ("config-plan",),
        "configure: placeholder config planning emitted",
    ),
)
