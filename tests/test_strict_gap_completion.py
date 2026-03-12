from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_strict_gap_master_plan_has_all_phases_completed() -> None:
    plan = ROOT / "docs/superpowers/plans/2026-03-11-design-strict-gap-closure.md"
    text = plan.read_text(encoding="utf-8")
    required = [
        "- [x] **Phase 53:",
        "- [x] **Phase 54:",
        "- [x] **Phase 55:",
        "- [x] **Phase 56:",
        "- [x] **Phase 57:",
        "- [x] **Phase 58:",
        "- [x] **Phase 59:",
    ]
    for marker in required:
        assert marker in text, f"missing completion marker {marker!r}"


def test_strict_gap_phase_plan_docs_exist() -> None:
    plans_dir = ROOT / "docs/superpowers/plans"
    expected = [
        "2026-03-11-phase56-vfs-filesystem-expansion.md",
        "2026-03-11-phase57-userspace-feature-alignment.md",
        "2026-03-11-phase58-boot-arch-boundary-tightening.md",
        "2026-03-11-phase59-final-conformance-audit.md",
    ]
    for name in expected:
        assert (plans_dir / name).is_file(), f"missing plan doc: {name}"
