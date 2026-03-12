from __future__ import annotations

import ctypes
import socket
import subprocess
import tempfile
import threading
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str], cwd: Path | None = None, input_text: str | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=(cwd or ROOT),
        text=True,
        input=input_text,
        capture_output=True,
    )


def test_c_libc_exports_basic_helpers() -> None:
    src = ROOT / "c-os/libc/libc.c"
    out = ROOT / "c-os/build/libqos_c_libc.so"
    out.parent.mkdir(parents=True, exist_ok=True)

    build = _run(["gcc", "-shared", "-fPIC", "-std=c11", "-O2", "-o", str(out), str(src)])
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}"

    lib = ctypes.CDLL(str(out))
    lib.qos_strlen.argtypes = [ctypes.c_char_p]
    lib.qos_strlen.restype = ctypes.c_size_t
    lib.qos_strcmp.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.qos_strcmp.restype = ctypes.c_int
    lib.qos_memset.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_size_t]
    lib.qos_memset.restype = ctypes.c_void_p
    lib.qos_memcpy.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    lib.qos_memcpy.restype = ctypes.c_void_p
    lib.qos_memcmp.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    lib.qos_memcmp.restype = ctypes.c_int
    lib.qos_memmove.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    lib.qos_memmove.restype = ctypes.c_void_p
    lib.qos_strncmp.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t]
    lib.qos_strncmp.restype = ctypes.c_int
    lib.qos_strcpy.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.qos_strcpy.restype = ctypes.c_char_p
    lib.qos_strncpy.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t]
    lib.qos_strncpy.restype = ctypes.c_char_p
    lib.qos_strchr.argtypes = [ctypes.c_char_p, ctypes.c_int]
    lib.qos_strchr.restype = ctypes.c_void_p
    lib.qos_strrchr.argtypes = [ctypes.c_char_p, ctypes.c_int]
    lib.qos_strrchr.restype = ctypes.c_void_p
    lib.qos_atoi.argtypes = [ctypes.c_char_p]
    lib.qos_atoi.restype = ctypes.c_int
    lib.qos_htons.argtypes = [ctypes.c_uint16]
    lib.qos_htons.restype = ctypes.c_uint16
    lib.qos_htonl.argtypes = [ctypes.c_uint32]
    lib.qos_htonl.restype = ctypes.c_uint32
    lib.qos_ntohs.argtypes = [ctypes.c_uint16]
    lib.qos_ntohs.restype = ctypes.c_uint16
    lib.qos_ntohl.argtypes = [ctypes.c_uint32]
    lib.qos_ntohl.restype = ctypes.c_uint32
    lib.qos_inet_addr.argtypes = [ctypes.c_char_p]
    lib.qos_inet_addr.restype = ctypes.c_uint32
    lib.qos_inet_ntoa.argtypes = [ctypes.c_uint32, ctypes.c_char_p, ctypes.c_size_t]
    lib.qos_inet_ntoa.restype = ctypes.c_char_p
    lib.qos_errno_set.argtypes = [ctypes.c_int]
    lib.qos_errno_set.restype = None
    lib.qos_errno_get.argtypes = []
    lib.qos_errno_get.restype = ctypes.c_int
    lib.qos_strerror.argtypes = [ctypes.c_int]
    lib.qos_strerror.restype = ctypes.c_char_p
    lib.qos_getpid.argtypes = []
    lib.qos_getpid.restype = ctypes.c_int
    assert hasattr(lib, "qos_vsnprintf")
    lib.qos_malloc.argtypes = [ctypes.c_size_t]
    lib.qos_malloc.restype = ctypes.c_void_p
    lib.qos_realloc.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    lib.qos_realloc.restype = ctypes.c_void_p
    lib.qos_free.argtypes = [ctypes.c_void_p]
    lib.qos_free.restype = None
    lib.qos_strtok.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.qos_strtok.restype = ctypes.c_char_p
    lib.qos_socket.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
    lib.qos_socket.restype = ctypes.c_int
    lib.qos_close.argtypes = [ctypes.c_int]
    lib.qos_close.restype = ctypes.c_int
    lib.qos_fopen.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.qos_fopen.restype = ctypes.c_void_p
    lib.qos_fwrite.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t, ctypes.c_void_p]
    lib.qos_fwrite.restype = ctypes.c_size_t
    lib.qos_fflush.argtypes = [ctypes.c_void_p]
    lib.qos_fflush.restype = ctypes.c_int
    lib.qos_fclose.argtypes = [ctypes.c_void_p]
    lib.qos_fclose.restype = ctypes.c_int

    for symbol in (
        "qos_printf",
        "qos_putchar",
        "qos_puts",
        "qos_getchar",
        "qos_fgets",
        "qos_fputs",
        "qos_fread",
        "qos_fseek",
        "qos_ftell",
        "qos_fileno",
        "qos_exit",
        "qos__exit",
        "qos_fork",
        "qos_execv",
        "qos_execve",
        "qos_waitpid",
        "qos_abort",
        "qos_bind",
        "qos_listen",
        "qos_accept",
        "qos_connect",
        "qos_send",
        "qos_recv",
        "qos_sendto",
        "qos_recvfrom",
        "qos_kill",
        "qos_signal",
        "qos_sigaction",
        "qos_sigprocmask",
        "qos_sigpending",
        "qos_sigsuspend",
        "qos_sigaltstack",
        "qos_raise",
    ):
        assert hasattr(lib, symbol), f"missing export: {symbol}"

    assert lib.qos_strlen(b"qos") == 3
    assert lib.qos_strcmp(b"abc", b"abc") == 0
    assert lib.qos_strcmp(b"abc", b"abd") < 0
    assert lib.qos_strcmp(b"abd", b"abc") > 0

    buf = (ctypes.c_ubyte * 8)()
    lib.qos_memset(buf, 0xAB, 8)
    assert list(buf) == [0xAB] * 8

    src_buf = (ctypes.c_ubyte * 8)(1, 2, 3, 4, 5, 6, 7, 8)
    lib.qos_memcpy(buf, src_buf, 8)
    assert list(buf) == [1, 2, 3, 4, 5, 6, 7, 8]
    assert lib.qos_memcmp(buf, src_buf, 8) == 0

    other_buf = (ctypes.c_ubyte * 8)(1, 2, 3, 4, 5, 6, 7, 9)
    assert lib.qos_memcmp(buf, other_buf, 8) < 0

    overlap = (ctypes.c_ubyte * 8)(1, 2, 3, 4, 5, 6, 7, 8)
    base_addr = ctypes.addressof(overlap)
    lib.qos_memmove(ctypes.c_void_p(base_addr + 2), ctypes.c_void_p(base_addr), 6)
    assert list(overlap) == [1, 2, 1, 2, 3, 4, 5, 6]

    assert lib.qos_strncmp(b"qos-shell", b"qos-sched", 5) == 0
    assert lib.qos_strncmp(b"abc", b"abd", 3) < 0

    dst = ctypes.create_string_buffer(16)
    lib.qos_strcpy(dst, ctypes.c_char_p(b"hello"))
    assert dst.value == b"hello"

    dst2 = ctypes.create_string_buffer(8)
    lib.qos_strncpy(dst2, ctypes.c_char_p(b"alphabet"), 4)
    assert dst2.raw[:5] == b"alph\x00"

    untouched = ctypes.create_string_buffer(b"XYZ")
    lib.qos_strncpy(untouched, ctypes.c_char_p(b"abc"), 0)
    assert untouched.raw[:3] == b"XYZ"

    sample = ctypes.create_string_buffer(b"qos-shell")
    base = ctypes.addressof(sample)
    p_first = lib.qos_strchr(sample, ord("s"))
    p_last = lib.qos_strrchr(sample, ord("s"))
    assert p_first != 0
    assert p_last != 0
    assert p_first - base == 2
    assert p_last - base == 4

    assert lib.qos_atoi(b"0") == 0
    assert lib.qos_atoi(b"42") == 42
    assert lib.qos_atoi(b"-17") == -17
    assert lib.qos_atoi(b"  9abc") == 9

    assert lib.qos_htons(0x1234) == 0x3412
    assert lib.qos_ntohs(0x3412) == 0x1234
    assert lib.qos_htonl(0x01020304) == 0x04030201
    assert lib.qos_ntohl(0x04030201) == 0x01020304

    assert lib.qos_inet_addr(b"10.0.2.15") == 0x0A00020F
    assert lib.qos_inet_addr(b"300.0.0.1") == 0xFFFFFFFF
    ipbuf = ctypes.create_string_buffer(32)
    assert lib.qos_inet_ntoa(0xC0A80101, ipbuf, 32) is not None
    assert ipbuf.value == b"192.168.1.1"
    tiny = ctypes.create_string_buffer(4)
    assert lib.qos_inet_ntoa(0xC0A80101, tiny, 4) is None

    lib.qos_errno_set(111)
    assert lib.qos_errno_get() == 111
    assert b"Connection refused" in lib.qos_strerror(111)

    # errno should be thread-local: worker writes must not clobber main thread.
    lib.qos_errno_set(7)
    thread_vals: list[int] = []

    def _errno_worker(v: int) -> None:
        lib.qos_errno_set(v)
        thread_vals.append(int(lib.qos_errno_get()))

    t1 = threading.Thread(target=_errno_worker, args=(11,))
    t2 = threading.Thread(target=_errno_worker, args=(22,))
    t1.start()
    t2.start()
    t1.join()
    t2.join()
    assert sorted(thread_vals) == [11, 22]
    assert lib.qos_errno_get() == 7

    snbuf = ctypes.create_string_buffer(32)
    assert lib.qos_snprintf(snbuf, 32, b"pid=%d", ctypes.c_int(7)) == 5
    assert snbuf.value == b"pid=7"

    assert lib.qos_getpid() > 0

    ptr = lib.qos_malloc(16)
    assert ptr
    ptr = lib.qos_realloc(ptr, 64)
    assert ptr
    lib.qos_free(ptr)

    tokens = ctypes.create_string_buffer(b"aa,bb,cc")
    assert lib.qos_strtok(tokens, b",") == b"aa"
    assert lib.qos_strtok(None, b",") == b"bb"
    assert lib.qos_strtok(None, b",") == b"cc"
    assert lib.qos_strtok(None, b",") is None

    sockfd = lib.qos_socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    assert sockfd >= 0
    assert lib.qos_close(sockfd) == 0

    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        path_str = tmp.name
    path = path_str.encode("utf-8")
    filep = lib.qos_fopen(path, b"wb")
    assert filep
    payload = ctypes.create_string_buffer(b"qos")
    assert lib.qos_fwrite(payload, 1, 3, filep) == 3
    assert lib.qos_fflush(filep) == 0
    assert lib.qos_fclose(filep) == 0
    Path(path_str).unlink(missing_ok=True)


