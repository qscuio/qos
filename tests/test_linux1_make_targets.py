from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAKEFILE = ROOT / "Makefile"
CURSES_RUNNER = ROOT / "scripts" / "run_linux1_curses.sh"


def test_linux1_curses_target_exists():
    text = MAKEFILE.read_text()
    assert "linux1-curses:" in text, "missing linux1-curses target in top-level Makefile"


def test_linux1_curses_runner_exists():
    assert CURSES_RUNNER.exists(), f"missing curses runner script: {CURSES_RUNNER}"
