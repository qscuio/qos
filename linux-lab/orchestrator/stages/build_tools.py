from __future__ import annotations

from pathlib import Path

from orchestrator.core.stages import StageDefinition, make_stage_result
from orchestrator.core.tooling import resolve_tool_keys, resolve_tool_plan


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _resolve_tool_groups(manifests, request) -> list[str]:
    kernel = manifests.kernels[request.kernel_version]
    groups: list[str] = []
    for group in kernel.tool_groups:
        if group not in groups:
            groups.append(group)
    for profile_key in request.profiles:
        for group in manifests.profiles[profile_key].tool_groups:
            if group not in groups:
                groups.append(group)
    return groups


def _resolve_host_tools(manifests, request) -> list[str]:
    host_tools: list[str] = []
    for profile_key in request.profiles:
        for tool in manifests.profiles[profile_key].host_tools:
            if tool not in host_tools:
                host_tools.append(tool)
    return host_tools


def _execute(request, manifests, request_root: Path) -> dict:
    tool_groups = _resolve_tool_groups(manifests, request)
    tool_keys = resolve_tool_keys(tool_groups)
    tools = resolve_tool_plan(tool_keys=tool_keys, linux_lab_root=LINUX_LAB_ROOT)
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"
    log_path = request_root / "logs" / "build-tools.log"
    log_path.write_text("build-tools: runtime metadata emitted\n", encoding="utf-8")
    return make_stage_result(
        stage="build-tools",
        status=status,
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "config-plan"),
        outputs=("tools-plan",),
        log_path=log_path,
        metadata={
            "tool_groups": tool_groups,
            "host_tools": _resolve_host_tools(manifests, request),
            "tools": tools,
        },
    )


STAGE = StageDefinition(
    name="build-tools",
    depends_on=("configure",),
    consumes=("resolved-request", "config-plan"),
    produces=("tools-plan",),
    supports_delegate=False,
    supports_placeholder=True,
    executor=_execute,
)
