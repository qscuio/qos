from __future__ import annotations

from orchestrator.core.stages import StageDefinition, plan_result_executor


STAGE = StageDefinition(
    name="build-examples",
    depends_on=("build-kernel", "build-tools"),
    consumes=("resolved-request", "kernel-plan", "tools-plan"),
    produces=("examples-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=plan_result_executor(
        "build-examples",
        ("resolved-request", "kernel-plan", "tools-plan"),
        ("examples-plan",),
        "build-examples: placeholder example planning emitted",
    ),
)
