from __future__ import annotations

from orchestrator.core.stages import StageDefinition, plan_result_executor


STAGE = StageDefinition(
    name="fetch",
    depends_on=(),
    consumes=("resolved-request",),
    produces=("source-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=plan_result_executor(
        "fetch",
        ("resolved-request",),
        ("source-plan",),
        "fetch: placeholder source planning emitted",
    ),
)
