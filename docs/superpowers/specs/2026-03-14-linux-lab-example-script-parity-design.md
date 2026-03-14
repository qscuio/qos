# Linux Lab Example And Script Parity Design

**Date:** 2026-03-14
**Status:** User-approved design draft

## Overview

Extend `linux-lab/` so it carries the full breadth of the sample and operator-helper surface that currently lives in `../qulk/modules/` and the relevant guest/runtime parts of `../qulk/scripts/`.

This work does not revert to the old `qulk` repository layout. It adapts the imported content to the `linux-lab` structure introduced in the earlier port design. The import preserves breadth first, then expands build enablement through catalog-driven orchestration.

This design covers two related gaps:

- example parity: kernel modules, userspace helpers, Rust samples, and BPF samples
- runtime/helper parity: SSH, guest-run, monitor, reboot, QMP, and network/TAP helper scripts

## Relationship To Existing Linux Lab Design

The base architecture remains the one defined in [docs/superpowers/specs/2026-03-14-linux-lab-design.md](/home/ubuntu/work/qos/docs/superpowers/specs/2026-03-14-linux-lab-design.md):

- `linux-lab/` stays a top-level subsystem
- the manifest-driven orchestrator remains the control plane
- `build/linux-lab/` remains the artifact root
- `bin/ulk` remains the compatibility entrypoint

This spec narrows in on the unfinished example and script parity work that the original design left to later phases.

## User-Confirmed Constraints

- The new content should adapt to the `linux-lab` structure rather than copying the old `qulk` layout as-is.
- The first parity pass should preserve breadth first.
- Both the sample corpus and the missing guest/network helper scripts are required.
- The import should preserve content even when not every sample is wired into automated builds on day one.

## Current-State Observations

The current `linux-lab` tree contains a curated example subset and a minimal runtime-helper surface:

- `linux-lab/examples/modules/` currently has a small `simple`, `ioctl`, and `debug` subset
- `linux-lab/examples/userspace/` currently has three C helpers
- `linux-lab/examples/rust/` currently carries `rust_learn`
- `linux-lab/examples/bpf/` currently carries a five-file learning subset
- `linux-lab/scripts/` currently carries `boot.sh`, `create-image.sh`, `connect`, `gdb`, `up.sh`, and `down.sh`

The source repository `../qulk` is much broader:

- `../qulk/modules/` currently has 71 top-level sample trees
- 66 of those trees have a `Makefile`
- 23 include custom load or runtime scripts
- 21 already show non-trivial integration traits such as userland helpers, git submodules, `Kconfig`, CMake, Docker, or dedicated test directories
- `../qulk/scripts/` contains the broader guest/runtime helper set such as `guest_run`, `guest_insmod`, `guest_rmmods`, `monitor`, `reboot`, `qmp`, `tap_and_veth_set.sh`, and `tap_and_veth_unset.sh`

The current example planner in [linux-lab/orchestrator/core/examples.py](/home/ubuntu/work/qos/linux-lab/orchestrator/core/examples.py) is hardcoded to four coarse groups. That design is too narrow for full sample parity.

## Goals

- Import the full breadth of `../qulk/modules/` into `linux-lab/examples/` using the new repo structure.
- Import the missing guest/runtime and network helper scripts from `../qulk/scripts/` into `linux-lab/scripts/`.
- Preserve enough local structure inside each imported sample tree that existing sample-local build assumptions keep working.
- Add a catalog layer so `linux-lab` can enumerate every imported sample even when some are not yet build-enabled.
- Replace the current hardcoded example planner with catalog-driven resolution.
- Preserve the existing curated example workflows while expanding the available corpus.

## Non-Goals

- Guarantee that every imported sample is build-enabled in the first parity import.
- Rewrite each imported sample into a fully normalized house style.
- Turn every script from `../qulk/scripts/` into a first-class orchestrator stage.
- Import unrelated `qulk` utilities that are not part of the kernel-lab operator workflow.

## Repository Layout

The expanded structure should look like this:

