#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MANIFEST="$ROOT/linux-1.0.0/manifests/linux1-sources.lock"
KERNEL_TREE="$ROOT/linux-1.0.0"
LILO_ARTIFACT_DIR="$ROOT/build/linux1/lilo"

if [[ ! -f "$MANIFEST" ]]; then
  echo "ERROR:verify-linux1-provenance:manifest not found"
  exit 2
fi
if [[ ! -d "$KERNEL_TREE" ]]; then
  echo "ERROR:verify-linux1-provenance:kernel tree missing ($KERNEL_TREE)"
  exit 2
fi

workdir="$(mktemp -d)"
trap 'rm -rf "$workdir"' EXIT

linux_tar=""
lilo_tar=""
while IFS='|' read -r name _url _sha filename; do
  [[ -z "${name:-}" ]] && continue
  [[ "${name:0:1}" == "#" ]] && continue
  case "$name" in
    linux-1.0.0) linux_tar="$ROOT/build/linux1/sources/$filename" ;;
    lilo-20) lilo_tar="$ROOT/build/linux1/sources/$filename" ;;
  esac
done < "$MANIFEST"

if [[ ! -f "$linux_tar" ]]; then
  echo "ERROR:verify-linux1-provenance:missing linux archive: $linux_tar"
  exit 2
fi
if [[ ! -f "$lilo_tar" ]]; then
  echo "ERROR:verify-linux1-provenance:missing lilo archive: $lilo_tar"
  exit 2
fi

mkdir -p "$workdir/src/linux" "$workdir/src/lilo"
tar -xf "$linux_tar" -C "$workdir/src/linux"

linux_src_candidate=""
if [[ -d "$workdir/src/linux/linux" ]]; then
  linux_src_candidate="$workdir/src/linux/linux"
else
  linux_src_candidate="$(find "$workdir/src/linux" -mindepth 1 -maxdepth 1 -type d | head -n1)"
fi
if [[ -z "$linux_src_candidate" || ! -d "$linux_src_candidate" ]]; then
  echo "ERROR:verify-linux1-provenance:could not locate extracted linux source"
  exit 2
fi

if compgen -G "$ROOT/linux-1.0.0/patches/kernel/*.patch" > /dev/null; then
  for p in "$ROOT"/linux-1.0.0/patches/kernel/*.patch; do
    patch -d "$linux_src_candidate" -p1 < "$p" >/dev/null
  done
fi

diff -ruN --exclude='.git' "$linux_src_candidate" "$KERNEL_TREE" >/dev/null || {
  echo "ERROR:verify-linux1-provenance:linux tree mismatch versus committed linux-1.0.0"
  exit 2
}

tar -xf "$lilo_tar" -C "$workdir/src/lilo"
lilo_src_candidate="$(find "$workdir/src/lilo" -mindepth 1 -maxdepth 1 -type d | head -n1)"
if [[ -z "$lilo_src_candidate" || ! -d "$lilo_src_candidate" ]]; then
  echo "ERROR:verify-linux1-provenance:could not locate extracted lilo source"
  exit 2
fi

if compgen -G "$ROOT/linux-1.0.0/patches/lilo/*.patch" > /dev/null; then
  for p in "$ROOT"/linux-1.0.0/patches/lilo/*.patch; do
    patch -d "$lilo_src_candidate" -p1 < "$p" >/dev/null
  done
fi

if [[ ! -x "$LILO_ARTIFACT_DIR/lilo" ]]; then
  echo "ERROR:verify-linux1-provenance:expected built lilo binary at $LILO_ARTIFACT_DIR/lilo"
  exit 2
fi

if ! "$LILO_ARTIFACT_DIR/lilo" -v >/tmp/lilo-version.out 2>&1; then
  echo "ERROR:verify-linux1-provenance:lilo version check failed"
  exit 1
fi

if ! grep -qi "lilo" /tmp/lilo-version.out; then
  echo "ERROR:verify-linux1-provenance:lilo version signature missing"
  exit 2
fi

echo "OK: provenance"
