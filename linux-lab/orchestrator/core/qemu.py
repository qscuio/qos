from __future__ import annotations

from pathlib import Path


def build_qemu_command(
    *,
    arch: str,
    kernel_image: Path,
    disk_image: Path,
    shared_dir: Path,
    network_up_script: Path | None = None,
    network_down_script: Path | None = None,
    pidfile: Path | None = None,
    enable_kvm: bool | None = None,
) -> list[str]:
    tap_up = str(network_up_script) if network_up_script is not None else "no"
    tap_down = str(network_down_script) if network_down_script is not None else "no"
    if arch == "x86_64":
        use_kvm = Path("/dev/kvm").exists() if enable_kvm is None else enable_kvm
        command = [
            "qemu-system-x86_64",
            "-qmp",
            "tcp:localhost:23456,server,nowait",
            "-smp",
            "4",
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
            "-net",
            "nic,model=e1000",
            "-net",
            f"tap,ifname=tap1,script={tap_up},downscript={tap_down}",
            "-net",
            "nic,model=e1000",
            "-net",
            "tap,ifname=tap2,script=no,downscript=no",
            "-nographic",
            "-s",
            "-S",
            "-monitor",
            "telnet:localhost:12345,server,nowait",
        ]
        if use_kvm:
            command[5:5] = ["-enable-kvm", "-cpu", "host"]
        else:
            command[5:5] = ["-accel", "tcg,thread=multi", "-cpu", "max"]
        if pidfile is not None:
            command.extend(["-pidfile", str(pidfile)])
        return command
    if arch == "arm64":
        command = [
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
            "-net",
            "nic,model=e1000",
            "-net",
            f"tap,ifname=tap1,script={tap_up},downscript={tap_down}",
            "-net",
            "nic,model=e1000",
            "-net",
            "tap,ifname=tap2,script=no,downscript=no",
            "-nographic",
            "-s",
            "-S",
            "-monitor",
            "telnet:localhost:12345,server,nowait",
        ]
        if pidfile is not None:
            command.extend(["-pidfile", str(pidfile)])
        return command
    raise ValueError(f"unsupported qemu arch: {arch}")
