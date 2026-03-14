from __future__ import annotations

from orchestrator.core.stages import StageDefinition, plan_result_executor


STAGE = StageDefinition(
    name="build-image",
    depends_on=("build-kernel", "build-tools", "build-examples"),
    consumes=("resolved-request", "kernel-plan", "tools-plan", "examples-plan"),
    produces=("image-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=plan_result_executor(
        "build-image",
        ("resolved-request", "kernel-plan", "tools-plan", "examples-plan"),
        ("image-plan",),
        "build-image: placeholder image planning emitted",
    ),
)
