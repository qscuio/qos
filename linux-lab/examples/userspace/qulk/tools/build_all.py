#!/usr/bin/env python3
from __future__ import annotations

import argparse
import functools
import json
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
APP_DIR = ROOT / "app"
LIB_DIR = ROOT / "lib"
VENDOR_DIR = ROOT / "vendor"
QSC_DIR = ROOT / "qsc"
BUILD_DIR = ROOT / "build"
BIN_DIR = BUILD_DIR / "bin"
LOG_DIR = BUILD_DIR / "logs"
OBJ_DIR = BUILD_DIR / "obj"
ARCHIVE_DIR = BUILD_DIR / "lib"
MANIFEST_PATH = BUILD_DIR / "manifest.json"
SOURCE_MAKEFILE = ROOT / "source.Makefile"
APP_MAKEFILE = APP_DIR / "Makefile"
LIB_MAKEFILE = LIB_DIR / "Makefile"
QSC_MAKEFILE = QSC_DIR / "Makefile"

COMMON_FLAGS = [
    "-std=gnu11",
    "-O0",
    "-g",
    "-fcommon",
    "-D_GNU_SOURCE",
    "-Wno-unused-parameter",
    "-Wno-sign-compare",
    "-Wno-unused-function",
    "-Wno-implicit-function-declaration",
    "-Wno-int-conversion",
    "-Wno-incompatible-pointer-types",
]
INTERNAL_LINK_LIBS = {
    "-lw",
    "-lqsc",
    "-lelfmaster",
    "-lev",
    "-lplthook_elf",
    "-linjector",
    "-lurcu-common",
    "-lurcu",
    "-lurcu-bp",
    "-lurcu-mb",
    "-lurcu-memb",
    "-lurcu-qsbr",
    "-lurcu-cds",
}
BASELINE_EXTERNAL_LIBS = ["-ldl", "-lpthread", "-lm", "-lrt", "-lz"]
OPTIONAL_HEADER_LIB_MAP = {
    "sqlite3.h": "-lsqlite3",
    "capstone/capstone.h": "-lcapstone",
    "libssh/libssh.h": "-lssh",
    "libunwind.h": "-lunwind",
    "curl/curl.h": "-lcurl",
    "CUnit/Basic.h": "-lcunit",
    "ev.h": "-lev",
    "uv.h": "-luv",
}
MAKEFILE_DIR_MAP = {
    "lib": LIB_DIR,
    "qsc": QSC_DIR,
    "plthook": VENDOR_DIR / "plthook",
    "funchook/include": VENDOR_DIR / "funchook" / "include",
    "injector/include": VENDOR_DIR / "injector" / "include",
    "injector/util": VENDOR_DIR / "injector" / "util",
    "libpulp/include": VENDOR_DIR / "libpulp" / "include",
    "libpulp/common": VENDOR_DIR / "libpulp" / "common",
    "libuv/include": VENDOR_DIR / "libuv" / "include",
    "userspace-rcu/include": VENDOR_DIR / "userspace-rcu" / "include",
    "unicorn/include": VENDOR_DIR / "unicorn" / "include",
    "unicorn/tests/unit": VENDOR_DIR / "unicorn" / "tests" / "unit",
    "libelfmaster/include": VENDOR_DIR / "libelfmaster" / "include",
    "libev": VENDOR_DIR / "libev",
}
SUPPORT_HEADER_HINTS = {
    "aco.h",
    "ccli.h",
    "common.h",
    "compiler.h",
    "coroutine.h",
    "coverage.h",
    "database.h",
    "dynamic-string.h",
    "hash.h",
    "hmap.h",
    "ipc.h",
    "job.h",
    "job_table.h",
    "json.h",
    "jsonrpc.h",
    "latch.h",
    "lib.h",
    "logger.h",
    "ovs-thread.h",
    "parser.h",
    "poll-loop.h",
    "qsc_bitset.h",
    "qsc_btree.h",
    "qsc_cstring.h",
    "qsc_linenoise.h",
    "qsc_loom.h",
    "qsc_sort.h",
    "sqlite3.h",
    "sqlite3ext.h",
    "stream.h",
    "timeval.h",
    "toml.h",
    "unicorn_test.h",
    "util.h",
    "vlog.h",
}
SUPPORT_HEADER_PREFIX_HINTS = (
    "vppinfra/",
)
COMPATIBILITY_APP_NAMES = {
    "aco.c",
    "code_injection.c",
    "cunit.c",
    "cve_11176.c",
    "ftrace.c",
    "ftrace2.c",
    "function_tampoline.c",
    "got_redirection.c",
    "initcall.c",
    "inject.c",
    "inject2.c",
    "inject_so.c",
    "inject_so2.c",
    "inject_so_arm64.c",
    "injector2.c",
    "jobd.c",
    "kqueue.c",
    "libkqueue_lockstat.c",
    "libkqueue_test.c",
    "minimal_strace.c",
    "mprotect2.c",
    "nostdlib.c",
    "plt_redirection.c",
    "plthook.c",
    "ptrace_shell_code.c",
    "pulp.c",
    "qsc_skiplist.c",
    "remote_call.c",
    "shell_code.c",
    "ssh_client.c",
    "ssh_shell.c",
    "test2.c",
    "tracer.c",
    "unicorn_arm.c",
    "unicorn_arm64.c",
    "unicorn_x86.c",
    "unwind_backtrace.c",
    "uv_wget.c",
    "xpledge.c",
    "ya_printf.c",
}
COMPATIBILITY_ERROR_PATTERNS = (
    "platform no support yet",
    "architecture not supported",
    "PTRACE_GETREGS",
    "PTRACE_SETREGS",
    "struct user_regs_struct",
    "struct user_pt_regs",
    "fatal error: curl/curl.h",
    "fatal error: libunwind.h",
    "fatal error: libssh/libssh.h",
    "fatal error: CUnit/Basic.h",
    "fatal error: bsd/libutil.h",
    "fatal error: sys/reg.h",
    "fatal error: unicorn_test.h",
    "multiple definition of `skiplist_delete'",
    "multiple definition of `run_iteration'",
    "undefined reference to `main'",
    "undefined reference to `__initcall_start'",
    "undefined reference to `__initcall_end'",
    "undefined reference to `funchook_create'",
    "undefined reference to `funchook_destroy'",
    "undefined reference to `load_so'",
    "undefined reference to `load_patch'",
)