def test_c_shell_help_and_exit() -> None:
    src = ROOT / "c-os/userspace/shell.c"
    out = ROOT / "c-os/build/qos-sh-c"
    out.parent.mkdir(parents=True, exist_ok=True)

    build = _run(["gcc", "-std=c11", "-O2", "-o", str(out), str(src)])
    assert build.returncode == 0, f"gcc failed:\n{build.stderr}"

    run = _run([str(out)], input_text="help\nexit\n")
    assert run.returncode == 0, f"shell failed:\n{run.stderr}\nstdout:\n{run.stdout}"
    text = run.stdout.lower()
    assert "help" in text
    assert "exit" in text


def test_rust_libc_unit_tests_pass() -> None:
    result = _run(["cargo", "test", "--quiet", "-p", "qos-libc"], cwd=ROOT / "rust-os")
    assert result.returncode == 0, f"cargo test failed:\n{result.stderr}\nstdout:\n{result.stdout}"


def test_rust_userspace_once_help() -> None:
    result = _run(
        ["cargo", "run", "--quiet", "-p", "qos-userspace", "--", "--once", "help"],
        cwd=ROOT / "rust-os",
    )
    assert result.returncode == 0, f"cargo run failed:\n{result.stderr}\nstdout:\n{result.stdout}"
    text = result.stdout.lower()
    assert "help" in text
    assert "exit" in text
    assert "placeholder" not in text
