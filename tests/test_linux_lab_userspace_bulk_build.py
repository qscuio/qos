from __future__ import annotations

import importlib.util
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "linux-lab" / "examples" / "userspace" / "qulk" / "tools" / "build_all.py"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_original_makefiles_drive_include_and_link_surface() -> None:
    module = _load_module("linux_lab_userspace_bulk_build", MODULE_PATH)
    graph = module.load_original_build_graph()

    include_dirs = {path.as_posix() for path in graph["include_dirs"]}
    assert "qsc" in include_dirs
    assert "lib/libkqueue/include" in include_dirs
    assert "lib/iperf/src" in include_dirs

    link_libs = graph["link_libs"]
    for lib in (
        "-lw",
        "-lqsc",
        "-lplthook_elf",
        "-lfunchook",
        "-linjector",
        "-lpulp",
        "-lunbound",
        "-lssl",
        "-lcrypto",
        "-lcunit",
        "-lunwind",
        "-lcurl",
        "-lbsd",
        "-lelf",
        "-ldw",
        "-lcapstone",
    ):
        assert lib in link_libs


def test_recursive_support_source_scan_matches_original_lib_makefile() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_recursive", MODULE_PATH)
    sources = {path.as_posix() for path in module.collect_recursive_support_sources()}

    assert "lib/frr/linklist.c" in sources
    assert "lib/vppinfra/unformat.c" in sources
    assert "lib/libkqueue/src/common/map.c" in sources
    assert "qsc/qsc_hash.c" in sources


def test_internal_support_sources_are_grouped_by_original_library_boundary() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_groups", MODULE_PATH)
    groups = module.group_support_sources_by_library()

    assert set(groups) == {"w", "qsc"}
    assert "lib/frr/linklist.c" in {path.as_posix() for path in groups["w"]}
    assert "lib/vppinfra/unformat.c" in {path.as_posix() for path in groups["w"]}
    assert "qsc/qsc_hash.c" in {path.as_posix() for path in groups["qsc"]}


def test_internal_archive_paths_follow_original_link_flags() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_archives", MODULE_PATH)
    archives = module.internal_archive_paths()

    assert archives["-lw"].as_posix().endswith("/build/lib/libw.a")
    assert archives["-lqsc"].as_posix().endswith("/build/lib/libqsc.a")
    assert archives["-lelfmaster"].as_posix().endswith("/build/lib/libelfmaster.a")
    assert archives["-lev"].as_posix().endswith("/build/lib/libev.a")
    assert archives["-lplthook_elf"].as_posix().endswith("/build/lib/libplthook_elf.a")
    assert archives["-linjector"].as_posix().endswith("/build/lib/libinjector.a")
    assert archives["-lurcu-common"].as_posix().endswith("/build/lib/liburcu_common.a")
    assert archives["-lurcu"].as_posix().endswith("/build/lib/liburcu.a")
    assert archives["-lurcu-bp"].as_posix().endswith("/build/lib/liburcu_bp.a")
    assert archives["-lurcu-mb"].as_posix().endswith("/build/lib/liburcu_mb.a")
    assert archives["-lurcu-memb"].as_posix().endswith("/build/lib/liburcu_memb.a")
    assert archives["-lurcu-qsbr"].as_posix().endswith("/build/lib/liburcu_qsbr.a")
    assert archives["-lurcu-cds"].as_posix().endswith("/build/lib/liburcu_cds.a")


def test_compile_attempts_always_include_internal_archives_as_fallback() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_attempts", MODULE_PATH)
    attempts = module.compile_source_variants('#include <stdio.h>\nint main(void) { return 0; }\n')

    assert len(attempts) == 2
    assert attempts[0] == ["<source>"]
    assert attempts[1][0] == "<source>"
    assert "-Wl,--start-group" in attempts[1]
    assert attempts[1][-1] == "-Wl,--end-group"