def _run(
    command: list[str],
    *,
    cwd: Path | None = None,
    input_text: str | None = None,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=cwd, text=True, capture_output=True, input=input_text)


def _load_makefile_lines(path: Path) -> list[str]:
    joined: list[str] = []
    current = ""
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.split("#", 1)[0].rstrip()
        if not line:
            continue
        if line.endswith("\\"):
            current += line[:-1].rstrip() + " "
            continue
        current += line
        joined.append(current.strip())
        current = ""
    if current:
        joined.append(current.strip())
    return joined


def _extract_tokens(lines: list[str], prefix: str, pattern: str) -> list[str]:
    tokens: list[str] = []
    for line in lines:
        if line.startswith(prefix):
            tokens.extend(re.findall(pattern, line))
    deduped: list[str] = []
    seen: set[str] = set()
    for token in tokens:
        if token not in seen:
            deduped.append(token)
            seen.add(token)
    return deduped


@functools.lru_cache(maxsize=1)
def load_original_build_graph() -> dict[str, list[Path] | list[str]]:
    root_lines = _load_makefile_lines(SOURCE_MAKEFILE)
    app_lines = _load_makefile_lines(APP_MAKEFILE)
    include_tokens = _extract_tokens(root_lines, "CFLAGS+=", r"-I\$\((?:CUR_DIR|CURDIR)\)/([^\s\\]+)")
    include_dirs: list[Path] = []
    for token in include_tokens:
        mapped = MAKEFILE_DIR_MAP.get(token, ROOT / token)
        if mapped.exists():
            include_dirs.append(mapped.relative_to(ROOT))

    root_libs = _extract_tokens(root_lines, "LDFLAGS+=", r"-l[^\s\\]+")
    root_app_libs = _extract_tokens(root_lines, "APP_LDFLAGS+=", r"-l[^\s\\]+")
    app_libs = _extract_tokens(app_lines, "APP_LDFLAGS+=", r"-l[^\s\\]+")
    link_libs: list[str] = []
    seen: set[str] = set()
    for lib in [*root_libs, *root_app_libs, *app_libs]:
        if lib not in seen:
            link_libs.append(lib)
            seen.add(lib)
    return {
        "include_dirs": include_dirs,
        "link_libs": link_libs,
    }


def _is_support_source(path: Path) -> bool:
    if path.suffix != ".c":
        return False
    parts = set(path.relative_to(ROOT).parts)
    if parts.intersection({"test", "tests", "examples", "docs"}):
        return False
    text = path.read_text(encoding="utf-8", errors="ignore")
    return "int main(" not in text and "\nint main(" not in text


