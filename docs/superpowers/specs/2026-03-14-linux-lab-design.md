# Linux Lab Modern Kernel Lab Port Design

**Date:** 2026-03-14
**Status:** User-approved design, revised after spec review

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

### Resolved Request Model

Every execution resolves to one normalized request object. The minimum schema is:

- `kernel_version`: canonical manifest key
- `arch`: canonical arch key
- `image_release`: canonical image manifest key
- `profiles`: deterministic ordered list of profile keys
- `mirror_region`: optional mirror override such as `cn` or `sg`
- `compat_mode`: boolean set when entered through `bin/ulk`
- `legacy_args`: normalized map of accepted compatibility arguments
- `artifact_root`: resolved output directory under `build/linux-lab/`

Compatibility normalization rules:

- `amd64` normalizes to `x86_64`
- `aarch64` normalizes to `arm64`
- omitted `arch` in compatibility mode resolves from host architecture if supported, otherwise fails with validation error
- omitted `kernel` in compatibility mode resolves from the kernel manifest marked `default_compat: true`
- omitted `image_release` in compatibility mode resolves from the image manifest marked `default_compat: true`
- omitted profiles in compatibility mode resolve to the manifest-defined `default-lab` profile set
- unknown compatibility keys are validation errors, not ignored input

Profile composition rules:

- profiles are resolved by key, then sorted by a manifest-defined precedence value and finally by name for deterministic path generation
- the `profile-set` artifact path segment is the joined normalized profile keys in resolved order
- profile fragments may append settings, dependencies, and example groups
- conflicting scalar settings across profiles are validation errors unless one profile explicitly declares that it replaces the other
- the resolved request written to state must include the fully expanded profile list and replacement decisions

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

### Stage Contract

The interface between `orchestrator/core` and `orchestrator/stages` is explicit.

Each stage definition must declare:

- `name`
- `depends_on`: ordered upstream stage names
- `consumes`: named artifact descriptors required as input
- `produces`: named artifact descriptors it owns
- `supports_delegate`: whether temporary helper delegation is allowed
- `supports_placeholder`: whether non-executing plan-mode output is allowed in the current phase

Each artifact descriptor must record:

- `name`
- `kind`
- `path`
- `producer_stage`
- `required`: boolean
- `metadata`: free-form map for checksum, manifest fingerprint, or command summary

Each stage result must be recorded as JSON with:

- `stage`
- `status`: one of `succeeded`, `failed`, `skipped`, `delegated`, `placeholder`
- `started_at`
- `ended_at`
- `request_fingerprint`
- `inputs`
- `outputs`
- `log_path`
- `error_kind`: optional enum such as `validation`, `prerequisite`, `runtime`
- `error_message`: optional string

This contract is the minimum needed to make stages independently implementable and testable without guessing internal runtime behavior.

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

## Root Integration Touchpoints

The `linux-lab` subsystem must fit the existing `qos` repository conventions through explicit root-level touchpoints.

Required touchpoints:

- root `Makefile` gains Linux-lab-specific targets for the phases that are actually implemented
- root `tests/` gains Linux-lab pytest coverage for manifest/runtime behavior
- root documentation adds a short `linux-lab` section in [README.md](/home/ubuntu/work/qos/README.md)
- generated artifacts remain under `build/linux-lab/` and must be covered by the repo ignore rules
- no existing root targets for `c-os`, `rust-os`, `xv6`, or `linux1` may change behavior as a side effect of Phase 1

Phase 1 touchpoints are intentionally narrow:

- `Makefile` target(s) to validate or plan Linux-lab requests
- pytest entrypoints for manifest/runtime tests
- README note that `linux-lab` exists and is in bootstrap state
- ignore coverage for `build/linux-lab/`

## Error Handling

Error handling is stage-scoped and resumable.

- manifest validation fails early for unknown kernel versions, unsupported arch/profile combinations, missing patch files, or incomplete image recipes
- each stage records start, end, input fingerprint, output paths, and failure reason in `state/`
- successful stage outputs are not destroyed automatically on downstream failure
- reruns default to reusing valid upstream outputs and re-executing the failed stage and its downstream dependents
- leaf-tool failures must surface the underlying command, exit code, and relevant log path

Host prerequisite classification:

- manifest syntax, unknown arguments, and unsupported kernel/arch/profile combinations are `validation` errors before stage execution
- missing host tools, missing privileges, blocked network access, or unavailable mirrors are `prerequisite` failures owned by the `prepare` stage
- downstream failures from `make`, `git`, `debootstrap`, QEMU, or helper commands are `runtime` failures in the owning stage

The default behavior in all phases is to check prerequisites, not install or mutate the host automatically. Any future install-missing mode would require explicit follow-up design approval.

Exit semantics:

- non-zero exits from the CLI indicate request failure
- validation failures are distinguished from runtime failures in structured state and console output
- compatibility fallback use is visible in logs

## Testing Strategy

The port needs tests at four levels.

### Phase 1 Tests

#### Manifest and Runtime Unit Tests

- schema validation
- request resolution
- stage dependency planning
- artifact path generation
- CLI argument translation from `ulk`-style inputs

#### Fixture and Mapping Tests

- known kernel manifests map to the right patch sets
- arch manifests expose the expected toolchain and QEMU settings
- image manifests cover supported release defaults and mirror overrides
- profile manifests map to the right config fragments and build steps

#### Stage Tests

- fetch and patch behavior against fixture manifests
- configure stage fragment composition
- tool build planning
- image and boot command assembly without needing a full system boot for every test

Phase 1 does not require real kernel builds, debootstrap runs, or QEMU guest boots to count as complete.

### Later-Phase Integration Smoke Tests

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

### Phase 1 Stage Minimum Behavior

Phase 1 must execute a runnable request-resolution and stage-planning pipeline. It does not need to perform a full kernel build.

| Stage | Phase 1 status | Minimum required behavior |
|------|----------------|---------------------------|
| `fetch` | placeholder | declare planned source artifacts and record state |
| `prepare` | implemented | validate host prerequisites without mutating the host |
| `patch` | placeholder | resolve patch paths and record intended patch application |
| `configure` | placeholder | resolve config families and profile fragments into planned outputs |
| `build-kernel` | placeholder | record planned kernel artifact descriptors |
| `build-tools` | placeholder | record planned tool artifact descriptors |
| `build-examples` | placeholder | record planned example groups and artifact roots |
| `build-image` | placeholder | record planned image outputs and selected image manifest |
| `boot` | placeholder | record planned QEMU profile and boot log path |

Phase 1 may not claim build parity, image parity, or guest-boot parity. Its purpose is to make requests, manifests, stage contracts, and repo integration concrete enough for later phases.

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
- the Phase 1 stage table above is implemented exactly as the minimum supported behavior
- root integration touchpoints are present for Makefile, pytest, README, and ignore coverage
- tests cover manifest validation, request resolution, stage planning, and `ulk` argument compatibility
- no existing `qos` subsystem behavior is regressed by the introduction of `linux-lab/`

## Planning Boundary

The implementation plan that follows this spec must target **Phase 1: scaffold and orchestrator bootstrap** only. The later parity phases are intentionally deferred until the scaffold exists and its interfaces are stable.
