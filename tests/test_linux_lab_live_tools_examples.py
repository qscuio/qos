from __future__ import annotations

import importlib.util
import subprocess
import sys
from pathlib import Path
from types import ModuleType, SimpleNamespace


ROOT = Path(__file__).resolve().parents[1]
LINUX_LAB_ROOT = ROOT / "linux-lab"
BUILD_TOOLS_STAGE = ROOT / "linux-lab" / "orchestrator" / "stages" / "build_tools.py"
BUILD_EXAMPLES_STAGE = ROOT / "linux-lab" / "orchestrator" / "stages" / "build_examples.py"
STATE_MODULE = ROOT / "linux-lab" / "orchestrator" / "core" / "state.py"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
    if str(LINUX_LAB_ROOT) not in sys.path:
        sys.path.insert(0, str(LINUX_LAB_ROOT))
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _git_init(path: Path) -> None:
    path.mkdir(parents=True)
    (path / "README.md").write_text("fixture\n", encoding="utf-8")
    subprocess.run(["git", "init"], cwd=path, check=True, capture_output=True, text=True)
    subprocess.run(["git", "add", "."], cwd=path, check=True, capture_output=True, text=True)
    subprocess.run(
        ["git", "-c", "user.name=Test", "-c", "user.email=test@example.com", "commit", "-m", "init"],
        cwd=path,
        check=True,
        capture_output=True,
        text=True,
    )


def _write_tool_manifest(root: Path, key: str, repo_url: str) -> None:
    (root / "tools").mkdir(parents=True, exist_ok=True)
    (root / "tools" / f"{key}.yaml").write_text(
        "\n".join(
            [
                f'key: "{key}"',
                f'repo_url: "{repo_url}"',
                f'checkout_dir: "build/linux-lab/tools/{key}"',
                "prepare_commands:",
                '  - ["sh", "-c", "printf prepared > prepared.txt"]',
                "build_commands:",
                '  - ["sh", "-c", "printf built > built.txt"]',
                "",
            ]
        ),
        encoding="utf-8",
    )


def _write_tool_manifest_with_build_workdir(root: Path, key: str, repo_url: str, build_workdir: str) -> None:
    (root / "tools").mkdir(parents=True, exist_ok=True)
    (root / "tools" / f"{key}.yaml").write_text(
        "\n".join(
            [
                f'key: "{key}"',
                f'repo_url: "{repo_url}"',
                f'checkout_dir: "build/linux-lab/tools/{key}"',
                f'build_workdir: "{build_workdir}"',
                "prepare_commands:",
                '  - ["sh", "-c", "mkdir -p examples/c && printf prepared > prepared.txt"]',
                "build_commands:",
                '  - ["sh", "-c", "printf built > built.txt"]',
                "",
            ]
        ),
        encoding="utf-8",
    )


def _make_request(tmp_path: Path) -> object:
    return SimpleNamespace(
        kernel_version="fixture",
        arch="x86_64",
        image_release="noble",
        profiles=[],
        mirror_region=None,
        compat_mode=False,
        legacy_args={},
        request_fingerprint="livetools123",
        artifact_root=tmp_path / "request",
        command="run",
        dry_run=False,
        stop_after=None,
    )


def test_build_tools_stage_executes_local_tool_plan(tmp_path: Path, monkeypatch) -> None:
    build_tools_mod = _load_module("linux_lab_build_tools_live", BUILD_TOOLS_STAGE)
    state_mod = _load_module("linux_lab_state_tools_live", STATE_MODULE)

    labroot = tmp_path / "labroot"
    crash_repo = tmp_path / "repos" / "crash"
    cgdb_repo = tmp_path / "repos" / "cgdb"
    _git_init(crash_repo)
    _git_init(cgdb_repo)
    _write_tool_manifest(labroot, "crash", str(crash_repo))
    _write_tool_manifest(labroot, "cgdb", str(cgdb_repo))
    monkeypatch.setenv("LINUX_LAB_ROOT_OVERRIDE", str(labroot))

    request = _make_request(tmp_path)
    request.profiles = ["debug-tools"]
    manifests = SimpleNamespace(
        kernels={"fixture": SimpleNamespace(tool_groups=[], example_groups=[])},
        profiles={
            "debug-tools": SimpleNamespace(
                tool_groups=["debug-tools"],
                host_tools=[],
                example_groups=[],
            )
        },
    )
    request_root = Path(request.artifact_root)
    state_mod.ensure_request_dirs(request_root)

    result = build_tools_mod.STAGE.executor(request, manifests, request_root)

    assert result["status"] == "succeeded"
    for key in ("crash", "cgdb"):
        checkout = tmp_path / "build" / "linux-lab" / "tools" / key
        assert (checkout / "prepared.txt").read_text(encoding="utf-8") == "prepared"
        assert (checkout / "built.txt").read_text(encoding="utf-8") == "built"