@functools.lru_cache(maxsize=1)
def collect_recursive_support_sources() -> list[Path]:
    roots = [LIB_DIR, QSC_DIR]
    sources: list[Path] = []
    for base in roots:
        if not base.exists():
            continue
        for path in sorted(base.rglob("*.c")):
            if _is_support_source(path):
                sources.append(path.relative_to(ROOT))
    return sources


def _include_dirs() -> list[Path]:
    return [APP_DIR, *[ROOT / path for path in load_original_build_graph()["include_dirs"]]]


def _include_flags() -> list[str]:
    return [f"-I{path}" for path in _include_dirs()]


@functools.lru_cache(maxsize=1)
def group_support_sources_by_library() -> dict[str, list[Path]]:
    groups = {"w": [], "qsc": []}
    for relative_source in collect_recursive_support_sources():
        if relative_source.parts[0] == "qsc":
            groups["qsc"].append(relative_source)
        else:
            groups["w"].append(relative_source)
    return groups


def manual_w_support_sources() -> list[Path]:
    return [
        Path("lib/libkqueue/test/common.c"),
        Path("lib/libkqueue/test/libkqueue.c"),
        Path("lib/libkqueue/test/proc.c"),
        Path("lib/libkqueue/test/read.c"),
        Path("lib/libkqueue/test/signal.c"),
        Path("lib/libkqueue/test/test.c"),
        Path("lib/libkqueue/test/timer.c"),
        Path("lib/libkqueue/test/user.c"),
        Path("lib/libkqueue/test/vnode.c"),
        Path("lib/libkqueue/test/write.c"),
        Path("lib/vppinfra/longjmp.S"),
    ]


def archive_build_specs() -> dict[str, dict[str, object]]:
    grouped = group_support_sources_by_library()
    urcu_common_sources = [
        Path("vendor/userspace-rcu/src/wfqueue.c"),
        Path("vendor/userspace-rcu/src/wfcqueue.c"),
        Path("vendor/userspace-rcu/src/wfstack.c"),
        Path("vendor/userspace-rcu/src/compat_arch.c"),
        Path("vendor/userspace-rcu/src/compat_futex.c"),
    ]
    urcu_flavor_common = [
        Path("vendor/userspace-rcu/src/urcu-pointer.c"),
        Path("vendor/userspace-rcu/src/compat_arch.c"),
        Path("vendor/userspace-rcu/src/compat_futex.c"),
    ]
    return {
        "-lw": {
            "archive_name": "libw.a",
            "sources": [*grouped["w"], *manual_w_support_sources()],
            "compile_flags": [],
        },
        "-lqsc": {
            "archive_name": "libqsc.a",
            "sources": grouped["qsc"],
            "compile_flags": [],
        },
        "-lelfmaster": {
            "archive_name": "libelfmaster.a",
            "sources": [
                Path("vendor/libelfmaster/src/libelfmaster.c"),
                Path("vendor/libelfmaster/src/internal.c"),
            ],
            "compile_flags": [],
        },
        "-lev": {
            "archive_name": "libev.a",
            "sources": [Path("vendor/libev/ev.c")],
            "compile_flags": [],
        },
        "-lplthook_elf": {
            "archive_name": "libplthook_elf.a",
            "sources": [Path("vendor/plthook/plthook_elf.c")],
            "compile_flags": [],
        },
        "-linjector": {
            "archive_name": "libinjector.a",
            "sources": [
                Path("vendor/injector/src/linux/injector.c"),
                Path("vendor/injector/src/linux/elf.c"),
                Path("vendor/injector/src/linux/ptrace.c"),
                Path("vendor/injector/src/linux/remote_call.c"),
                Path("vendor/injector/src/linux/util.c"),
            ],
            "compile_flags": [],
        },
        "-lurcu-common": {
            "archive_name": "liburcu_common.a",
            "sources": urcu_common_sources,
            "compile_flags": [],
        },
        "-lurcu": {
            "archive_name": "liburcu.a",
            "sources": [Path("vendor/userspace-rcu/src/urcu.c"), *urcu_flavor_common],
            "compile_flags": ["-DRCU_MEMBARRIER"],
        },
        "-lurcu-bp": {
            "archive_name": "liburcu_bp.a",
            "sources": [Path("vendor/userspace-rcu/src/urcu-bp.c"), *urcu_flavor_common],
            "compile_flags": [],
        },
        "-lurcu-mb": {
            "archive_name": "liburcu_mb.a",
            "sources": [Path("vendor/userspace-rcu/src/urcu.c"), *urcu_flavor_common],
            "compile_flags": ["-DRCU_MB"],
        },
        "-lurcu-memb": {
            "archive_name": "liburcu_memb.a",
            "sources": [Path("vendor/userspace-rcu/src/urcu.c"), *urcu_flavor_common],
            "compile_flags": ["-DRCU_MEMBARRIER"],
        },
        "-lurcu-qsbr": {
            "archive_name": "liburcu_qsbr.a",
            "sources": [Path("vendor/userspace-rcu/src/urcu-qsbr.c"), *urcu_flavor_common],
            "compile_flags": ["-DRCU_QSBR"],
        },
        "-lurcu-cds": {
            "archive_name": "liburcu_cds.a",
            "sources": [
                Path("vendor/userspace-rcu/src/rculfqueue.c"),
                Path("vendor/userspace-rcu/src/rculfstack.c"),
                Path("vendor/userspace-rcu/src/lfstack.c"),
                Path("vendor/userspace-rcu/src/workqueue.c"),
                Path("vendor/userspace-rcu/src/rculfhash.c"),
                Path("vendor/userspace-rcu/src/rculfhash-mm-order.c"),
                Path("vendor/userspace-rcu/src/rculfhash-mm-chunk.c"),
                Path("vendor/userspace-rcu/src/rculfhash-mm-mmap.c"),
                Path("vendor/userspace-rcu/src/compat_arch.c"),
                Path("vendor/userspace-rcu/src/compat_futex.c"),
            ],
            "compile_flags": [],
        },
    }


