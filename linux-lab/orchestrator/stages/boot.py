from __future__ import annotations

from orchestrator.core.stages import StageDefinition, plan_result_executor


STAGE = StageDefinition(
    name="boot",
    depends_on=("build-image",),
    consumes=("resolved-request", "image-plan"),
    produces=("boot-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=plan_result_executor(
        "boot",
        ("resolved-request", "image-plan"),
        ("boot-plan",),
        "boot: placeholder boot planning emitted",
    ),
)
