from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIBC_C = ROOT / "c-os/libc/libc.c"

# These calls indicate direct host libc delegation instead of standalone wrappers.
NAMES = [
    "malloc",
    "free",
    "realloc",
    "strtok",
    "printf",
    "vprintf",
    "putchar",
    "puts",
    "getchar",
    "fgets",
    "fputs",
    "fopen",
    "fclose",
    "fread",
    "fwrite",
    "fseek",
    "ftell",
    "fflush",
    "fileno",
    "fork",
    "execv",
    "execve",
    "waitpid",
    "socket",
    "bind",
    "listen",
    "accept",
    "connect",
    "send",
    "recv",
    "sendto",
    "recvfrom",
    "close",
    "kill",
    "signal",
    "sigaction",
    "sigprocmask",
    "sigpending",
    "sigsuspend",
    "sigaltstack",
    "raise",
]
FORBIDDEN = re.compile(r"(?<!qos_)\b(" + "|".join(NAMES) + r")\s*\(")


def test_c_libc_does_not_delegate_core_calls_to_host_glibc() -> None:
    text = LIBC_C.read_text(encoding="utf-8")
    offenders = sorted(set(m.group(1) for m in FORBIDDEN.finditer(text)))
    assert offenders == [], f"direct host libc delegation found: {offenders}"
