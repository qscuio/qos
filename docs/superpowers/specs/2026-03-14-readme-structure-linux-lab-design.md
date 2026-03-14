# README Structure And Linux-Lab Usage Design

## Summary

Rewrite the top-level `README.md` so it matches the current repository shape.

The new README should:

- lead with the repo structure
- explain the five OS roots
- keep shared top-level infrastructure visible
- provide short usage guidance per OS
- improve the `linux-lab` section so native CLI usage and `ulk` usage are easy to find

This is a README-only change. It is not a broader docs reorganization.

## Goals

- Make the repository layout easy to understand from the top-level README.
- Reflect the current ownership model:
  - `c-os/`
  - `rust-os/`
  - `linux-1.0.0/`
  - `linux-lab/`
  - `xv6/`
- Document shared top-level directories:
  - `scripts/`
  - `tests/`
  - `qemu/`
  - `tools/`
  - `docs/`
  - `build/`
- Give a compact quick-start for each OS root.
- Make `linux-lab` usage task-oriented and concrete.

## Non-Goals

- Do not rewrite other docs in this pass.
- Do not add deep architecture discussion that belongs in `docs/`.
- Do not duplicate every advanced `linux-lab` profile, stage, or artifact detail.
- Do not turn the README into a complete operator manual.

## Target README Structure

The README should use this high-level order:

1. Overview
2. Repo Structure
3. Prerequisites
4. Quick Start
5. OS Guides
6. Shared Build And Test
7. Manual QEMU Notes

## Repo Structure Section

The first major section after the overview should describe the repository layout.

It should include one short description for each OS root:

- `c-os/`
- `rust-os/`
- `linux-1.0.0/`
- `linux-lab/`
- `xv6/`

It should also include a short shared-infrastructure list:

- `scripts/`
- `tests/`
- `qemu/`
- `tools/`
- `docs/`
- `build/`

The purpose is orientation, not exhaustive detail.

## OS Guides Section

The README should include one compact subsection per OS:

- `c-os`
- `rust-os`
- `linux-1.0.0`
- `linux-lab`
- `xv6`

Each subsection should include:

- one sentence about what that subtree is for
- one or two concrete commands
- artifact or output notes only if they help first use

## Linux 1.0.0 Content

The Linux 1.0.0 subsection should mention:

- build and boot entrypoint: `make linux1`
- grouped Linux1 scripts under `scripts/linux1/`
- owned content under `linux-1.0.0/`:
  - `userspace/`
  - `manifests/`
  - `patches/`

It should not repeat the full provenance or historical design material that already lives in `docs/`.

## Linux-Lab Content

The `linux-lab` subsection should be task-oriented.

It should explicitly show:

- native CLI location: `linux-lab/bin/linux-lab`
- compatibility shim: `linux-lab/bin/ulk`
- main make targets:
  - `make linux-lab-validate`
  - `make linux-lab-plan`
  - `make linux-lab-run`
- one direct `ulk` example:
  - `linux-lab/bin/ulk arch=x86_64 kernel=6.9.8 mirror=sg`
- one note on profiles:
  - `default-lab` is the curated path
  - `full-samples` expands to the broad imported sample set
- artifact location:
  - `build/linux-lab/requests/`

The section should emphasize common first actions:

- validate a request
- generate a plan
- run a dry-run workflow

## Content Boundaries

The README should stay concise.

Include:

- repo layout
- short per-OS usage
- concrete commands that work today
- a clearer `linux-lab` front door

Exclude:

- long history
- design rationale already recorded elsewhere
- deep lab internals
- exhaustive command listings for every subsystem

## Verification

This change should verify README accuracy with lightweight checks:

- confirm the README uses the current grouped Linux1 and xv6 script paths
- confirm the README does not reintroduce removed flat Linux1 script names
- run `make -n linux1`
- run `make -n xv6`
- run a cheap `linux-lab` command only if it remains lightweight in this environment

## Success Criteria

The README update is complete when:

- the README starts with the repo structure rather than mixed workflow detail
- all five OS roots are documented
- the shared top-level directories are documented
- `linux-lab` usage is clearer and more actionable
- the README remains concise enough to serve as a front door
