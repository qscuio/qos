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
        default_mirror="http://archive.ubuntu.com/ubuntu/"
        ;;
    arm64)
        debarch="arm64"
        default_mirror="http://ports.ubuntu.com/ubuntu-ports/"
        ;;
    *)
        debarch="$arch"
        default_mirror="http://archive.ubuntu.com/ubuntu/"
        ;;
esac

mirror="${mirror_url:-$default_mirror}"

if ! command -v debootstrap >/dev/null 2>&1; then
    echo "debootstrap is required for live image creation" >&2
    exit 1
fi

sudo mkdir -p "$chroot_dir"
sudo debootstrap --arch="$debarch" "$release" "$chroot_dir" "$mirror"
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