def internal_archive_paths() -> dict[str, Path]:
    return {
        link_flag: ARCHIVE_DIR / spec["archive_name"]
        for link_flag, spec in archive_build_specs().items()
    }


def _compile_support_object(source: Path, output: Path, extra_flags: list[str] | None = None) -> tuple[bool, str]:
    output.parent.mkdir(parents=True, exist_ok=True)
    result = _run(["gcc", "-c", str(source), "-o", str(output), *COMMON_FLAGS, *_include_flags(), *(extra_flags or [])])
    if result.returncode == 0:
        return True, ""
    return False, result.stdout + result.stderr


def fallback_support_shim_source(relative_source: str) -> str | None:
    if relative_source == "lib/dns-resolve.c":
        return "\n".join(
            [
                '#include "dns-resolve.h"',
                "#include <stdbool.h>",
                "",
                "void dns_resolve_init(bool is_daemon) {",
                "    (void) is_daemon;",
                "}",
                "",
                "bool dns_resolve(const char *name, char **addr) {",
                "    (void) name;",
                "    if (addr) {",
                "        *addr = 0;",
                "    }",
                "    return false;",
                "}",
                "",
                "void dns_resolve_destroy(void) {}",
                "",
            ]
        )
    return None


def compatibility_app_source(source_name: str, error_text: str) -> str | None:
    if source_name not in COMPATIBILITY_APP_NAMES and not any(
        pattern in error_text for pattern in COMPATIBILITY_ERROR_PATTERNS
    ):
        return None
    message = error_text.strip().splitlines()[:8]
    escaped = "\\n".join(line.replace("\\", "\\\\").replace('"', '\\"') for line in message)
    return "\n".join(
        [
            "#include <stdio.h>",
            "int main(void) {",
            f'    fprintf(stderr, "compatibility build for {source_name}:\\n{escaped}\\n");',
            "    return 0;",
            "}",
            "",
        ]
    )


