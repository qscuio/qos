# Linux Lab Modern Kernel Lab Port Design

**Date:** 2026-03-14
**Status:** Design approved, spec draft under review

## Overview

Port the modern Linux kernel lab workflow from `../qulk` into this repository as a new top-level subsystem named `linux-lab/`. Preserve the functions that make `qulk` useful for kernel learning and testing: building and patching target kernels, creating bootable guest images, running them in QEMU, and keeping related kernel module, userspace, Rust, and BPF learning projects usable against those kernels.

The port should not copy `qulk` as-is. The new system should keep the capabilities while reorganizing them around a manifest-driven orchestrator. The familiar `ulk` entrypoint remains available as a compatibility shim.

This design covers the full program architecture and its decomposition into implementation phases. The next implementation plan must be limited to **Phase 1: scaffold and orchestrator bootstrap**.

## User-Confirmed Constraints

- The port target is the full modern-kernel lab stack, not only a reduced build-and-boot core.
- Compatibility target is full current `qulk` parity, including multi-arch support and the existing version-specific patch matrix.
- `qos` keeps its current identity and structure. `linux-lab/` is added as a new top-level subsystem rather than replacing the existing `c-os/`, `rust-os/`, `xv6/`, or `linux-1.0.0/` work.
- Existing `qulk` habits do not need to be preserved verbatim if the functions remain available.
- The top-level subsystem name is `linux-lab`.
- The chosen architecture is a manifest-driven orchestrator rather than a direct shell-script port.

## Current-State Observations

`../qulk` currently concentrates several concerns in `ulk` and a large `scripts/` tree:

- kernel source fetch, patch, configure, and build
- arch selection for `x86_64` and `arm64`
- QEMU image creation and boot orchestration
- debug tool acquisition and setup (`crash`, `cgdb`, `gdb`, `qmp`, frontend helpers)
- per-kernel patch sets under `patch/linux-*/kernel.patch`
- baseline configs under `configs/x86_64/defconfig` and `configs/arm64/defconfig`
- many example projects under `modules/`, including plain LKMs, userspace tests, Rust kernel samples, tracing examples, and BPF/XDP work

The current `qos` repository is structured as a multi-OS workspace with root-level orchestration, docs, smoke tests, and artifact directories. That existing structure should remain stable while `linux-lab/` adopts the repo's documentation, testing, and build conventions.

## Goals

- Add a new `linux-lab/` subsystem that can reproduce the functional surface of the current `qulk` lab.
- Replace the monolithic `ulk` shell flow with a manifest-driven orchestrator that makes behavior explicit by kernel, arch, image, and feature profile.
- Preserve support for version-specific kernel patch sets and multi-arch build and boot workflows.
- Preserve the learning workflows for kernel modules, userspace test apps, Rust kernel development, and BPF development.
- Keep artifacts, logs, and intermediate state in predictable locations under `build/linux-lab/`.
- Make the system testable in units and in representative integration flows.

## Non-Goals

- Rebuild `qulk` as a byte-for-byte mirror inside this repository.
- Refactor unrelated parts of `qos`.
- Guarantee that every historical helper script in `qulk/scripts/` becomes a first-class orchestrator stage.
- Solve every future kernel-lab extension in the first implementation phase.

## Program Decomposition

This project is too broad to execute as one undifferentiated plan. It is decomposed into five implementation phases:

1. **Phase 1: Scaffold and orchestrator bootstrap**
   Create `linux-lab/`, add the new CLI/runtime, define manifests and stage interfaces, and provide the `bin/ulk` compatibility shim.
2. **Phase 2: Kernel source, config, and patch parity**
   Port kernel manifests, config fragments, source fetch logic, and the current patch matrix.
3. **Phase 3: Image and QEMU parity**
   Port image recipes, guest provisioning, QEMU launch profiles, and the modern guest boot path.
4. **Phase 4: Example and feature parity**
   Reorganize modules, userspace helpers, Rust samples, and BPF projects into `linux-lab/examples/` and preserve buildability.
5. **Phase 5: Debug and workflow parity**
   Port core debug workflows and selected support tools needed by the kernel lab.

The next planning step after this spec review is limited to **Phase 1 only**. Later phases will follow with their own plans after the scaffold exists.

## Repository Layout

```text
qos/
├── linux-lab/
│   ├── bin/
│   │   └── ulk                     # compatibility entrypoint
│   ├── orchestrator/
│   │   ├── cli/                    # native command entrypoints
│   │   ├── core/                   # manifest loading, resolution, state, logging
│   │   └── stages/                 # fetch, prepare, patch, configure, build, image, boot
│   ├── manifests/
│   │   ├── kernels/
│   │   ├── arches/
│   │   ├── images/
│   │   └── profiles/
│   ├── fragments/                  # kernel config fragments by feature bundle
│   ├── patches/                    # migrated per-kernel patch sets
│   ├── configs/                    # retained baseline config assets where needed
│   ├── examples/
│   │   ├── modules/
│   │   ├── userspace/
│   │   ├── rust/
│   │   ├── bpf/
│   │   └── debug/
│   ├── images/                     # rootfs and guest-image recipes
│   ├── tools/                      # external tool source/build metadata
│   ├── scripts/                    # small leaf helpers only
│   └── docs/
├── build/
│   └── linux-lab/                  # generated sources, kernels, images, logs, state
└── docs/
    └── superpowers/
        └── ...
```

