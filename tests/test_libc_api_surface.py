from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

C_LIBC = ROOT / "c-os/libc/libc.c"
RUST_LIBC = ROOT / "rust-os/libc/src/lib.rs"


REQUIRED = [
    # Memory
    "qos_malloc",
    "qos_free",
    "qos_realloc",
    # String
    "qos_memcpy",
    "qos_memset",
    "qos_memmove",
    "qos_strlen",
    "qos_strcpy",
    "qos_strncpy",
    "qos_strcmp",
    "qos_strncmp",
    "qos_strchr",
    "qos_strrchr",
    "qos_strtok",
    "qos_atoi",
    "qos_snprintf",
    "qos_vsnprintf",
    # I/O
    "qos_printf",
    "qos_putchar",
    "qos_puts",
    "qos_getchar",
    "qos_fgets",
    "qos_fputs",
    # File
    "qos_fopen",
    "qos_fclose",
    "qos_fread",
    "qos_fwrite",
    "qos_fseek",
    "qos_ftell",
    "qos_fflush",
    "qos_fileno",
    # Process
    "qos_exit",
    "qos__exit",
    "qos_getpid",
    "qos_fork",
    "qos_execv",
    "qos_execve",
    "qos_waitpid",
    "qos_abort",
    # Network
    "qos_socket",
    "qos_bind",
    "qos_listen",
    "qos_accept",
    "qos_connect",
    "qos_send",
    "qos_recv",
    "qos_sendto",
    "qos_recvfrom",
    "qos_close",
    "qos_htons",
    "qos_htonl",
    "qos_ntohs",
    "qos_ntohl",
    "qos_inet_addr",
    "qos_inet_ntoa",
    # Signals
    "qos_kill",
    "qos_signal",
    "qos_sigaction",
    "qos_sigprocmask",
    "qos_sigpending",
    "qos_sigsuspend",
    "qos_sigaltstack",
    "qos_raise",
    # Error
    "qos_errno_set",
    "qos_errno_get",
    "qos_strerror",
    "qos_perror",
]


def _has_symbol(text: str, name: str) -> bool:
    return re.search(rf"\b{name}\b", text) is not None


def test_c_libc_exports_required_api_surface() -> None:
    text = C_LIBC.read_text(encoding="utf-8")
    missing = [name for name in REQUIRED if not _has_symbol(text, name)]
    assert missing == [], f"C libc missing symbols: {missing}"


def test_rust_libc_exports_required_api_surface() -> None:
    text = RUST_LIBC.read_text(encoding="utf-8")
    missing = [name for name in REQUIRED if not _has_symbol(text, name)]
    assert missing == [], f"Rust libc missing symbols: {missing}"
