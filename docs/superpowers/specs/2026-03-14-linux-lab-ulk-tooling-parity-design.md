# Linux Lab ULK Tooling Parity Design

**Date:** 2026-03-14
**Status:** User-approved design draft

## Overview

Tighten the `linux-lab/` port so it covers the standalone non-kernel surface that the original `../qulk/ulk` script actually uses, without importing the broad `../qulk/userspace/` tree.

This pass is narrower than the earlier example-parity import. It does not expand scope to every userspace app or library in `qulk`. It only closes the gap between what `ulk` drives today and what `linux-lab` exposes through its orchestrator and helper structure.

The audited `ulk` surface is:

- external upstream tools: `crash`, `cgdb`, `libbpf-bootstrap`, `retsnoop`
- kernel in-tree tools: `tools/libapi` and `make -C <kernel>/tools O=<build> subdir=tools all`
- repo-local userspace build: `modules/rust_learn/user`

## Relationship To Existing Linux Lab Design

This spec builds on the existing `linux-lab` architecture in [docs/superpowers/specs/2026-03-14-linux-lab-design.md](/home/ubuntu/work/qos/docs/superpowers/specs/2026-03-14-linux-lab-design.md) and the earlier example/script import in [docs/superpowers/specs/2026-03-14-linux-lab-example-script-parity-design.md](/home/ubuntu/work/qos/docs/superpowers/specs/2026-03-14-linux-lab-example-script-parity-design.md).

The earlier work already established:

- `linux-lab/` as the top-level subsystem
- `linux-lab/tools/` for manifest-managed upstream tooling
- `linux-lab/examples/` for repo-local samples
- `build/linux-lab/` as the artifact root
- `linux-lab/bin/ulk` as the compatibility entrypoint

This spec narrows in on the remaining tooling and userspace parity needed for faithful `ulk` behavior.

## User-Confirmed Constraints

- Only port the standalone parts that the original `ulk` script actually used.
- Do not import the whole `../qulk/userspace/` tree.
- Adapt the result to the `linux-lab` structure rather than restore the old repo layout.
- Include the external tools, but keep them manifest-managed rather than vendoring their source into git.
- Pull in repo-local support code only if the `ulk`-used flows truly require it.

## Goals

- Make `linux-lab` cover the full standalone non-kernel surface that `ulk` used.
- Preserve the current `linux-lab` structure and stage ownership.
- Represent kernel in-tree tools as first-class planned artifacts.
- Keep repo-local sample userspace with its owning example tree.
- Avoid importing unrelated standalone userspace trees that `ulk` never called.

## Non-Goals

- Port every app, library, or tool under `../qulk/userspace/`.
- Vendor upstream tool repositories into this repo.
- Reorganize sample-owned userspace out of `linux-lab/examples/`.
- Claim live guest validation for every guest-built tool in this pass.

## Current-State Observations

The current `linux-lab` tree already includes most of the right structure:

- [linux-lab/tools](/home/ubuntu/work/qos/linux-lab/tools) contains manifests for `crash`, `cgdb`, `libbpf-bootstrap`, and `retsnoop`
- [linux-lab/orchestrator/stages/build_tools.py](/home/ubuntu/work/qos/linux-lab/orchestrator/stages/build_tools.py) already resolves and executes external tool plans
- [linux-lab/orchestrator/stages/build_examples.py](/home/ubuntu/work/qos/linux-lab/orchestrator/stages/build_examples.py) already resolves repo-local example builds
- [linux-lab/examples/rust/rust_learn/user](/home/ubuntu/work/qos/linux-lab/examples/rust/rust_learn/user) already exists as the repo-local userspace app path that matches `ulk`

The remaining mismatch is not broad repository shape. It is fidelity:

- `build-tools` does not yet model the kernel in-tree tool work that `ulk` runs
- `crash` needs explicit support for repo-local extension assets copied into `extensions/`
- the stage metadata does not yet make the `ulk`-used tool surface explicit
- no evidence shows that `ulk` itself requires the wider standalone `../qulk/userspace/*` trees

## Architecture

Keep one `linux-lab` control path and extend only the surfaces that map directly to `ulk`.

Repository layout:

- upstream external tools stay in [linux-lab/tools](/home/ubuntu/work/qos/linux-lab/tools) as manifest-managed checkouts
- repo-local sample-owned userspace stays in [linux-lab/examples](/home/ubuntu/work/qos/linux-lab/examples)
- no new top-level `linux-lab/userspace/` tree is introduced unless a missing repo-local helper is proven necessary by the `ulk` audit
- kernel `tools/*` are treated as build products of the selected kernel workspace, not as imported source under `linux-lab`

Execution ownership:

- `build-tools` owns upstream tools and kernel in-tree tools
- `build-examples` owns repo-local sample userspace such as `rust_learn/user`
- `build-image` and `boot` remain unchanged except for consuming clearer artifact metadata

This keeps the port small and faithful. The lab remains organized by source ownership rather than by whether something happens to produce a userland binary.

