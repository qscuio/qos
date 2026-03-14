from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_linux_lab_repo_layout_exists() -> None:
    expected = [
        "linux-lab/bin/linux-lab",
        "linux-lab/bin/ulk",
        "linux-lab/orchestrator/cli.py",
        "linux-lab/orchestrator/core/manifests.py",
        "linux-lab/orchestrator/core/request.py",
        "linux-lab/orchestrator/stages/fetch.py",
        "linux-lab/manifests/kernels/6.18.4.yaml",
        "linux-lab/manifests/arches/x86_64.yaml",
        "linux-lab/manifests/images/noble.yaml",
        "linux-lab/manifests/profiles/default-lab.yaml",
    ]

    missing = [path for path in expected if not (ROOT / path).exists()]
    assert missing == [], f"missing linux-lab scaffold paths: {missing}"


def test_linux_lab_make_targets_and_docs_exist() -> None:
    makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    ignore = (ROOT / ".gitignore").read_text(encoding="utf-8")

    assert "linux-lab-validate:" in makefile
    assert "linux-lab-plan:" in makefile
    assert "linux-lab-run:" in makefile
    assert "linux-lab" in readme
    assert "--dry-run" in readme
    assert "build/linux-lab/" in ignore