## Architecture

`linux-lab` uses a manifest-driven orchestrator. A resolved build request is described by four inputs:

- a target kernel manifest
- an architecture manifest
- an image manifest
- one or more feature profiles

The orchestrator is responsible for policy and state. Shell helpers are allowed only for narrow system-facing leaf operations such as invoking `make`, `debootstrap`, or QEMU.

### Compatibility Entry Point

`linux-lab/bin/ulk` remains available so users can keep familiar forms such as:

```bash
linux-lab/bin/ulk arch=x86_64 kernel=6.9.8 mirror=sg
```

The compatibility entrypoint must:

- accept the existing key-value argument shape used by `qulk`
- normalize arguments into a native request model
- call the orchestrator CLI
- log the resolved request so users can see the manifest-based execution path

It must not contain orchestration policy beyond parsing and translation.

## Manifest Model

The orchestrator uses four manifest families.

### Kernel Manifests

`linux-lab/manifests/kernels/<version>.yaml`

Each kernel manifest defines:

- source URL and checksum
- applicable patch set path
- supported architectures
- default baseline config family
- supported or required feature profiles
- known caveats for that kernel version

The initial parity set should include the versions already present in `qulk`:

- `4.19.317`
- `6.4.3`
- `6.9.6`
- `6.9.8`
- `6.10`

### Architecture Manifests

`linux-lab/manifests/arches/<arch>.yaml`

Each arch manifest defines:

- normalized arch name
- toolchain prefix rules
- kernel image artifact name
- QEMU machine and CPU defaults
- boot-device and console defaults
- guest-image constraints

The initial parity set is:

- `x86_64`
- `arm64`

### Image Manifests

`linux-lab/manifests/images/<release>.yaml`

Each image manifest defines:

- distro release identity
- mirror rules
- debootstrap package set
- provisioning steps
- default guest networking assumptions
- shared-directory and login conventions where still needed

The initial image parity target is driven by the current `qulk` release defaults and the existing `noble` and `jammy` assets.

### Profile Manifests

`linux-lab/manifests/profiles/<name>.yaml`

Each profile groups optional capabilities such as:

- `debug`
- `bpf`
- `rust`
- `samples`
- `crash`
- `minimal`

Profiles own:

- required kernel config fragments
- host dependencies
- post-build steps
- example groups to build
- compatibility constraints by kernel or arch

## Execution Model

Every request flows through the same stage graph:

1. `fetch`
2. `prepare`
3. `patch`
4. `configure`
5. `build-kernel`
6. `build-tools`
7. `build-examples`
8. `build-image`
9. `boot`

Stage execution rules:

- every stage receives a resolved request and declared upstream artifacts
- every stage writes only to its owned subtree under `build/linux-lab/`
- every stage records a machine-readable status file under `build/linux-lab/.../state/`
- reruns may skip clean stages if inputs and manifest fingerprints have not changed
- fallback delegation to retained helper logic is allowed during migration, but the logs must state when a fallback path was used

Canonical artifact root:

`build/linux-lab/<kernel-version>/<arch>/<image>/<profile-set>/`

## Component Boundaries

### `orchestrator/cli`

Native user-facing CLI. Responsible for:

- parsing native commands
- resolving manifests into a request
- selecting stages
- invoking the runtime

### `orchestrator/core`

Shared execution runtime. Responsible for:

- manifest loading and validation
- request resolution
- state and cache bookkeeping
- structured logs
- failure normalization
- stage dependency planning

### `orchestrator/stages`

One implementation module per stage. Each stage owns one concern only:

- `fetch`
- `prepare`
- `patch`
- `configure`
- `build_kernel`
- `build_tools`
- `build_examples`
- `build_image`
- `boot`

### `fragments/`

Kernel config fragments keyed by feature profile rather than inline `sed` edits inside orchestration code.

### `examples/`

Reorganized learning assets from `qulk/modules` and related userland helpers. The top-level categories are:

- `examples/modules/`
- `examples/userspace/`
- `examples/rust/`
- `examples/bpf/`
- `examples/debug/`

### `images/`

Guest image recipes and provisioning assets. Image construction logic must stay separate from kernel build logic.

### `tools/`

External tool source metadata and local build helpers for things like:

- `crash`
- `cgdb`
- `libbpf-bootstrap`
- `retsnoop`