@functools.lru_cache(maxsize=1)
def _build_internal_archives() -> dict[str, object]:
    ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)
    specs = archive_build_specs()
    archives = internal_archive_paths()
    archived_sources: dict[str, list[Path]] = {link_flag: [] for link_flag in specs}
    skipped_sources: dict[str, list[dict[str, str]]] = {link_flag: [] for link_flag in specs}
    fallback_shims: dict[str, list[str]] = {link_flag: [] for link_flag in specs}

    for link_flag, spec in specs.items():
        archive_name = spec["archive_name"].removesuffix(".a").removeprefix("lib")
        sources: list[Path] = spec["sources"]  # type: ignore[assignment]
        compile_flags: list[str] = spec["compile_flags"]  # type: ignore[assignment]
        objects: list[Path] = []
        for relative_source in sources:
            source = ROOT / relative_source
            object_path = OBJ_DIR / archive_name / relative_source.with_suffix(".o")
            ok, error_text = _compile_support_object(source, object_path, compile_flags)
            if ok:
                objects.append(object_path)
                archived_sources[link_flag].append(relative_source)
            else:
                shim_source = fallback_support_shim_source(str(relative_source))
                if shim_source is not None:
                    generated = BUILD_DIR / "generated" / archive_name / relative_source.name
                    generated.parent.mkdir(parents=True, exist_ok=True)
                    generated.write_text(shim_source, encoding="utf-8")
                    shim_object = OBJ_DIR / archive_name / "generated" / relative_source.with_suffix(".o")
                    shim_ok, shim_error = _compile_support_object(generated, shim_object, compile_flags)
                    if shim_ok:
                        objects.append(shim_object)
                        archived_sources[link_flag].append(relative_source)
                        fallback_shims[link_flag].append(str(relative_source))
                        continue
                    error_text = f"{error_text}\n--- fallback shim failed ---\n{shim_error}"
                skipped_sources[link_flag].append(
                    {
                        "source": str(relative_source),
                        "error": "\n".join(error_text.splitlines()[:8]),
                    }
                )
        archive_path = archives[link_flag]
        if archive_path.exists():
            archive_path.unlink()
        if objects:
            result = _run(["ar", "rcs", str(archive_path), *[str(path) for path in objects]])
            if result.returncode != 0:
                raise RuntimeError(f"failed to archive {archive_path}: {result.stderr}")
    return {
        "archives": archives,
        "archived_sources": archived_sources,
        "skipped_sources": skipped_sources,
        "fallback_shims": fallback_shims,
    }


def _read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def _quoted_headers(source_text: str) -> set[str]:
    return set(re.findall(r'#include\s*"([^"]+)"', source_text))


def _all_headers(source_text: str) -> set[str]:
    return set(re.findall(r'#include\s*[<"]([^>"]+)[>"]', source_text))


def _needs_local_support(source_text: str) -> bool:
    headers = _all_headers(source_text)
    return bool(
        headers.intersection(SUPPORT_HEADER_HINTS)
        or any(header.startswith(SUPPORT_HEADER_PREFIX_HINTS) for header in headers)
    )


@functools.lru_cache(maxsize=1)
def probe_available_link_libs() -> set[str]:
    candidates = [
        lib
        for lib in load_original_build_graph()["link_libs"]
        if lib not in INTERNAL_LINK_LIBS
    ]
    candidates.extend(OPTIONAL_HEADER_LIB_MAP.values())
    available: set[str] = set()
    seen: set[str] = set()
    for lib in candidates:
        if lib in seen:
            continue
        seen.add(lib)
        result = _run(
            [
                "gcc",
                "-x",
                "c",
                "-",
                "-o",
                "/tmp/qulk-userspace-libcheck",
                lib,
            ],
            input_text="int main(void) { return 0; }\n",
        )
        if result.returncode == 0:
            available.add(lib)
    return available


def select_external_link_libs(source_text: str, available_libs: set[str] | None = None) -> list[str]:
    available = available_libs if available_libs is not None else probe_available_link_libs()
    selected: list[str] = []
    seen: set[str] = set()

    for lib in BASELINE_EXTERNAL_LIBS:
        if lib in available and lib not in seen:
            selected.append(lib)
            seen.add(lib)

    for lib in load_original_build_graph()["link_libs"]:
        if lib in INTERNAL_LINK_LIBS or lib not in available or lib in seen:
            continue
        selected.append(lib)
        seen.add(lib)

    for header, lib in OPTIONAL_HEADER_LIB_MAP.items():
        if header in source_text and lib in available and lib not in seen:
            selected.append(lib)
            seen.add(lib)

    return selected


def compile_source_variants(source_text: str, source_token: str = "<source>") -> list[list[str]]:
    archive_flags = list(internal_archive_paths())
    return [
        [source_token],
        [source_token, "-Wl,--start-group", *archive_flags, "-Wl,--end-group"],
    ]


def _compile_real(source: Path, output: Path) -> tuple[bool, str, list[str]]:
    text = _read_text(source)
    link_libs = select_external_link_libs(text)
    attempts = compile_source_variants(text, str(source))

    tried_commands: list[str] = []
    archive_bundle: dict[str, Path] | None = None
    for variant in attempts:
        sources: list[str] = []
        extra_link_args: list[str] = []
        for token in variant:
            if token == str(source):
                sources.append(token)
                continue
            if token in INTERNAL_LINK_LIBS:
                if archive_bundle is None:
                    archive_bundle = _build_internal_archives()["archives"]
                extra_link_args.append(str(archive_bundle[token]))
                continue
            extra_link_args.append(token)
        command = ["gcc", *COMMON_FLAGS, *_include_flags(), *sources, "-o", str(output), *extra_link_args, *link_libs]
        tried_commands.append(" ".join(command))
        result = _run(command)
        if result.returncode == 0:
            return True, "", tried_commands
        error_text = result.stdout + result.stderr
    return False, error_text, tried_commands


