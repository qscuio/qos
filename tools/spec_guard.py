#!/usr/bin/env python3
"""Consistency checks for the QOS architecture spec."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


def _section(text: str, heading: str) -> str:
    pattern = re.compile(rf"^###\s+{re.escape(heading)}\s*$", re.MULTILINE)
    start_match = pattern.search(text)
    if not start_match:
        return ""

    start = start_match.end()
    remainder = text[start:]
    end_match = re.search(r"^###\s+", remainder, re.MULTILINE)
    if end_match:
        return remainder[: end_match.start()]
    return remainder


def validate_spec(spec_text: str) -> list[str]:
    errors: list[str] = []
    lower = spec_text.lower()

    if "wnohang" in lower:
        if not re.search(r"^\s*\d+\s+waitpid\(", spec_text, re.MULTILINE):
            errors.append(
                "E001 waitpid/WNOHANG mismatch: shell behavior uses waitpid(-1, WNOHANG) but syscall table does not define waitpid()."
            )

    if "doubly-linked list" in lower:
        has_next = re.search(r"struct\s+thread\s*\*\s*next\b", spec_text) is not None
        has_prev = re.search(r"struct\s+thread\s*\*\s*prev\b", spec_text) is not None
        if has_next and not has_prev:
            errors.append(
                "E002 run queue mismatch: run queue is described as doubly-linked but thread_t lacks a prev pointer."
            )

    if "| ext2" in lower or "virtio-blk" in lower or "/data" in spec_text:
        drivers = _section(spec_text, "Drivers").lower()
        storage_patterns = (
            r"\bvirtio-blk\b",
            r"\bahci\b",
            r"\bata\b",
            r"\bide\b",
            r"\bnvme\b",
            r"\bstorage\s+driver\b",
            r"\bblock\s+driver\b",
        )
        if drivers and not any(re.search(pattern, drivers) for pattern in storage_patterns):
            errors.append(
                "E003 storage-driver gap: ext2 /data is specified but no storage driver appears in the Drivers section."
            )

    fork_row_match = re.search(r"\|\s*`fork\(\)`\s*\|.*", spec_text)
    exec_row_match = re.search(r"\|\s*`exec\(\)`\s*\|.*", spec_text)

    fork_inherits_pending = False
    if fork_row_match:
        fork_row = fork_row_match.group(0).lower()
        fork_inherits_pending = bool(
            "sig_pending" in fork_row
            and not any(term in fork_row for term in ("starts empty", "is empty", "cleared", "reset"))
        )

    exec_preserves_pending = False
    if exec_row_match:
        exec_row = exec_row_match.group(0).lower()
        exec_preserves_pending = bool(
            "sig_pending" in exec_row
            and not any(term in exec_row for term in ("cleared", "is empty", "starts empty", "reset"))
        )
    if fork_inherits_pending or exec_preserves_pending:
        errors.append(
            "E004 signal semantics mismatch: pending signals should not be inherited across fork() and should be cleared on exec()."
        )

    if "enable 64-bit long mode" in lower and "in-protected-mode disk i/o" in lower:
        errors.append(
            "E005 stage2 mode wording mismatch: disk I/O step says 'in-protected-mode' after long mode is enabled."
        )

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("spec", type=Path, help="Path to architecture spec markdown file")
    args = parser.parse_args()

    errors = validate_spec(args.spec.read_text(encoding="utf-8"))
    if not errors:
        print("Spec consistency checks passed")
        return 0

    for err in errors:
        print(err)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