def test_build_tools_stage_executes_tool_builds_in_manifest_subdirectory(tmp_path: Path, monkeypatch) -> None:
    build_tools_mod = _load_module("linux_lab_build_tools_subdir_live", BUILD_TOOLS_STAGE)
    state_mod = _load_module("linux_lab_state_tools_subdir_live", STATE_MODULE)

    labroot = tmp_path / "labroot"
    repo = tmp_path / "repos" / "libbpf-bootstrap"
    retsnoop_repo = tmp_path / "repos" / "retsnoop"
    _git_init(repo)
    _git_init(retsnoop_repo)
    _write_tool_manifest_with_build_workdir(labroot, "libbpf-bootstrap", str(repo), "examples/c")
    _write_tool_manifest(labroot, "retsnoop", str(retsnoop_repo))
    monkeypatch.setenv("LINUX_LAB_ROOT_OVERRIDE", str(labroot))

    request = _make_request(tmp_path)
    request.profiles = ["bpf"]
    manifests = SimpleNamespace(
        kernels={"fixture": SimpleNamespace(tool_groups=[], example_groups=[])},
        profiles={
            "bpf": SimpleNamespace(
                tool_groups=["bpf-core"],
                host_tools=[],
                example_groups=[],
            )
        },
    )
    request_root = Path(request.artifact_root)
    state_mod.ensure_request_dirs(request_root)

    result = build_tools_mod.STAGE.executor(request, manifests, request_root)

    checkout = tmp_path / "build" / "linux-lab" / "tools" / "libbpf-bootstrap"
    assert result["status"] == "succeeded"
    assert (checkout / "prepared.txt").read_text(encoding="utf-8") == "prepared"
    assert (checkout / "examples" / "c" / "built.txt").read_text(encoding="utf-8") == "built"


def test_build_examples_stage_executes_local_example_plan(tmp_path: Path, monkeypatch) -> None:
    build_examples_mod = _load_module("linux_lab_build_examples_live", BUILD_EXAMPLES_STAGE)
    state_mod = _load_module("linux_lab_state_examples_live", STATE_MODULE)

    labroot = tmp_path / "labroot"
    modules_debug = labroot / "examples" / "modules" / "debug"
    modules_simple = labroot / "examples" / "modules" / "simple"
    modules_ioctl = labroot / "examples" / "modules" / "ioctl"
    userspace_app = labroot / "examples" / "userspace" / "app"
    rust_user = labroot / "examples" / "rust" / "rust_learn" / "user"
    rust_root = labroot / "examples" / "rust" / "rust_learn"
    bpf_dir = labroot / "examples" / "bpf" / "learn"
    for path in (modules_debug, modules_simple, modules_ioctl, userspace_app, rust_user, rust_root, bpf_dir):
        path.mkdir(parents=True, exist_ok=True)

    build_root = tmp_path / "request" / "workspace" / "build"
    build_root.mkdir(parents=True, exist_ok=True)
    (build_root / "Makefile").write_text(
        "all:\n\tmkdir -p $(M)\n\tprintf built > $(M)/module.ok\n",
        encoding="utf-8",
    )

    for filename in ("poll.c", "char_file.c", "netlink_demo.c"):
        (userspace_app / filename).write_text("int main(void) { return 0; }\n", encoding="utf-8")
    (rust_user / "Makefile").write_text("all:\n\tprintf built > rust-user.ok\n", encoding="utf-8")
    (rust_root / "hello_rust.rs").write_text("// rust sample\n", encoding="utf-8")
    (rust_root / "rust_chardev.rs").write_text("// rust chardev\n", encoding="utf-8")
    for filename in ("hello.c", "packet_filter.c", "tracing.c", "xdp.c", "open.c"):
        (bpf_dir / filename).write_text("int prog(void) { return 0; }\n", encoding="utf-8")

    monkeypatch.setenv("LINUX_LAB_ROOT_OVERRIDE", str(labroot))

    request = _make_request(tmp_path)
    request.profiles = ["default-lab"]
    manifests = SimpleNamespace(
        kernels={"fixture": SimpleNamespace(tool_groups=[], example_groups=[])},
        profiles={
            "default-lab": SimpleNamespace(
                example_groups=["modules-core", "userspace-core", "rust-core", "bpf-core"],
                tool_groups=[],
            )
        },
    )
    request_root = Path(request.artifact_root)
    state_mod.ensure_request_dirs(request_root)

    result = build_examples_mod.STAGE.executor(request, manifests, request_root)

    assert result["status"] == "succeeded"
    for module_dir in (modules_debug, modules_simple, modules_ioctl):
        assert (module_dir / "module.ok").read_text(encoding="utf-8") == "built"
    for binary in ("poll", "char_file", "netlink_demo"):
        assert (userspace_app / binary).is_file()
    assert (rust_user / "rust-user.ok").read_text(encoding="utf-8") == "built"
    for obj in ("hello.o", "packet_filter.o", "tracing.o", "xdp.o", "open.o"):
        assert (bpf_dir / obj).is_file()
