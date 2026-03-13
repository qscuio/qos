from __future__ import annotations

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_aarch64_c_guest_shell_supports_touch_edit_and_redirection(tmp_path: Path) -> None:
    build = subprocess.run(
        ["make", "-C", "c-os", "ARCH=aarch64", "build"],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    assert build.returncode == 0, f"stderr:\n{build.stderr}\nstdout:\n{build.stdout}"

    disk = ROOT / "c-os/build/aarch64/test-shell.disk.img"
    disk.parent.mkdir(parents=True, exist_ok=True)
    create = subprocess.run(
        ["truncate", "-s", "64M", str(disk)],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    assert create.returncode == 0, f"stderr:\n{create.stderr}\nstdout:\n{create.stdout}"

    kernel = ROOT / "c-os/build/aarch64/kernel.elf"
    initramfs = ROOT / "c-os/build/aarch64/initramfs.cpio"
    map_log = tmp_path / "mapwatch-c.log"
    script = "\n".join(
        [
            "touch /tmp/note",
            "edit /tmp/note hello-qos",
            "cat /tmp/note",
            "echo pipe-edit | edit /tmp/note",
            "cat /tmp/note",
            "echo hello > /tmp/msg",
            "cat < /tmp/msg",
            "ls /proc",
            "cat /proc/kernel/status",
            "cat /proc/syscalls",
            "cat /proc/runtime/map",
            "mapwatch side on",
            "mapwatch side off",
            "insmod /lib/modules/qos_test.ko",
            "rmmod /lib/modules/qos_test.ko",
            "wqdemo",
            "exit",
            "",
        ]
    )
    run = subprocess.run(
        ["timeout", "6s", "./qemu/aarch64.sh", str(disk), str(kernel), str(initramfs)],
        cwd=ROOT,
        text=True,
        capture_output=True,
        input=script,
        env={**os.environ, "QOS_MAPWATCH_LOG": str(map_log)},
    )
    assert run.returncode in (0, 124), f"stderr:\n{run.stderr}\nstdout:\n{run.stdout}"
    text = f"{run.stdout}\n{run.stderr}"
    assert "QOS kernel started impl=c arch=aarch64" in text, text
    assert "qos-sh:/>" in text, text
    assert "created file /tmp/note" in text, text
    assert "edited /tmp/note" in text, text
    assert "hello-qos" in text, text
    assert "pipe-edit" in text, text
    assert "hello" in text, text
    assert "runtime/map" in text, text
    assert "1/status" in text, text
    assert "InitState:" in text, text
    assert "SyscallCount:" in text, text
    assert "CurrentPid:" in text, text
    assert "CurrentProc:" in text, text
    assert "CurrentAsid:" in text, text
    assert "mapwatch side: on (host stream)" in text, text
    assert "insmod: loaded id=" in text, text
    assert "path=/lib/modules/qos_test.ko" in text, text
    assert "rmmod: unloaded id=" in text, text
    assert "workqueue demo: pending=0 completed=2 hits=3" in text, text
    map_text = map_log.read_text(encoding="utf-8")
    assert "[mapwatch-side] /proc/runtime/map" in map_text, map_text
    assert "CurrentPid:" in map_text, map_text
    assert "Proc0:" in map_text, map_text


def test_aarch64_rust_guest_shell_supports_ls_pwd_cat_touch_and_edit(tmp_path: Path) -> None:
    build = subprocess.run(
        ["make", "-C", "rust-os", "ARCH=aarch64", "build"],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    assert build.returncode == 0, f"stderr:\n{build.stderr}\nstdout:\n{build.stdout}"

    disk = ROOT / "rust-os/build/aarch64/test-shell.disk.img"
    disk.parent.mkdir(parents=True, exist_ok=True)
    create = subprocess.run(
        ["truncate", "-s", "64M", str(disk)],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    assert create.returncode == 0, f"stderr:\n{create.stderr}\nstdout:\n{create.stdout}"

    kernel = ROOT / "rust-os/build/aarch64/kernel.elf"
    initramfs = ROOT / "rust-os/build/aarch64/initramfs.cpio"
    map_log = tmp_path / "mapwatch-rust.log"
    script = "\n".join(
        [
            "ls",
            "pwd",
            "touch /tmp/rnote",
            "edit /tmp/rnote hello-rust",
            "cat /tmp/rnote",
            "ls /proc",
            "cat /proc/kernel/status",
            "cat /proc/syscalls",
            "cat /proc/runtime/map",
            "mapwatch side on",
            "mapwatch side off",
            "insmod /lib/modules/qos_test.ko",
            "rmmod /lib/modules/qos_test.ko",
            "wqdemo",
            "exit",
            "",
        ]
    )
    run = subprocess.run(
        ["timeout", "6s", "./qemu/aarch64.sh", str(disk), str(kernel), str(initramfs)],
        cwd=ROOT,
        text=True,
        capture_output=True,
        input=script,
        env={**os.environ, "QOS_MAPWATCH_LOG": str(map_log)},
    )
    assert run.returncode in (0, 124), f"stderr:\n{run.stderr}\nstdout:\n{run.stdout}"
    text = f"{run.stdout}\n{run.stderr}"
    assert "QOS kernel started impl=rust arch=aarch64" in text, text
    assert "qos-sh:/>" in text, text
    assert "bin" in text, text
    assert "created file /tmp/rnote" in text, text
    assert "edited /tmp/rnote" in text, text
    assert "hello-rust" in text, text
    assert "runtime/map" in text, text
    assert "1/status" in text, text
    assert "InitState:" in text, text
    assert "SyscallCount:" in text, text
    assert "CurrentPid:" in text, text
    assert "CurrentProc:" in text, text
    assert "CurrentAsid:" in text, text
    assert "mapwatch side: on (host stream)" in text, text
    assert "insmod: loaded id=" in text, text
    assert "path=/lib/modules/qos_test.ko" in text, text
    assert "rmmod: unloaded id=" in text, text
    assert "workqueue demo: pending=0 completed=2 hits=3" in text, text
    map_text = map_log.read_text(encoding="utf-8")
    assert "[mapwatch-side] /proc/runtime/map" in map_text, map_text
    assert "CurrentPid:" in map_text, map_text
    assert "Proc0:" in map_text, map_text
