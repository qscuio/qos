from __future__ import annotations

import os
import shutil
from pathlib import Path

from orchestrator.core.stages import StageDefinition, append_log, make_stage_result, run_stage_command
from orchestrator.core.tooling import (
    resolve_kernel_tool_plan,
    resolve_kernel_workspace_paths,
    resolve_tool_keys,
    resolve_tool_plan,
)


LINUX_LAB_ROOT = Path(__file__).resolve().parents[2]


def _linux_lab_root() -> Path:
    override = os.environ.get("LINUX_LAB_ROOT_OVERRIDE")
    if override:
        return Path(override)
    return LINUX_LAB_ROOT


def _resolve_tool_groups(manifests, request) -> list[str]:
    if "minimal" in request.profiles:
        return []
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
    if "minimal" in request.profiles:
        return []
    host_tools: list[str] = []
    for profile_key in request.profiles:
        for tool in manifests.profiles[profile_key].host_tools:
            if tool not in host_tools:
                host_tools.append(tool)
    return host_tools


class MissingAssetError(RuntimeError):
    pass


class MissingKernelWorkspaceError(RuntimeError):
    pass


def _copy_post_prepare_assets(tool: dict, checkout_dir: Path) -> None:
    for copy_rule in tool["post_prepare_asset_copies"]:
        source = Path(copy_rule["from"])
        if not source.exists():
            if tool["key"] == "crash":
                raise MissingAssetError(f"missing crash extension asset source: {source}")
            raise MissingAssetError(f"missing asset source: {source}")
        destination = checkout_dir / copy_rule["to"]
        if source.is_dir():
            shutil.copytree(source, destination, dirs_exist_ok=True)
        else:
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)


def _kernel_tool_cwd(request, kernel_tool: dict) -> Path:
    kernel_workspace = resolve_kernel_workspace_paths(request=request)
    if kernel_tool["name"] == "tools/libapi":
        return kernel_workspace["kernel_tree"]
    return kernel_workspace["workspace_root"]


def _validate_kernel_workspaces(request) -> None:
    kernel_workspace = resolve_kernel_workspace_paths(request=request)
    required_paths = [
        kernel_workspace["build_root"],
        kernel_workspace["tools_root"],
    ]
    for path in required_paths:
        if not path.exists():
            raise MissingKernelWorkspaceError(f"missing kernel workspace: {path}")


def _failed_stage_result(
    *,
    request,
    log_path: Path,
    metadata: dict,
    error_kind: str,
    error_message: str,
) -> dict:
    append_log(log_path, error_message)
    return make_stage_result(
        stage="build-tools",
        status="failed",
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "config-plan"),
        outputs=("tools-plan",),
        log_path=log_path,
        metadata=metadata,
        error_kind=error_kind,
        error_message=error_message,
    )


def _execute(request, manifests, request_root: Path) -> dict:
    tool_groups = _resolve_tool_groups(manifests, request)
    tool_keys = resolve_tool_keys(tool_groups)
    external_tools = resolve_tool_plan(tool_keys=tool_keys, linux_lab_root=_linux_lab_root())
    kernel_tools = resolve_kernel_tool_plan(request=request, manifests=manifests) if "kernel-tools" in tool_groups else []
    post_prepare_asset_copies: list[dict[str, str]] = []
    for tool in external_tools:
        for copy_rule in tool["post_prepare_asset_copies"]:
            if copy_rule not in post_prepare_asset_copies:
                post_prepare_asset_copies.append(copy_rule)
    status = "placeholder"
    if request.command == "run" and request.dry_run:
        status = "dry-run"
    log_path = request_root / "logs" / "build-tools.log"
    log_path.write_text("build-tools: runtime metadata emitted\n", encoding="utf-8")
    metadata = {
        "tool_groups": tool_groups,
        "host_tools": _resolve_host_tools(manifests, request),
        "tools": external_tools,
        "external_tools": external_tools,
        "kernel_tools": kernel_tools,
        "post_prepare_asset_copies": post_prepare_asset_copies,
    }
    if request.command == "run" and not request.dry_run:
        try:
            for tool in external_tools:
                checkout_dir = Path(tool["checkout_dir"])
                build_workdir = Path(tool.get("build_workdir", tool["checkout_dir"]))
                if not checkout_dir.exists():
                    checkout_dir.parent.mkdir(parents=True, exist_ok=True)
                    run_stage_command(tool["clone_command"], cwd=checkout_dir.parent, log_path=log_path)
                for command in tool["prepare_commands"]:
                    run_stage_command(command, cwd=checkout_dir, log_path=log_path)
                _copy_post_prepare_assets(tool, checkout_dir)
                if tool["build_policy"] == "host-build":
                    for command in tool["build_commands"]:
                        run_stage_command(command, cwd=build_workdir, log_path=log_path)
            if kernel_tools:
                _validate_kernel_workspaces(request)
                for kernel_tool in kernel_tools:
                    run_stage_command(
                        kernel_tool["command"],
                        cwd=_kernel_tool_cwd(request, kernel_tool),
                        log_path=log_path,
                    )
            status = "succeeded"
        except MissingAssetError as exc:
            return _failed_stage_result(
                request=request,
                log_path=log_path,
                metadata=metadata,
                error_kind="missing-asset",
                error_message=str(exc),
            )
        except MissingKernelWorkspaceError as exc:
            return _failed_stage_result(
                request=request,
                log_path=log_path,
                metadata=metadata,
                error_kind="missing-kernel-workspace",
                error_message=str(exc),
            )
        except Exception as exc:
            return _failed_stage_result(
                request=request,
                log_path=log_path,
                metadata=metadata,
                error_kind="command-failed",
                error_message=str(exc),
            )
    return make_stage_result(
        stage="build-tools",
        status=status,
        request_fingerprint=request.request_fingerprint,
        inputs=("resolved-request", "config-plan"),
        outputs=("tools-plan",),
        log_path=log_path,
        metadata=metadata,
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
