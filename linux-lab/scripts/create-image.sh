#!/usr/bin/env bash

set -euo pipefail

arch=""
release=""
image_path=""
chroot_dir=""
netcfg_path=""
sources_list_path=""
mirror_url=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --arch)
            arch="$2"
            shift 2
            ;;
        --release)
            release="$2"
            shift 2
            ;;
        --image)
            image_path="$2"
            shift 2
            ;;
        --chroot)
            chroot_dir="$2"
            shift 2
            ;;
        --netcfg)
            netcfg_path="$2"
            shift 2
            ;;
        --sources-list)
            sources_list_path="$2"
            shift 2
            ;;
        --mirror-url)
            mirror_url="$2"
            shift 2
            ;;
        *)
            echo "unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

if [[ -z "$arch" || -z "$release" || -z "$image_path" || -z "$chroot_dir" || -z "$netcfg_path" || -z "$sources_list_path" ]]; then
    echo "missing required arguments" >&2
    exit 1
fi

mode="${LINUX_LAB_IMAGE_MODE:-live}"
image_size_mb="${LINUX_LAB_IMAGE_SIZE_MB:-8192}"
host_arch="${LINUX_LAB_HOST_ARCH:-}"

if ! command -v mkfs.ext4 >/dev/null 2>&1; then
    echo "mkfs.ext4 is required for linux-lab image creation" >&2
    exit 1
fi

mkdir -p "$(dirname "$image_path")"
mkdir -p "$chroot_dir"

if [[ "$mode" == "mock" ]]; then
    mkdir -p "$chroot_dir/etc/netplan" "$chroot_dir/etc/apt"
    install -m 0644 "$netcfg_path" "$chroot_dir/etc/netplan/01-netcfg.yaml"
    install -m 0644 "$sources_list_path" "$chroot_dir/etc/apt/sources.list"
    truncate -s "${image_size_mb}M" "$image_path"
    {
        echo "arch=$arch"
        echo "release=$release"
        echo "image=$image_path"
        echo "chroot=$chroot_dir"
        echo "mirror=${mirror_url:-default}"
        echo "mode=mock"
    } > "$chroot_dir/linux-lab-image.env"
    truncate -s "${image_size_mb}M" "$image_path"
    mkfs.ext4 -F -d "$chroot_dir" "$image_path" >/dev/null 2>&1
    echo "mock ext4 image created at $image_path"
    exit 0
fi

case "$arch" in
    x86_64)
        debarch="amd64"
        qemu_static="qemu-x86_64-static"
        default_mirror="http://archive.ubuntu.com/ubuntu/"
        ;;
    arm64)
        debarch="arm64"
        qemu_static="qemu-aarch64-static"
        default_mirror="http://ports.ubuntu.com/ubuntu-ports/"
        ;;
    *)
        debarch="$arch"
        qemu_static="qemu-$arch-static"
        default_mirror="http://archive.ubuntu.com/ubuntu/"
        ;;
esac

mirror="${mirror_url:-$default_mirror}"

if ! command -v debootstrap >/dev/null 2>&1; then
    echo "debootstrap is required for live image creation" >&2
    exit 1
fi

if [[ -z "$host_arch" ]]; then
    host_arch="$(dpkg --print-architecture)"
fi

debootstrap_args=(--arch="$debarch")
if [[ "$host_arch" != "$debarch" ]]; then
    debootstrap_args=(--foreign "${debootstrap_args[@]}")
fi

sudo mkdir -p "$chroot_dir"
sudo debootstrap "${debootstrap_args[@]}" "$release" "$chroot_dir" "$mirror"
if [[ "$host_arch" != "$debarch" ]]; then
    if ! command -v "$qemu_static" >/dev/null 2>&1; then
        echo "$qemu_static is required for cross-architecture image creation" >&2
        exit 1
    fi
    sudo install -D -m 0755 "$(command -v "$qemu_static")" "$chroot_dir/usr/bin/$qemu_static"
    sudo chroot "$chroot_dir" "/usr/bin/$qemu_static" /bin/bash -c "/debootstrap/debootstrap --second-stage"
fi
sudo install -D -m 0644 "$netcfg_path" "$chroot_dir/etc/netplan/01-netcfg.yaml"
sudo install -D -m 0644 "$sources_list_path" "$chroot_dir/etc/apt/sources.list"
truncate -s "${image_size_mb}M" "$image_path"
{
    echo "arch=$arch"
    echo "release=$release"
    echo "image=$image_path"
    echo "chroot=$chroot_dir"
    echo "mirror=$mirror"
    echo "mode=live"
} | sudo tee "$chroot_dir/linux-lab-image.env" >/dev/null
sudo mkfs.ext4 -F -d "$chroot_dir" "$image_path" >/dev/null 2>&1
sudo chown "$(id -u):$(id -g)" "$image_path"
echo "ext4 image environment created under $chroot_dir"