## Components

### `build-tools`

`build-tools` gains an explicit kernel-tools sub-plan alongside the existing external tool plan.

It must model three kinds of work:

- external tool checkout and prepare
- kernel in-tree tool build commands
- tool-side post-prepare actions such as copying `crash` extension assets

The stage output metadata should separate these concerns clearly so dry-run and live runs show what will happen.

### Tool manifests

The manifests under [linux-lab/tools](/home/ubuntu/work/qos/linux-lab/tools) remain the source of truth for upstream tools.

Required fidelity updates:

- `crash` must record the extension asset copy behavior that `ulk` performed after `gdb_unzip`
- `cgdb` remains host-built
- `libbpf-bootstrap` remains manifest-managed, but its default behavior stays limited to checkout or light prepare unless a profile explicitly requests more
- `retsnoop` remains manifest-managed under the same rule

The manifest schema may grow modestly, for example with `post_prepare_commands` or `asset_copy` fields, but it should stay flat and easy to inspect.

### `build-examples`

`build-examples` remains the owner of repo-local example userspace.

For this pass, the explicit `ulk` requirement is:

- build [linux-lab/examples/rust/rust_learn/user](/home/ubuntu/work/qos/linux-lab/examples/rust/rust_learn/user) when the matching Rust example path is in scope

If a later audit proves that `ulk` depended on a small repo-local helper outside example trees, that helper may be added surgically. Broad standalone userspace imports remain out of scope.

## Data Flow

The request pipeline stays the same through `fetch`, `patch`, `configure`, and `build-kernel`.

The `ulk`-relevant flow after that is:

1. `build-tools` resolves external tool groups from kernel and profile manifests
2. `build-tools` resolves a kernel-tools sub-plan from the selected kernel workspace
3. `build-tools` records post-prepare actions such as `crash` extension asset copy
4. `build-examples` resolves repo-local example plans, including `rust_learn/user`
5. later stages consume artifact metadata but do not change ownership

Planned artifacts should be reported in stage metadata as:

- `external_tools`: manifest-backed checkout/build entries
- `kernel_tools`: in-tree kernel tool build entries
- `example_userspace`: repo-local userspace build entries tied to example ownership

Artifact locations:

- external tool workspaces stay under `build/linux-lab/tools/<key>/`
- kernel in-tree tool outputs stay in the selected kernel build workspace and are referenced by path in stage metadata
- repo-local userspace outputs stay with the example workspace and are referenced through `examples-plan`

## Manifest And Metadata Contract

The minimum metadata contract for this pass is:

- `build-tools` state must list resolved external tools by key
- `build-tools` state must list the kernel in-tree commands it will run
- `build-tools` state must expose any extra asset-copy or post-prepare steps
- `build-examples` state must continue to show the repo-local userspace entries it will build

Dry-run output must make the following visible without reading code:

- whether `crash`, `cgdb`, `libbpf-bootstrap`, and `retsnoop` are in scope
- whether kernel `tools/libapi` and `subdir=tools all` are in scope
- whether `rust_learn/user` is in scope

## Error Handling

This pass follows the existing stage-scoped error model.

Required behavior:

- missing repo-local extension assets for `crash` fail the tool stage with a specific error message
- missing kernel workspace paths needed for in-tree tool builds fail before command execution
- disabled or minimal profiles keep omitting optional tool and example work without producing errors
- tool manifest schema errors fail early during planning rather than deep inside a live run

The failure unit remains the stage. A failed tool step should write its log and state without erasing previously completed outputs.

## Testing

Add or update tests in four areas.

Tool manifest tests:

- verify the `crash` manifest expresses its extra repo-local setup
- verify existing tool manifests still resolve cleanly

Planner and metadata tests:

- verify `build-tools` dry-run metadata includes external tool entries and kernel in-tree tool entries
- verify `build-examples` still includes `rust_learn/user` through the catalog-driven flow
- verify minimal profiles still omit optional tooling and examples

Fixture-backed live executor tests:

- verify post-prepare actions run for a tool plan
- verify kernel-tool commands are emitted and executed in the selected workspace
- verify repo-local userspace builds still run from the example-owned path

Regression tests:

- keep current runtime/network helper tests green
- keep existing broad example and tool catalog tests green

## Success Criteria

This pass is complete when:

- `linux-lab` covers the full standalone surface that `ulk` actually used
- no broad `../qulk/userspace/*` import is added without a proven dependency
- `build-tools` metadata clearly reports external tools and kernel in-tree tools
- `build-examples` continues to own `rust_learn/user`
- dry-run and fixture-backed live tests prove that coverage

## Implementation Notes

This work should prefer extension of existing structures over new subsystems:

- extend the current tool manifest schema instead of adding a second tool catalog
- extend `build-tools` instead of creating a dedicated kernel-tools stage
- keep example-owned userspace in the example catalog rather than mirroring it elsewhere

That is the shortest path to faithful `ulk` parity with the least structural churn.