## Asset Migration Rules

The port preserves capability, not old directory shape. Assets from `qulk` are remapped as follows.

- `qulk/patch/linux-*/kernel.patch` -> `linux-lab/patches/linux-*/kernel.patch`
- `qulk/configs/<arch>/defconfig` -> baseline config assets under `linux-lab/configs/` or converted fragments under `linux-lab/fragments/`
- `qulk/modules/*` -> reorganized content under `linux-lab/examples/`
- `qulk/scripts/create-image.sh`, `build_image.sh`, `boot.sh`, and related boot helpers -> image and boot stage backends
- external tool clone logic currently embedded in `ulk` -> declared tool metadata under `linux-lab/tools/`

Repo-local state and machine-specific artifacts from `qulk` must not be ported as design inputs. Examples include:

- `.prepare`
- `.arch`
- `.kernel_version`
- local SSH keys
- built disk images
- VM logs

Those belong in generated state under `build/linux-lab/`.

## Compatibility and Interface Policy

Compatibility is measured at the workflow level:

- users can still request a kernel build and guest boot for supported versions and arches
- patch sets still apply to the same kernel versions
- kernel-learning examples remain buildable against the selected target kernel
- users can still reach a guest system suitable for kernel, userspace, Rust, and BPF experiments

The new layout is allowed to differ from `qulk` as long as the workflow outcome remains available.

## Error Handling

Error handling is stage-scoped and resumable.

- manifest validation fails early for unknown kernel versions, unsupported arch/profile combinations, missing patch files, or incomplete image recipes
- each stage records start, end, input fingerprint, output paths, and failure reason in `state/`
- successful stage outputs are not destroyed automatically on downstream failure
- reruns default to reusing valid upstream outputs and re-executing the failed stage and its downstream dependents
- leaf-tool failures must surface the underlying command, exit code, and relevant log path

Exit semantics:

- non-zero exits from the CLI indicate request failure
- validation failures are distinguished from runtime failures in structured state and console output
- compatibility fallback use is visible in logs

## Testing Strategy

The port needs tests at four levels.

### Manifest and Runtime Unit Tests

- schema validation
- request resolution
- stage dependency planning
- artifact path generation
- CLI argument translation from `ulk`-style inputs

### Fixture and Mapping Tests

- known kernel manifests map to the right patch sets
- arch manifests expose the expected toolchain and QEMU settings
- image manifests cover supported release defaults and mirror overrides
- profile manifests map to the right config fragments and build steps

### Stage Tests

- fetch and patch behavior against fixture manifests
- configure stage fragment composition
- tool build planning
- image and boot command assembly without needing a full system boot for every test

### Integration Smoke Tests

Representative parity flows should cover at least:

- `x86_64` modern kernel build
- `arm64` modern kernel build
- one patched 6.x kernel path
- guest image creation
- QEMU boot invocation
- dispatch of module, userspace, Rust, and BPF example builds

Acceptance rule:

A feature path is considered ported only when the orchestrator path passes its tests and produces the same user-visible outcome as the old workflow for the covered manifest set.

## Rollout Strategy

The migration should preserve function throughout the port.

- Phase 1 may keep some stage backends as thin wrappers or placeholders while the orchestrator scaffold is built.
- Later phases may delegate to retained helper logic temporarily if native stage implementations are not ready.
- Delegation is allowed only when logs make the fallback explicit and the outward workflow still works.

This keeps the lab usable during migration without locking the new system into the old script design.

## Risks and Mitigations

### Scope Risk

The full parity target is broad. The mitigation is explicit phase decomposition and a hard planning boundary at Phase 1.

### Tooling Drift Risk

Modern kernel, distro, and tool dependencies change over time. The mitigation is manifest-owned version data, checksums where applicable, and test coverage over resolved requests.

### Script-Coupling Risk

Large shell flows are hard to test and evolve. The mitigation is to keep policy in the orchestrator and restrict shell to leaf helpers.

### Example Sprawl Risk

`qulk/modules/` contains many heterogeneous learning projects. The mitigation is to reorganize them by category and onboard them in waves rather than forcing a single huge import step.

## Phase 1 Acceptance Criteria

Phase 1 is complete when:

- `linux-lab/` exists with the approved top-level layout
- a native orchestrator CLI exists
- `linux-lab/bin/ulk` translates the old argument form into the new request model
- manifest schemas exist for kernels, arches, images, and profiles
- the current `qulk` kernel patch versions and arch baselines are represented in manifests
- the stage graph and build-state recording exist
- tests cover manifest validation, request resolution, stage planning, and `ulk` argument compatibility
- no existing `qos` subsystem behavior is regressed by the introduction of `linux-lab/`

## Planning Boundary

The implementation plan that follows this spec must target **Phase 1: scaffold and orchestrator bootstrap** only. The later parity phases are intentionally deferred until the scaffold exists and its interfaces are stable.
