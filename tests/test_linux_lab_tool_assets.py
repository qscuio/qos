from __future__ import annotations

import importlib.util
import json
import shutil
import subprocess
from pathlib import Path
from types import ModuleType

import yaml


ROOT = Path(__file__).resolve().parents[1]
BIN_LINUX_LAB = ROOT / "linux-lab" / "bin" / "linux-lab"
REQUESTS_ROOT = ROOT / "build" / "linux-lab" / "requests"
MODULE_PATH = ROOT / "linux-lab" / "orchestrator" / "core" / "tooling.py"


def _load_module(name: str, path: Path) -> ModuleType:
    assert path.is_file(), f"missing module file: {path}"
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _read_yaml(path: Path) -> dict:
    data = yaml.safe_load(path.read_text(encoding="utf-8"))
    assert isinstance(data, dict), f"{path} must decode to a mapping"
    return data


def _run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True)


def _latest_request_root() -> Path:
    request_dirs = sorted([path for path in REQUESTS_ROOT.iterdir() if path.is_dir()])
    assert request_dirs, f"no request directories in {REQUESTS_ROOT}"
    return request_dirs[-1]


def setup_function() -> None:
    shutil.rmtree(ROOT / "build" / "linux-lab", ignore_errors=True)


def test_external_tool_manifests_are_ported() -> None:
    expected = {
        "crash": ROOT / "linux-lab" / "tools" / "crash.yaml",
        "cgdb": ROOT / "linux-lab" / "tools" / "cgdb.yaml",
        "libbpf-bootstrap": ROOT / "linux-lab" / "tools" / "libbpf-bootstrap.yaml",
        "retsnoop": ROOT / "linux-lab" / "tools" / "retsnoop.yaml",
    }

    for key, path in expected.items():
        manifest = _read_yaml(path)
        assert manifest["key"] == key
        assert manifest["repo_url"].startswith("https://")
        assert manifest["checkout_dir"].startswith("build/linux-lab/tools/")


def test_tooling_helper_resolves_clone_commands() -> None:
    module = _load_module("linux_lab_tooling", MODULE_PATH)
    manifests = module.load_tool_manifests(ROOT / "linux-lab" / "tools")
    plan = module.resolve_tool_plan(
        tool_keys=["crash", "libbpf-bootstrap"],
        linux_lab_root=ROOT / "linux-lab",
    )

    assert [item["key"] for item in plan] == ["crash", "libbpf-bootstrap"]
    assert plan[0]["clone_command"][:2] == ["git", "clone"]
    assert plan[1]["prepare_commands"][0][:3] == ["git", "submodule", "update"]


def test_run_dry_run_emits_tool_metadata() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "run",
            "--kernel",
            "6.18.4",
            "--arch",
            "x86_64",
            "--image",
            "noble",
            "--profile",
            "default-lab",
            "--dry-run",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    tool_state = json.loads((request_root / "state" / "build-tools.json").read_text(encoding="utf-8"))
    assert tool_state["status"] == "dry-run"
    assert [item["key"] for item in tool_state["metadata"]["tools"]] == [
        "crash",
        "cgdb",
        "libbpf-bootstrap",
        "retsnoop",
    ]


def test_minimal_profile_omits_optional_tool_metadata() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "run",
            "--kernel",
            "6.18.4",
            "--arch",
            "arm64",
            "--image",
            "noble",
            "--profile",
            "minimal",
            "--dry-run",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    tool_state = json.loads((request_root / "state" / "build-tools.json").read_text(encoding="utf-8"))
    assert tool_state["status"] == "dry-run"
    assert tool_state["metadata"]["tool_groups"] == []
    assert tool_state["metadata"]["tools"] == []


def test_tooling_helper_supports_subdirectory_build_workdir(tmp_path: Path) -> None:
    module = _load_module("linux_lab_tooling_subdir", MODULE_PATH)

    tools_root = tmp_path / "labroot" / "tools"
    tools_root.mkdir(parents=True, exist_ok=True)
    (tools_root / "libbpf-bootstrap.yaml").write_text(
        "\n".join(
            [
                'key: "libbpf-bootstrap"',
                'repo_url: "https://example.invalid/libbpf-bootstrap.git"',
                'checkout_dir: "build/linux-lab/tools/libbpf-bootstrap"',
                'build_workdir: "examples/c"',
                "prepare_commands:",
                '  - ["git", "submodule", "update", "--init"]',
                "build_commands:",
                '  - ["make", "minimal"]',
                "",
            ]
        ),
        encoding="utf-8",
    )

    plan = module.resolve_tool_plan(
        tool_keys=["libbpf-bootstrap"],
        linux_lab_root=tmp_path / "labroot",
    )

    assert len(plan) == 1
    assert plan[0]["build_workdir"].endswith("build/linux-lab/tools/libbpf-bootstrap/examples/c")
    assert plan[0]["build_commands"] == [["make", "minimal"]]
