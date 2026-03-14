from __future__ import annotations

import importlib.util
import json
import shutil
import subprocess
from pathlib import Path
from types import ModuleType

import pytest
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

    crash = _read_yaml(expected["crash"])
    assert crash["build_policy"] == "host-build"
    assert crash["post_prepare_asset_copies"] == [
        {
            "from": "linux-lab/tools/assets/crash/extensions/",
            "to": "extensions/",
        }
    ]

    assert _read_yaml(expected["cgdb"])["build_policy"] == "host-build"
    assert _read_yaml(expected["libbpf-bootstrap"])["build_policy"] == "guest-build"
    assert _read_yaml(expected["retsnoop"])["build_policy"] == "guest-build"


def test_selected_kernel_manifests_include_kernel_tools_group() -> None:
    kernel_paths = [
        ROOT / "linux-lab" / "manifests" / "kernels" / "4.19.317.yaml",
        ROOT / "linux-lab" / "manifests" / "kernels" / "6.4.3.yaml",
        ROOT / "linux-lab" / "manifests" / "kernels" / "6.9.6.yaml",
        ROOT / "linux-lab" / "manifests" / "kernels" / "6.9.8.yaml",
        ROOT / "linux-lab" / "manifests" / "kernels" / "6.10.yaml",
        ROOT / "linux-lab" / "manifests" / "kernels" / "6.18.4.yaml",
    ]

    for path in kernel_paths:
        manifest = _read_yaml(path)
        assert "kernel-tools" in manifest["tool_groups"], path.name


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
    assert plan[0]["build_policy"] == "host-build"
    assert plan[0]["post_prepare_asset_copies"] == [
        {
            "from": str(ROOT / "linux-lab" / "tools" / "assets" / "crash" / "extensions"),
            "to": "extensions/",
        }
    ]
    assert plan[1]["build_policy"] == "guest-build"
    assert plan[1]["guest_build_commands"] == [["make", "minimal"]]
    assert plan[1]["build_commands"] == []


def test_tooling_helper_rejects_unknown_build_policy(tmp_path: Path) -> None:
    module = _load_module("linux_lab_tooling_invalid_policy", MODULE_PATH)
    tools_root = tmp_path / "labroot" / "tools"
    tools_root.mkdir(parents=True, exist_ok=True)
    (tools_root / "broken.yaml").write_text(
        "\n".join(
            [
                'key: "broken"',
                'repo_url: "https://example.invalid/broken.git"',
                'checkout_dir: "build/linux-lab/tools/broken"',
                'build_policy: "wrong"',
                "",
            ]
        ),
        encoding="utf-8",
    )

    with pytest.raises(ValueError, match="build_policy"):
        module.resolve_tool_plan(
            tool_keys=["broken"],
            linux_lab_root=tmp_path / "labroot",
        )


@pytest.mark.parametrize(
    ("copy_rule", "match"),
    [
        ({"from": "/tmp/crash/extensions", "to": "extensions/"}, "repo-relative"),
        ({"from": "linux-lab/tools/assets/crash/extensions/", "to": "/tmp/extensions"}, "checkout-relative"),
        ({"from": "../tools/assets/crash/extensions/", "to": "extensions/"}, "repo-relative"),
        ({"from": "linux-lab/tools/assets/crash/extensions/", "to": "../extensions"}, "checkout-relative"),
    ],
)
def test_tooling_helper_rejects_invalid_asset_copy_paths(
    tmp_path: Path, copy_rule: dict[str, str], match: str
) -> None:
    module = _load_module("linux_lab_tooling_invalid_copies", MODULE_PATH)
    tools_root = tmp_path / "labroot" / "tools"
    tools_root.mkdir(parents=True, exist_ok=True)
    (tools_root / "crash.yaml").write_text(
        "\n".join(
            [
                'key: "crash"',
                'repo_url: "https://example.invalid/crash.git"',
                'checkout_dir: "build/linux-lab/tools/crash"',
                'build_policy: "host-build"',
                "prepare_commands: []",
                "build_commands: []",
                "guest_build_commands: []",
                "post_prepare_asset_copies:",
                f'  - from: "{copy_rule["from"]}"',
                f'    to: "{copy_rule["to"]}"',
                "",
            ]
        ),
        encoding="utf-8",
    )

    with pytest.raises(ValueError, match=match):
        module.resolve_tool_plan(
            tool_keys=["crash"],
            linux_lab_root=tmp_path / "labroot",
        )