```text
linux-lab/
├── examples/
│   ├── modules/
│   │   ├── core/
│   │   ├── debug/
│   │   ├── drivers/
│   │   ├── memory/
│   │   ├── network/
│   │   ├── tracing/
│   │   └── ...
│   ├── userspace/
│   │   ├── app/
│   │   ├── module-tests/
│   │   └── ...
│   ├── rust/
│   │   ├── rust/
│   │   ├── rust-learn/
│   │   └── ...
│   └── bpf/
│       ├── learn/
│       ├── xdp/
│       └── ...
├── catalog/
│   └── examples/
│       ├── simple.yaml
│       ├── ioctl.yaml
│       ├── rust_learn.yaml
│       └── ...
└── scripts/
    ├── runtime/
    │   ├── connect
    │   ├── guest_run
    │   ├── guest_insmod
    │   ├── guest_rmmods
    │   ├── monitor
    │   ├── reboot
    │   └── qmp
    ├── network/
    │   ├── up.sh
    │   ├── down.sh
    │   ├── tap_and_veth_set.sh
    │   └── tap_and_veth_unset.sh
    ├── connect
    ├── guest_run
    ├── guest_insmod
    ├── guest_rmmods
    ├── monitor
    ├── reboot
    └── qmp
```

The top-level wrappers in `linux-lab/scripts/` are compatibility entrypoints. They should delegate into the structured `runtime/` or `network/` subdirectories.

## Import Strategy

The chosen import strategy is hybrid:

- outer placement is normalized into the `linux-lab` hierarchy
- inner sample trees stay largely intact
- per-sample metadata lives beside the imported content in catalog files

That gives the port three useful properties:

1. the repo layout becomes consistent and discoverable
2. sample-local build assumptions remain stable enough for phased enablement
3. catalog metadata can evolve without rewriting the imported source trees

## Example Taxonomy

Imported samples are grouped by kind and then by category.

Kind buckets:

- `module`
- `userspace`
- `rust`
- `bpf`

Category buckets are descriptive rather than exact mirrors of old directory names. Initial categories should be chosen from the dominant topic of each sample:

- `core`
- `debug`
- `drivers`
- `memory`
- `network`
- `tracing`
- `security`
- `filesystem`
- `concurrency`
- `io`
- `experimental`

The categorization goal is discoverability, not perfect ontology. If a sample does not fit cleanly, place it in the least-wrong category and express the nuance in catalog tags.

## Example Catalog Model

Every imported sample gets a catalog file in `linux-lab/catalog/examples/`.

Minimum fields:

- `key`: stable identifier
- `kind`: `module`, `userspace`, `rust`, or `bpf`
- `category`: normalized category name
- `source`: imported source path under `linux-lab/examples/`
- `origin`: original `../qulk/...` path
- `build_mode`: one of `kbuild-module`, `gcc-userspace`, `rust-user`, `bpf-clang`, `custom-make`, or `disabled`
- `build_workdir`: optional relative working directory
- `build_commands`: optional explicit commands for custom cases
- `runtime_helpers`: list of helper scripts relevant to the sample
- `requires`: host tools, kernel config notes, or runtime assumptions
- `tags`: descriptive discovery tags
- `enabled`: boolean indicating whether the orchestrator should build it by default
- `notes`: freeform migration note

Catalog rules:

- every imported sample must have a catalog entry
- imported but unsupported samples must still be cataloged with `enabled: false`
- catalog keys must be unique
- the catalog is the source of truth for orchestrator enumeration

## Script Parity Model

The required script import scope is limited to operator workflows tied to the kernel lab.

Required runtime helpers:

- `connect`
- `guest_run`
- `guest_insmod`
- `guest_rmmods`
- `monitor`
- `reboot`
- `qmp`
- `gdb`

Required network helpers:

- `up.sh`
- `down.sh`
- `tap_and_veth_set.sh`
- `tap_and_veth_unset.sh`

Script rules:

- imported scripts should prefer environment-driven configuration over hardcoded keys or paths
- existing `linux-lab` behavior must not regress
- top-level wrappers should preserve the familiar operator command names
- shared implementation details should live in `runtime/` or `network/`, not in duplicated wrapper logic

## Orchestrator Changes

The example planner must move from hardcoded group functions to catalog-driven resolution.

Current limitation:

- the planner currently knows only `modules-core`, `userspace-core`, `rust-core`, and `bpf-core`

Target behavior:

- load the full example catalog
- filter by requested example groups, tags, enabled state, and profiles
- return build plans derived from catalog metadata rather than embedded Python lists

The initial migration keeps the existing group names for backward compatibility, but each group resolves through catalog membership instead of hardcoded paths.

Compatibility rule:

- existing curated workflows such as `default-lab` remain valid
- those profiles simply resolve to larger catalog selections over time

## Data Flow

The expanded example flow is:

1. import sample trees under `linux-lab/examples/...`
2. catalog each imported sample under `linux-lab/catalog/examples/*.yaml`
3. resolve profiles into example groups
4. map groups into catalog entries
5. filter catalog entries by `enabled`
6. derive build commands from `build_mode` or `build_commands`
7. emit stage metadata for both:
   - all discovered catalog entries in scope
   - the enabled subset selected for build execution

This split is important. It lets the system preserve breadth immediately while keeping execution conservative.

## Error Handling

Catalog loading and helper resolution should fail clearly.

Required validation:

- missing catalog files for imported samples are errors
- duplicate catalog keys are errors
- unknown `build_mode` values are errors
- enabled samples without a resolvable build plan are errors
- wrapper scripts that point to missing runtime/network implementations are errors

Runtime behavior:

- disabled samples are reported in metadata but skipped during build execution
- custom sample failures should identify the exact catalog key and command that failed
- missing optional runtime helpers for a sample should be recorded in metadata rather than silently ignored

## Testing

Testing is split into four layers.

Repository and import coverage:

- confirm the full expected script surface exists under `linux-lab/scripts/`
- confirm all imported sample trees exist under `linux-lab/examples/`
- confirm catalog coverage matches the imported sample set

Catalog loading tests:

- confirm unique keys
- confirm valid kind/category/build mode values
- confirm catalog-driven planners can enumerate enabled and disabled samples correctly

Planner tests:

- confirm current profiles still resolve curated builds
- confirm disabled imported samples do not run in build stages
- confirm custom `build_workdir` and `build_commands` cases are honored

Runtime helper tests:

- confirm `connect`, `guest_run`, `guest_insmod`, `guest_rmmods`, `monitor`, `reboot`, and `qmp` wrappers dispatch correctly
- confirm network helper wrappers expose the expected script locations

## Implementation Phases

This parity expansion should be implemented in three passes.

### Pass 1: Breadth Import

- import all relevant sample trees from `../qulk/modules/` into `linux-lab/examples/`
- import the missing runtime/network helper scripts into `linux-lab/scripts/runtime/` and `linux-lab/scripts/network/`
- add top-level compatibility wrappers in `linux-lab/scripts/`
- create catalog files for every imported sample
- mark only the currently known-good subset as `enabled: true`

### Pass 2: Catalog-Driven Orchestration

- replace hardcoded example planning with catalog-driven resolution
- emit richer stage metadata for discovered versus enabled samples
- keep current curated profile behavior stable

### Pass 3: Build Enablement Expansion

- enable more imported samples in batches
- add special-case planners for samples with tests, userland helpers, or custom build flows
- grow profile coverage without destabilizing the base lab workflows

## Acceptance Criteria

This parity expansion is complete when:

- every relevant sample tree from `../qulk/modules/` exists under `linux-lab/examples/`
- every imported sample has a catalog entry
- the missing guest/runtime and network helper scripts exist under `linux-lab/scripts/`
- current curated workflows still pass
- the orchestrator resolves examples through the catalog rather than hardcoded path lists
- disabled samples remain visible in metadata until later enablement work turns them on
