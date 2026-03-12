from pathlib import Path

from tools.spec_guard import validate_spec


def _has(code: str, errors: list[str]) -> bool:
    return any(err.startswith(code) for err in errors)


def test_flags_waitpid_wnohang_mismatch() -> None:
    spec = """
Syscalls:
  4  wait(pid, status*)

With full signal support, the shell registers a SIGCHLD handler via sigaction()
that calls waitpid(-1, WNOHANG) in a loop to reap zombies.
"""
    errors = validate_spec(spec)
    assert _has("E001", errors)


def test_flags_doubly_linked_without_prev_pointer() -> None:
    spec = """
Run queue is a doubly-linked list.

typedef struct thread {
    struct thread* next;
} thread_t;
"""
    errors = validate_spec(spec)
    assert _has("E002", errors)


def test_flags_ext2_without_storage_driver() -> None:
    spec = """
| ext2 | /data | On QEMU virtio-blk disk image. |

### Drivers
| Architecture | NIC |
| x86-64 | e1000 |
"""
    errors = validate_spec(spec)
    assert _has("E003", errors)


def test_flags_incorrect_signal_pending_inheritance() -> None:
    spec = """
| `fork()` | Child inherits: `sig_handlers`, `sig_blocked`, `sig_pending`, `sig_altstack` |
| `exec()` | `sig_blocked` preserved; `sig_pending` preserved; all handlers reset to SIG_DFL except SIG_IGN |
"""
    errors = validate_spec(spec)
    assert _has("E004", errors)


def test_architecture_spec_has_no_consistency_errors() -> None:
    spec_path = (
        Path(__file__).resolve().parents[1]
        / "docs"
        / "superpowers"
        / "specs"
        / "2026-03-11-qos-design.md"
    )
    errors = validate_spec(spec_path.read_text(encoding="utf-8"))
    assert errors == []
