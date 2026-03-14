# Repo Ownership Reorg Design

## Summary

Reorganize the repository around five operating-system roots:

- `c-os/`
- `rust-os/`
- `linux-1.0.0/`
- `linux-lab/`
- `xv6/`

Keep shared infrastructure at the top level:

- `scripts/`
- `tests/`
- `qemu/`
- `tools/`
- `docs/`
- `build/`

This is an ownership-first cleanup. It moves clearly OS-owned content under the owning OS directory, keeps shared helpers top-level, and preserves the existing top-level `make` entrypoints.

## Goals

- Make OS ownership obvious from the repository layout.
- Move Linux 1.0.0 assets under `linux-1.0.0/` instead of scattering them across top-level directories.
- Keep `c-os/`, `rust-os/`, `linux-lab/`, and `xv6/` as first-class OS roots.
- Preserve current user-facing `make` targets.
- Update tests and docs in the same change so the repo remains internally consistent.

## Non-Goals

- Do not move shared `qemu/` or `tools/` content into an OS directory in this pass.
- Do not redesign build behavior beyond path and ownership cleanup.
- Do not split top-level tests by OS; tests remain top-level unless there is a strong reason to move them later.
- Do not force all OS-specific helpers under OS directories if keeping them grouped under shared top-level `scripts/` is cleaner.

## Target Layout

### Top-Level Roots

The repository should keep these top-level directories:

- `c-os/`
- `rust-os/`
- `linux-1.0.0/`
- `linux-lab/`
- `xv6/`
- `scripts/`
- `tests/`
- `qemu/`
- `tools/`
- `docs/`
- `build/`

### Linux 1.0.0 Ownership Moves

Move these Linux 1.0.0 assets under `linux-1.0.0/`:

- `linux1-userspace/` → `linux-1.0.0/userspace/`
- `manifests/linux1-sources.lock` → `linux-1.0.0/manifests/linux1-sources.lock`
- `patches/linux-1.0.0/` → `linux-1.0.0/patches/kernel/`
- `patches/lilo/` → `linux-1.0.0/patches/lilo/`

### Script Grouping

Keep `scripts/` top-level, but group owned scripts by subsystem:

- `scripts/linux1/fetch_sources.sh`
- `scripts/linux1/build_kernel.sh`
- `scripts/linux1/build_userspace.sh`
- `scripts/linux1/build_lilo.sh`
- `scripts/linux1/mk_disk.sh`
- `scripts/linux1/run_qemu.sh`
- `scripts/linux1/run_curses.sh`
- `scripts/linux1/verify_provenance.sh`
- `scripts/xv6/build.sh`

This layout keeps shared infrastructure visible while removing the current flat mix of unrelated script names.

## Scope Boundaries

This reorg applies the ownership model unevenly by design:

- `linux-1.0.0/` gets the main ownership cleanup because it currently has the most top-level spillover.
- `xv6/` only needs script regrouping in this pass.
- `c-os/`, `rust-os/`, and `linux-lab/` already have primary OS roots, so they are not being restructured unless an asset is clearly misplaced.
- `qemu/` and `tools/` remain top-level because they still serve more than one subsystem.

The result is cleaner ownership without speculative moves.

## Compatibility Model

The reorg must preserve current operator entrypoints:

- `make linux1`
- `make linux1-curses`
- `make test-linux1`
- `make xv6`
- `make xv6-clean`
- `make test-xv6`

Top-level targets remain stable. The implementation updates the internal paths they call.

Where an old script path may still matter during migration, a thin compatibility wrapper is allowed for one pass. The Makefile and tests should move to the new grouped paths immediately.

## Internal Path Mapping

### Makefile Recipe Mapping

Linux 1.0.0 targets should stop calling flat script names and instead call:

- `scripts/linux1/fetch_sources.sh`
- `scripts/linux1/build_kernel.sh`
- `scripts/linux1/build_userspace.sh`
- `scripts/linux1/build_lilo.sh`
- `scripts/linux1/mk_disk.sh`
- `scripts/linux1/run_qemu.sh`
- `scripts/linux1/run_curses.sh`
- `scripts/linux1/verify_provenance.sh`

`xv6` targets should call:

- `scripts/xv6/build.sh`

### Linux 1.0.0 Script Path Mapping

Linux 1.0.0 scripts should resolve these locations from the repository root:

- `ROOT/linux1-userspace/src` → `ROOT/linux-1.0.0/userspace/src`
- `ROOT/manifests/linux1-sources.lock` → `ROOT/linux-1.0.0/manifests/linux1-sources.lock`
- `ROOT/patches/linux-1.0.0` → `ROOT/linux-1.0.0/patches/kernel`
- `ROOT/patches/lilo` → `ROOT/linux-1.0.0/patches/lilo`

Scripts should keep deriving paths from the repository root, not from the current working directory.

## Error Handling And Migration Safety

This is a compatibility-preserving reorg, not a flag day rewrite.

Rules:

- Update paths in scripts, Makefile targets, docs, and tests in the same change.
- Prefer direct failures on missing paths over silent fallback behavior.
- Keep path derivation root-relative so moved files remain robust.
- Allow tiny compatibility wrappers only where they reduce migration risk for user-facing paths.

Expected failure mode:

- if an old path remains referenced, path-assertion tests should fail clearly
- build scripts should not silently create new incorrect directories because a relative path changed underneath them

## Testing Strategy

Update tests to assert the new ownership model directly.

### Linux 1.0.0 Tests

- `tests/test_linux1_userspace_layout.py`
  - assert `linux-1.0.0/userspace/...`
  - stop asserting `linux1-userspace/...`
- `tests/test_linux1.py`
  - assert manifests and patches under `linux-1.0.0/...`
- `tests/test_linux1_make_targets.py`
  - assert `scripts/linux1/...` paths

### xv6 Tests

- `tests/test_xv6.py`
  - assert `scripts/xv6/build.sh`

### Repo Layout Tests

- `tests/test_repo_layout.py`
  - assert the new ownership layout
  - assert that old top-level Linux 1.0.0 ownership directories are gone

### Regression Coverage

- existing `linux-lab` tests must remain green
- existing top-level `make` target assertions must remain green

## Implementation Constraints

- Use the existing OS roots; do not invent new top-level subsystem names.
- Keep shared infrastructure top-level unless ownership is clearly single-OS.
- Prefer path-only changes over behavioral changes.
- Keep the reorg understandable: each moved asset should have an obvious owner.

## Success Criteria

The reorg is complete when all of the following are true:

- the repository presents five clear OS roots: `c-os/`, `rust-os/`, `linux-1.0.0/`, `linux-lab/`, and `xv6/`
- Linux 1.0.0-owned userspace, manifests, and patches live under `linux-1.0.0/`
- Linux 1.0.0 and xv6 scripts are grouped under `scripts/linux1/` and `scripts/xv6/`
- top-level `make` targets stay stable
- repo layout tests and Linux 1.0.0/xv6 path tests pass
- unrelated shared infrastructure remains top-level