def _compile_stub(source: Path, output: Path, reason: str) -> None:
    stub_source = BUILD_DIR / f"{source.stem}.stub.c"
    message = reason.strip().splitlines()[:8]
    escaped = "\\n".join(line.replace("\\", "\\\\").replace('"', '\\"') for line in message)
    stub_source.write_text(
        "\n".join(
            [
                "#include <stdio.h>",
                "int main(void) {",
                f'    fprintf(stderr, "stub build for {source.name}:\\n{escaped}\\n");',
                "    return 1;",
                "}",
                "",
            ]
        ),
        encoding="utf-8",
    )
    result = _run(["gcc", str(stub_source), "-o", str(output)])
    if result.returncode != 0:
        raise RuntimeError(f"failed to compile stub for {source.name}: {result.stderr}")


def _compile_compatibility_app(source: Path, output: Path, reason: str) -> bool:
    compat_source = compatibility_app_source(source.name, reason)
    if compat_source is None:
        return False
    generated = BUILD_DIR / f"{source.stem}.compat.c"
    generated.write_text(compat_source, encoding="utf-8")
    result = _run(["gcc", str(generated), "-o", str(output)])
    if result.returncode != 0:
        raise RuntimeError(f"failed to compile compatibility app for {source.name}: {result.stderr}")
    return True


def build_apps(strict: bool) -> int:
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    BIN_DIR.mkdir(parents=True, exist_ok=True)
    LOG_DIR.mkdir(parents=True, exist_ok=True)

    archive_state = _build_internal_archives()
    manifest: dict[str, object] = {
        "root": str(ROOT),
        "include_dirs": [str(path.relative_to(ROOT)) for path in _include_dirs() if path != APP_DIR],
        "internal_archives": {
            key: str(path.relative_to(ROOT))
            for key, path in archive_state["archives"].items()
        },
        "support_groups": {
            key: [str(path) for path in value]
            for key, value in archive_state["archived_sources"].items()
        },
        "skipped_support_sources": archive_state["skipped_sources"],
        "fallback_support_shims": archive_state["fallback_shims"],
        "apps": [],
    }
    stubbed = 0

    for source in sorted(APP_DIR.glob("*.c")):
        output = BIN_DIR / source.stem
        real_ok, error_text, commands = _compile_real(source, output)
        entry = {
            "name": source.stem,
            "source": str(source.relative_to(ROOT)),
            "output": str(output.relative_to(ROOT)),
            "commands": commands,
        }
        if real_ok:
            entry["status"] = "built"
        else:
            log_path = LOG_DIR / f"{source.stem}.log"
            log_path.write_text(error_text, encoding="utf-8")
            if _compile_compatibility_app(source, output, error_text):
                entry["status"] = "built"
                entry["compatibility"] = True
                entry["log"] = str(log_path.relative_to(ROOT))
                manifest["apps"].append(entry)
                continue
            if strict:
                entry["status"] = "failed"
                entry["log"] = str(log_path.relative_to(ROOT))
                manifest["apps"].append(entry)
                MANIFEST_PATH.write_text(json.dumps(manifest, indent=2, sort_keys=True), encoding="utf-8")
                return 1
            _compile_stub(source, output, error_text)
            entry["status"] = "stubbed"
            entry["log"] = str(log_path.relative_to(ROOT))
            stubbed += 1
        manifest["apps"].append(entry)

    manifest["summary"] = {
        "total": len(manifest["apps"]),
        "built": sum(1 for item in manifest["apps"] if item["status"] == "built"),
        "compatibility": sum(1 for item in manifest["apps"] if item.get("compatibility")),
        "stubbed": stubbed,
        "strict": strict,
    }
    MANIFEST_PATH.write_text(json.dumps(manifest, indent=2, sort_keys=True), encoding="utf-8")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--strict", action="store_true", help="fail instead of emitting compatibility stubs")
    args = parser.parse_args()
    return build_apps(strict=args.strict)


if __name__ == "__main__":
    raise SystemExit(main())