def test_archive_build_specs_cover_vendor_archives_and_flavor_specific_rcu_flags() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_archive_specs", MODULE_PATH)
    specs = module.archive_build_specs()

    assert "vendor/libev/ev.c" in {path.as_posix() for path in specs["-lev"]["sources"]}
    assert "vendor/plthook/plthook_elf.c" in {path.as_posix() for path in specs["-lplthook_elf"]["sources"]}
    assert "vendor/injector/src/linux/injector.c" in {path.as_posix() for path in specs["-linjector"]["sources"]}
    assert specs["-lurcu"]["compile_flags"] == ["-DRCU_MEMBARRIER"]
    assert specs["-lurcu-memb"]["compile_flags"] == ["-DRCU_MEMBARRIER"]
    assert specs["-lurcu-mb"]["compile_flags"] == ["-DRCU_MB"]
    assert specs["-lurcu-qsbr"]["compile_flags"] == ["-DRCU_QSBR"]


def test_manual_w_support_sources_include_kqueue_test_helpers_and_vpp_longjmp_asm() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_manual_w_support", MODULE_PATH)
    extras = {path.as_posix() for path in module.manual_w_support_sources()}

    assert "lib/libkqueue/test/common.c" in extras
    assert "lib/libkqueue/test/test.c" in extras
    assert "lib/vppinfra/longjmp.S" in extras


def test_external_link_libs_filter_unavailable_root_libs_but_keep_needed_available_ones() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_link_libs", MODULE_PATH)
    source = '#include <stdio.h>\nint main(void) { return 0; }\n'
    selected = module.select_external_link_libs(
        source,
        available_libs={"-lpthread", "-lrt", "-lm", "-lz", "-ldl", "-lssl", "-lcrypto", "-lelf", "-ldw"},
    )

    assert "-lpthread" in selected
    assert "-lrt" in selected
    assert "-lm" in selected
    assert "-lz" in selected
    assert "-ldl" in selected
    assert "-lssl" in selected
    assert "-lcrypto" in selected
    assert "-lelf" in selected
    assert "-ldw" in selected
    assert "-lunbound" not in selected
    assert "-lcunit" not in selected
    assert "-lcurl" not in selected
    assert "-lbsd" not in selected


def test_external_link_libs_add_header_driven_optional_libs_when_available() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_link_hints", MODULE_PATH)
    source = '#include <sqlite3.h>\n#include <capstone/capstone.h>\n#include <libssh/libssh.h>\n'
    selected = module.select_external_link_libs(
        source,
        available_libs={"-lpthread", "-lrt", "-lm", "-lz", "-ldl", "-lsqlite3", "-lcapstone"},
    )

    assert "-lsqlite3" in selected
    assert "-lcapstone" in selected
    assert "-lssh" not in selected


def test_local_support_detection_covers_quoted_and_angle_bracket_imported_headers() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_local_support", MODULE_PATH)

    assert module._needs_local_support('#include "coroutine.h"\n')
    assert module._needs_local_support("#include <vppinfra/format.h>\n")


def test_dns_resolve_missing_source_has_a_fallback_shim() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_dns_shim", MODULE_PATH)
    shim = module.fallback_support_shim_source("lib/dns-resolve.c")

    assert shim is not None
    assert "dns_resolve_init" in shim
    assert "bool dns_resolve" in shim
    assert "dns_resolve_destroy" in shim


def test_compatibility_app_source_covers_missing_sdk_and_unsupported_arch_cases() -> None:
    module = _load_module("linux_lab_userspace_bulk_build_compat_apps", MODULE_PATH)

    curl_compat = module.compatibility_app_source(
        "uv_wget.c",
        "/tmp/uv_wget.c:5:10: fatal error: curl/curl.h: No such file or directory\n",
    )
    assert curl_compat is not None
    assert "compatibility build for uv_wget.c" in curl_compat

    arch_compat = module.compatibility_app_source(
        "inject_so_arm64.c",
        "error: ‘PTRACE_GETREGS’ undeclared\n",
    )
    assert arch_compat is not None
    assert "compatibility build for inject_so_arm64.c" in arch_compat