def test_tooling_helper_resolves_asset_copy_sources_from_override_root(tmp_path: Path) -> None:
    module = _load_module("linux_lab_tooling_override_assets", MODULE_PATH)
    labroot = tmp_path / "labroot"
    tools_root = labroot / "tools"
    tools_root.mkdir(parents=True, exist_ok=True)
    asset_source = labroot / "tools" / "assets" / "crash" / "extensions"
    asset_source.mkdir(parents=True, exist_ok=True)
    (asset_source / "echo.c").write_text("fixture\n", encoding="utf-8")
    (tools_root / "crash.yaml").write_text(
        "\n".join(
            [
                'key: "crash"',
                'repo_url: "https://example.invalid/crash.git"',
                'checkout_dir: "build/linux-lab/tools/crash"',
                'build_policy: "host-build"',
                "prepare_commands: []",
                "build_commands: []",
                "guest_build_commands: []",
                "post_prepare_asset_copies:",
                '  - from: "linux-lab/tools/assets/crash/extensions/"',
                '    to: "extensions/"',
                "",
            ]
        ),
        encoding="utf-8",
    )

    plan = module.resolve_tool_plan(tool_keys=["crash"], linux_lab_root=labroot)

    assert plan[0]["post_prepare_asset_copies"] == [
        {
            "from": str(asset_source),
            "to": "extensions/",
        }
    ]


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
    metadata = tool_state["metadata"]
    assert [item["key"] for item in metadata["tools"]] == [
        "crash",
        "cgdb",
        "libbpf-bootstrap",
        "retsnoop",
    ]
    assert [item["key"] for item in metadata["external_tools"]] == [
        "crash",
        "cgdb",
        "libbpf-bootstrap",
        "retsnoop",
    ]
    assert metadata["tools"] == metadata["external_tools"]
    assert metadata["post_prepare_asset_copies"] == [
        {
            "from": str(ROOT / "linux-lab" / "tools" / "assets" / "crash" / "extensions"),
            "to": "extensions/",
        }
    ]

    external_tools = {item["key"]: item for item in metadata["external_tools"]}
    assert external_tools["crash"]["build_policy"] == "host-build"
    assert external_tools["cgdb"]["build_policy"] == "host-build"
    assert external_tools["libbpf-bootstrap"]["build_policy"] == "guest-build"
    assert external_tools["retsnoop"]["build_policy"] == "guest-build"
    assert external_tools["crash"]["guest_build_commands"] == []
    assert external_tools["cgdb"]["guest_build_commands"] == []
    assert external_tools["libbpf-bootstrap"]["guest_build_commands"] == [["make", "minimal"]]
    assert external_tools["retsnoop"]["guest_build_commands"] == [["make"]]
    assert external_tools["crash"]["post_prepare_asset_copies"] == [
        {
            "from": str(ROOT / "linux-lab" / "tools" / "assets" / "crash" / "extensions"),
            "to": "extensions/",
        }
    ]
    assert external_tools["cgdb"]["post_prepare_asset_copies"] == []
    assert external_tools["libbpf-bootstrap"]["post_prepare_asset_copies"] == []
    assert external_tools["retsnoop"]["post_prepare_asset_copies"] == []

    for item in metadata["external_tools"]:
        for key in (
            "key",
            "build_policy",
            "checkout_dir",
            "prepare_commands",
            "build_commands",
            "guest_build_commands",
            "post_prepare_asset_copies",
        ):
            assert key in item, (item["key"], key)

    build_root = request_root / "workspace" / "build"
    kernel_tree = request_root / "workspace" / "kernel" / "linux-6.18.4"
    assert metadata["kernel_tools"] == [
        {
            "name": "tools/libapi",
            "command": [
                "make",
                f"O={build_root}",
                "ARCH=x86_64",
                "CROSS_COMPILE=x86_64-linux-gnu-",
                "tools/libapi",
            ],
        },
        {
            "name": "kernel-tools",
            "command": [
                "make",
                "-C",
                str(kernel_tree / "tools"),
                f"O={build_root}",
                "subdir=tools",
                "all",
            ],
        },
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
    assert tool_state["metadata"]["external_tools"] == []
    assert tool_state["metadata"]["kernel_tools"] == []
    assert tool_state["metadata"]["post_prepare_asset_copies"] == []


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
