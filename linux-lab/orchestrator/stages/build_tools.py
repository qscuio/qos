from __future__ import annotations

from orchestrator.core.stages import StageDefinition, plan_result_executor


STAGE = StageDefinition(
    name="build-tools",
    depends_on=("configure",),
    consumes=("resolved-request", "config-plan"),
    produces=("tools-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=plan_result_executor(
        "build-tools",
        ("resolved-request", "config-plan"),
        ("tools-plan",),
        "build-tools: placeholder tool planning emitted",
    ),
)
