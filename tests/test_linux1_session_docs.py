from __future__ import annotations

import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DOC = ROOT / "docs" / "superpowers" / "sessions" / "2026-03-14-linux1-authentic-session-history.md"
CACHE_DOC = (
    ROOT
    / "docs"
    / "superpowers"
    / "sessions"
    / "2026-03-14-linux1-authentic-cache-sanitized.jsonl"
)
MANIFEST = (
    ROOT
    / "docs"
    / "superpowers"
    / "sessions"
    / "2026-03-14-linux1-authentic-cache-manifest.md"
)


def test_linux1_session_history_doc_captures_transcript_and_debug_method() -> None:
    assert DOC.is_file(), f"missing session history doc: {DOC}"
    text = DOC.read_text(encoding="utf-8")

    required_markers = [
        "## Visible Session Transcript",
        "## Reconstructed Progress Log",
        "## Debugging Method",
        "## Hypothesis Log",
        "## Command And Evidence Log",
        "## Limits Of This Record",
        "The current implementation is a test harness, not a bootable OS.",
        "please check the design and plan for xv6 and execute the plan for me.",
        "Linux 1.0.0 now boots in this repo and reaches the custom init plus shell prompt.",
        "Because the work was done in a separate Git worktree, not in your current `main` checkout.",
    ]

    for marker in required_markers:
        assert marker in text, f"missing marker {marker!r} from {DOC}"


def test_linux1_generated_build_outputs_are_gitignored() -> None:
    generated_paths = [
        "linux-1.0.0/.config",
        "linux-1.0.0/.depend",
        "linux-1.0.0/.version",
        "linux-1.0.0/config.old",
        "linux-1.0.0/boot/bootsect",
        "linux-1.0.0/boot/bootsect.o",
        "linux-1.0.0/boot/bootsect.s",
        "linux-1.0.0/boot/setup",
        "linux-1.0.0/include/linux/autoconf.h",
        "linux-1.0.0/kernel/ksyms.lst",
        "linux-1.0.0/tools/build",
        "linux-1.0.0/tools/version.h",
        "linux-1.0.0/tools/zSystem",
        "linux-1.0.0/zBoot/zSystem",
        "linux-1.0.0/zImage",
        "linux-1.0.0/zSystem.map",
        "linux-1.0.0/drivers/FPU-emu/reg_div.o",
        "linux-1.0.0/drivers/FPU-emu/math.a",
        "linux-1.0.0/fs/ext2/.depend",
        "linux-1.0.0/kernel/sys_call.s",
    ]

    for path in generated_paths:
        result = subprocess.run(
            ["git", "check-ignore", path],
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert result.returncode == 0, f"expected {path} to be ignored"


def test_linux1_cache_export_exists_and_is_sanitized() -> None:
    assert CACHE_DOC.is_file(), f"missing sanitized cache export: {CACHE_DOC}"
    assert MANIFEST.is_file(), f"missing cache manifest: {MANIFEST}"

    manifest_text = MANIFEST.read_text(encoding="utf-8")
    assert "Protected records omitted" in manifest_text
    assert "reasoning" in manifest_text
    assert "developer messages" in manifest_text

    lines = CACHE_DOC.read_text(encoding="utf-8").splitlines()
    assert len(lines) > 1000, "cache export is unexpectedly small"

    parsed = [json.loads(line) for line in lines[:200]]
    assert all(item.get("type") == "response_item" for item in parsed)

    payload_types = {item["payload"]["type"] for item in parsed}
    assert "message" in payload_types
    assert "function_call" in payload_types or "custom_tool_call" in payload_types

    all_items = [json.loads(line) for line in lines]
    assert any(
        item["payload"].get("type") == "message" and item["payload"].get("role") == "assistant"
        for item in all_items
    )
    assert any(
        item["payload"].get("type") == "message" and item["payload"].get("role") == "user"
        for item in all_items
    )
    assert any(
        item["payload"].get("type") in {"function_call", "custom_tool_call"}
        for item in all_items
    )
    assert all(item.get("type") == "response_item" for item in all_items)
    assert all(item["payload"].get("type") != "reasoning" for item in all_items)
    assert all(
        not (
            item["payload"].get("type") == "message"
            and item["payload"].get("role") == "developer"
        )
        for item in all_items
    )
