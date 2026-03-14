from __future__ import annotations

from orchestrator.core.stages import StageDefinition, plan_result_executor


STAGE = StageDefinition(
    name="build-kernel",
    depends_on=("configure",),
    consumes=("resolved-request", "config-plan"),
    produces=("kernel-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=plan_result_executor(
        "build-kernel",
        ("resolved-request", "config-plan"),
        ("kernel-plan",),
        "build-kernel: placeholder kernel artifact planning emitted",
    ),
)
