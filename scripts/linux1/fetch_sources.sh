#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MANIFEST="$ROOT/linux-1.0.0/manifests/linux1-sources.lock"
OUTDIR="$ROOT/build/linux1/sources"

mkdir -p "$OUTDIR"

if [[ ! -f "$MANIFEST" ]]; then
  echo "ERROR:fetch-linux1-sources:manifest not found: $MANIFEST"
  exit 2
fi

missing_hash=0

while IFS='|' read -r name url sha filename; do
  [[ -z "${name:-}" ]] && continue
  [[ "${name:0:1}" == "#" ]] && continue

  if [[ -z "${url:-}" || -z "${sha:-}" || -z "${filename:-}" ]]; then
    echo "ERROR:fetch-linux1-sources:invalid manifest row for $name"
    exit 2
  fi

  out="$OUTDIR/$filename"
  if [[ ! -f "$out" ]]; then
    if ! curl -fsSL "$url" -o "$out"; then
      echo "ERROR:fetch-linux1-sources:download failed for $name from $url"
      exit 1
    fi
  fi

  actual_sha="$(sha256sum "$out" | awk '{print $1}')"

  if [[ "$sha" == "AUTO_SHA256" ]]; then
    echo "AUTO_SHA256:$name:$actual_sha"
    missing_hash=1
    continue
  fi

  if [[ "$actual_sha" != "$sha" ]]; then
    echo "ERROR:fetch-linux1-sources:checksum mismatch for $name"
    echo "expected=$sha"
    echo "actual=$actual_sha"
    exit 2
  fi

  echo "OK:fetch-linux1-sources:$name"
done < "$MANIFEST"

if [[ "$missing_hash" -ne 0 ]]; then
  echo "ERROR:fetch-linux1-sources:auto sha placeholders remain in manifest"
  exit 2
fi
