from __future__ import annotations

from pathlib import Path


def build_qemu_command(*, arch: str, kernel_image: Path, disk_image: Path, shared_dir: Path) -> list[str]:
    if arch == "x86_64":
        return [
            "qemu-system-x86_64",
            "-qmp",
            "tcp:localhost:23456,server,nowait",
            "-smp",
            "4",
            "-enable-kvm",
            "-cpu",
            "host",
            "-m",
            "8G",
            "-kernel",
            str(kernel_image),
            "-append",
            "console=ttyS0 root=/dev/sda earlyprintk=serial nokaslr page_owner=on",
            "-drive",
            f"file={disk_image},format=raw",
            "-fsdev",
            f"local,id=myid,path={shared_dir},security_model=mapped",
            "-device",
            "virtio-9p-pci,fsdev=myid,mount_tag=hostshare",
            "-nographic",
            "-s",
            "-S",
            "-monitor",
            "telnet:localhost:12345,server,nowait",
        ]
    if arch == "arm64":
        return [
            "qemu-system-aarch64",
            "-qmp",
            "tcp:localhost:23456,server,nowait",
            "-machine",
            "virt,gic-version=max",
            "-accel",
            "tcg,thread=multi",
            "-cpu",
            "max",
            "-smp",
            "4",
            "-m",
            "8G",
            "-append",
            "console=ttyAMA0 root=/dev/vda earlyprintk=serial slub_debug=UZ page_owner=on",
            "-kernel",
            str(kernel_image),
            "-drive",
            f"file={disk_image},format=raw",
            "-fsdev",
            f"local,id=myid,path={shared_dir},security_model=mapped",
            "-device",
            "virtio-9p-pci,fsdev=myid,mount_tag=hostshare",
            "-nographic",
            "-s",
            "-S",
            "-monitor",
            "telnet:localhost:12345,server,nowait",
        ]
    raise ValueError(f"unsupported qemu arch: {arch}")
