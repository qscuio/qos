#!/usr/bin/env python3
"""Validate C and Rust boot_info contract consistency."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


C_TYPE_TO_RUST = {
    "uint64_t": "u64",
    "uint32_t": "u32",
    "uint8_t": "u8",
}


def _extract_c_boot_info_fields(c_text: str) -> list[tuple[str, str]]:
    body = ""
    for match in re.finditer(
        r"typedef\s+struct\s*\{(?P<body>.*?)\}\s*(?P<name>[A-Za-z_]\w*)\s*;",
        c_text,
        re.S,
    ):
        if match.group("name") == "boot_info_t":
            body = match.group("body")
            break

    if not body:
        return []
    fields: list[tuple[str, str]] = []
    for line in body.splitlines():
        clean = line.split("//", 1)[0].strip()
        if not clean:
            continue

        array_match = re.match(r"([A-Za-z_][\w\s]*)\s+([A-Za-z_][\w]*)\s*\[[^\]]+\]\s*;", clean)
        if array_match:
            c_type = " ".join(array_match.group(1).split())
            name = array_match.group(2)
            if c_type in C_TYPE_TO_RUST:
                fields.append((name, C_TYPE_TO_RUST[c_type]))
            else:
                fields.append((name, "opaque"))
            continue

        field_match = re.match(r"([A-Za-z_][\w\s]*)\s+([A-Za-z_][\w]*)\s*;", clean)
        if field_match:
            c_type = " ".join(field_match.group(1).split())
            name = field_match.group(2)
            rust_type = C_TYPE_TO_RUST.get(c_type, "opaque")
            fields.append((name, rust_type))

    return fields


def _extract_rust_boot_info_fields(r_text: str) -> list[tuple[str, str]]:
    match = re.search(r"pub\s+struct\s+BootInfo\s*\{(?P<body>.*?)\}", r_text, re.S)
    if not match:
        return []

    body = match.group("body")
    fields: list[tuple[str, str]] = []

    for line in body.splitlines():
        clean = line.split("//", 1)[0].strip().rstrip(",")
        if not clean.startswith("pub "):
            continue

        field_match = re.match(r"pub\s+([A-Za-z_][\w]*)\s*:\s*([^,]+)$", clean)
        if not field_match:
            continue

        name = field_match.group(1)
        rust_type = field_match.group(2).strip()
        fields.append((name, rust_type))

    return fields


def validate_boot_info_contract(c_text: str, r_text: str) -> list[str]:
    issues: list[str] = []

    c_fields = _extract_c_boot_info_fields(c_text)
    r_fields = _extract_rust_boot_info_fields(r_text)

    if not c_fields:
        issues.append("B000 missing C boot_info_t definition")
        return issues

    if not r_fields:
        issues.append("B000 missing Rust BootInfo definition")
        return issues

    if len(c_fields) != len(r_fields):
        issues.append(
            f"B001 field count mismatch: C has {len(c_fields)} fields, Rust has {len(r_fields)} fields"
        )
        return issues

    for idx, ((c_name, c_type), (r_name, r_type)) in enumerate(zip(c_fields, r_fields), start=1):
        if c_name != r_name:
            issues.append(
                f"B002 field name mismatch at position {idx}: C '{c_name}' vs Rust '{r_name}'"
            )
            continue

        if c_type == "opaque":
            continue

        normalized_r_type = r_type
        if r_type.startswith("["):
            normalized_r_type = "array"

        if normalized_r_type not in (c_type, "array") and c_name not in ("mmap_entries", "_pad1"):
            issues.append(
                f"B003 field type mismatch for '{c_name}': expected Rust type compatible with {c_type}, got {r_type}"
            )

    return issues


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("c_header", type=Path)
    parser.add_argument("rust_file", type=Path)
    args = parser.parse_args()

    issues = validate_boot_info_contract(
        args.c_header.read_text(encoding="utf-8"),
        args.rust_file.read_text(encoding="utf-8"),
    )

    if not issues:
        print("boot_info contract checks passed")
        return 0

    for issue in issues:
        print(issue)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
