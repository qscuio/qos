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
│   │   ├── ulk                     # compatibility entrypoint
│   │   └── linux-lab               # native Phase 1 CLI entrypoint
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

### Phase 1 CLI Contract

Phase 1 exposes exactly two native commands:

- `validate`: parse inputs, normalize compatibility arguments, and resolve manifests statically without running stages
- `plan`: perform `validate`, then execute the Phase 1 stage graph and write stage state records under the resolved request directory

Phase 1 does not expose a native `run` command yet.

Exit rules:

- `validate` exits `0` only when static request resolution succeeds
- `plan` exits `0` only when static request resolution succeeds and the Phase 1 stage graph completes
- both commands exit non-zero on validation failure
- only `plan` may surface stage-level failures

`validate` is read-only in Phase 1. It does not create `request.json`, `validate.json`, or any stage-state files.

Compatibility-mode rule:

- `linux-lab/bin/ulk` maps to the native `plan` command in Phase 1
- a successful Phase 1 `ulk` invocation means "request accepted and placeholder stage plan emitted", not "kernel build or boot completed"

The native CLI entrypoint in Phase 1 is:

```bash
linux-lab/bin/linux-lab <validate|plan> \
  --kernel <version> \
  --arch <x86_64|arm64> \
  --image <noble|jammy> \
  --profile <name> [--profile <name> ...] \
  [--mirror <region>]
```

In native mode for Phase 1:

- `--kernel` is required
- `--arch` is required
- `--image` is required
- at least one `--profile` is required
- `--mirror` is optional
- native mode does not apply compatibility defaults for omitted flags

Phase 1 native examples:

```bash
linux-lab/bin/linux-lab validate \
  --kernel 6.18.4 \
  --arch x86_64 \
  --image noble \
  --profile default-lab

linux-lab/bin/linux-lab plan \
  --kernel 6.9.8 \
  --arch arm64 \
  --image jammy \
  --profile debug \
  --profile rust \
  --mirror sg
```

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

In Phase 1, `artifact_root` is exactly `build/linux-lab/requests/<request-fingerprint>/`.

The supported compatibility keys in Phase 1 are exactly:

- `arch`
- `kernel`
- `mirror`

No other `ulk` keys are accepted in Phase 1.

Compatibility parser rules in Phase 1:

- every compatibility argument must be exactly `key=value`
- malformed arguments are validation errors
- duplicate keys are validation errors

Compatibility normalization rules:

- `amd64` normalizes to `x86_64`
- `aarch64` normalizes to `arm64`
- omitted `arch` in compatibility mode resolves from host architecture if supported, otherwise fails with validation error
- omitted `kernel` in compatibility mode resolves from the kernel manifest marked `default_compat: true`
- omitted `image_release` in compatibility mode resolves from the image manifest marked `default_compat: true`
- omitted profiles in compatibility mode resolve to the manifest-defined `default-lab` profile set
- explicit `mirror_region` must be present in the selected image manifest's `mirror_regions`
- unknown compatibility keys are validation errors, not ignored input

Compatibility defaults are deterministic:

- exactly one kernel manifest must set `default_compat: true`
- exactly one image manifest must set `default_compat: true`
- zero or multiple defaults in either manifest family are validation errors

Profile composition rules:

- profiles are resolved by key, then sorted by a manifest-defined precedence value and finally by name for deterministic path generation
- the `profile-set` artifact path segment is the joined normalized profile keys in resolved order
- in Phase 1, profile planning is limited to `replaces`, `expands_to`, `tool_groups`, `example_groups`, and `post_build_refs`
- Phase 1 does not define scalar profile settings or dependency graphs beyond meta-profile expansion and replacement
- the resolved request written to state must include the fully expanded profile list and replacement decisions

### Compatibility Entry Point

`linux-lab/bin/ulk` remains available so users can keep familiar forms such as:

```bash
linux-lab/bin/ulk arch=x86_64 kernel=6.9.8 mirror=sg
```

The compatibility entrypoint must:

- accept the existing key-value argument shape used by `qulk`
- normalize arguments into a native request model
- call the native `plan` command in Phase 1
- log the resolved request so users can see the manifest-based execution path

It must not contain orchestration policy beyond parsing and translation.

## Manifest Model

The orchestrator uses four manifest families.

### Phase 1 Minimum Manifest Fields

Phase 1 schemas must explicitly support the fields needed for deterministic request resolution.

Kernel manifest minimum fields:

- `key`
- `supported_arches`
- `supported_profiles`
- `required_profiles`: optional list, default empty
- `default_compat`: boolean
- `source_url`: optional placeholder string
- `source_sha256`: optional placeholder string
- `patch_set`: optional placeholder string
- `config_family`: optional placeholder string
- `tool_groups`: optional list, default empty
- `example_groups`: optional list, default empty

Image manifest minimum fields:

- `key`
- `default_compat`: boolean
- `mirror_regions`: list of accepted mirror keys
- `recipe_ref`: optional placeholder string

Architecture manifest minimum fields:

- `key`
- `toolchain_prefix`
- `kernel_image_name`
- `qemu_machine`
- `qemu_cpu`
- `console`
- `supported_images`

Profile manifest minimum fields:

- `key`
- `kind`: `concrete` or `meta`
- `precedence`: integer
- `replaces`: list of profile keys, default empty
- `expands_to`: list of profile keys, allowed only for `kind: meta`
- `fragment_ref`: optional placeholder string
- `compatible_kernels`: optional list, default wildcard
- `compatible_arches`: optional list, default wildcard
- `host_tools`: optional list, default empty
- `tool_groups`: optional list, default empty
- `example_groups`: optional list, default empty
- `post_build_refs`: optional list, default empty

Schema evolution rule:

- Phase 1 fields above are the locked minimum contract for planning
- later phases may extend these schemas with additional optional fields
- later phases may not redefine the meaning of the Phase 1 fields above

Phase 1 profile resolution order is:

1. expand meta-profiles in listed order
2. deduplicate by first occurrence
3. apply `replaces`
4. sort the remaining concrete profiles by `precedence`, then by `key`
5. emit the sorted concrete profile list as the final `profile-set`

Unsupported combination validation in Phase 1 uses only manifest-owned fields:

- `kernel.supported_arches`
- `kernel.supported_profiles`
- `kernel.required_profiles`
- `arch.supported_images`
- `image.mirror_regions`
- `profile.compatible_kernels`
- `profile.compatible_arches`

`kernel.required_profiles` is strict in Phase 1. Missing required profiles are validation errors; they are not auto-added during request resolution.

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

- `6.18.4` with `default_compat: true`
- `4.19.317`
- `6.4.3`
- `6.9.6`
- `6.9.8`
- `6.10`

Phase 1 requires only schema-valid kernel manifest entries for these versions. Porting actual patch assets and full config parity remains Phase 2 work.

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

Phase 1 requires schema-valid arch manifests for both keys, including placeholder toolchain and QEMU planning data.

### Image Manifests

`linux-lab/manifests/images/<release>.yaml`

Each image manifest defines:

- distro release identity
- mirror rules
- debootstrap package set
- provisioning steps
- default guest networking assumptions
- shared-directory and login conventions where still needed

The explicit Phase 1 image manifest keys are:

- `noble` with `default_compat: true`
- `jammy`

Phase 1 requires only schema-valid image manifest entries for these keys. Real image-build recipes remain Phase 3 work.

### Profile Manifests

`linux-lab/manifests/profiles/<name>.yaml`

Each profile groups optional capabilities such as:

- `debug`
- `bpf`
- `rust`
- `samples`
- `debug-tools`
- `minimal`

Profiles own:

- required kernel config fragments
- host dependencies
- post-build steps
- example groups to build
- compatibility constraints by kernel or arch

The explicit Phase 1 profile keys are:

- `debug`
- `bpf`
- `rust`
- `samples`
- `debug-tools`
- `minimal`
- `default-lab`

`default-lab` is a meta-profile used by compatibility mode. In Phase 1 it expands, in this order, to:

1. `debug`
2. `bpf`
3. `rust`
4. `samples`
5. `debug-tools`

The concrete Phase 1 precedence values are:

- `debug`: `10`
- `bpf`: `20`
- `rust`: `30`
- `samples`: `40`
- `debug-tools`: `50`
- `minimal`: `100`

Phase 1 requires only schema-valid profile manifests for these keys. Real config fragments and example onboarding remain later-phase work.

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
- later phases may skip clean stages if inputs and manifest fingerprints have not changed
- fallback delegation to retained helper logic is allowed during migration, but the logs must state when a fallback path was used

Canonical request root:

`build/linux-lab/requests/<request-fingerprint>/`

`request-fingerprint` is a deterministic hash over:

- `kernel_version`
- `arch`
- `image_release`
- normalized `profile-set`
- `mirror_region`
- `compat_mode`
- orchestrator schema version

The minimum Phase 1 request directory layout is:

```text
build/linux-lab/requests/<request-fingerprint>/
├── request.json
├── state/
│   ├── validate.json
│   └── <stage>.json
└── logs/
```

Phase 1 does not need stage-result caching beyond this fingerprinted request root. Resumable reuse across mutable build artifacts starts in later phases.

If `plan` is re-run with the same request fingerprint in Phase 1, it overwrites `request.json`, `validate.json`, and `<stage>.json` in place. Phase 1 always re-emits the full stage-state set in dependency order and does not skip unchanged placeholder stages. Historical run retention is a later-phase concern.

`validate.json` is emitted only by the `plan` command and follows the same result schema as other stage-state files, with `stage` set to `validate`.

Phase 1 state examples:

```json
{
  "kernel_version": "6.18.4",
  "arch": "x86_64",
  "image_release": "noble",
  "profiles": ["debug", "bpf", "rust", "samples", "debug-tools"],
  "mirror_region": null,
  "compat_mode": false,
  "legacy_args": {},
  "artifact_root": "build/linux-lab/requests/2a4b..."
}
```

```json
{
  "stage": "validate",
  "status": "succeeded",
  "started_at": "2026-03-14T12:00:00Z",
  "ended_at": "2026-03-14T12:00:00Z",
  "request_fingerprint": "2a4b...",
  "inputs": ["resolved-request"],
  "outputs": ["resolved-request"],
  "log_path": "build/linux-lab/requests/2a4b.../logs/validate.log",
  "error_kind": null,
  "error_message": null
}
```

```json
{
  "stage": "build-image",
  "status": "placeholder",
  "started_at": "2026-03-14T12:00:01Z",
  "ended_at": "2026-03-14T12:00:01Z",
  "request_fingerprint": "2a4b...",
  "inputs": ["resolved-request", "kernel-plan", "tools-plan", "examples-plan"],
  "outputs": ["image-plan"],
  "log_path": "build/linux-lab/requests/2a4b.../logs/build-image.log",
  "error_kind": null,
  "error_message": null
}
```

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

#### Phase 1 Stage I/O

The canonical Phase 1 artifact descriptors are:

- `resolved-request` (`request`)
- `source-plan` (`plan`)
- `prepare-report` (`report`)
- `patch-plan` (`plan`)
- `config-plan` (`plan`)
- `kernel-plan` (`plan`)
- `tools-plan` (`plan`)
- `examples-plan` (`plan`)
- `image-plan` (`plan`)
- `boot-plan` (`plan`)

Phase 1 stage I/O table:

| Stage | Depends on | Consumes | Produces |
|------|------------|----------|----------|
| `fetch` | none | `resolved-request` | `source-plan` |
| `prepare` | `fetch` | `resolved-request`, `source-plan` | `prepare-report` |
| `patch` | `prepare` | `resolved-request`, `source-plan`, `prepare-report` | `patch-plan` |
| `configure` | `patch` | `resolved-request`, `patch-plan` | `config-plan` |
| `build-kernel` | `configure` | `resolved-request`, `config-plan` | `kernel-plan` |
| `build-tools` | `configure` | `resolved-request`, `config-plan` | `tools-plan` |
| `build-examples` | `build-kernel`, `build-tools` | `resolved-request`, `kernel-plan`, `tools-plan` | `examples-plan` |
| `build-image` | `build-kernel`, `build-tools`, `build-examples` | `resolved-request`, `kernel-plan`, `tools-plan`, `examples-plan` | `image-plan` |
| `boot` | `build-image` | `resolved-request`, `image-plan` | `boot-plan` |

## Component Boundaries

### `orchestrator/cli`

Native user-facing CLI. Responsible for:

- parsing native commands and raw arguments
- invoking `orchestrator/core` for request resolution
- selecting the top-level operation (`validate` or `plan`)
- rendering user-facing output and exit codes

### `orchestrator/core`

Shared execution runtime. Responsible for:

- manifest loading and validation
- request resolution
- stage planning
- state and cache bookkeeping
- structured logs
- failure normalization
- invoking stage implementations

### `orchestrator/stages`

One implementation module per stage. Each stage owns one concern only:

- `fetch`
- `prepare`
- `patch`
- `configure`
- `build-kernel`
- `build-tools`
- `build-examples`
- `build-image`
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

- `Makefile` targets `linux-lab-validate` and `linux-lab-plan`
- pytest entrypoints for manifest/runtime tests
- README note that `linux-lab` exists and is in bootstrap state
- ignore coverage for `build/linux-lab/`

## Error Handling

Error handling is stage-scoped and resumable.

- manifest validation fails early for unknown kernel versions, unsupported arch/profile combinations, and unsupported compatibility arguments
- manifest validation fails early for invalid mirror overrides and ambiguous compatibility defaults
- each stage records start, end, input fingerprint, output paths, and failure reason in `state/`
- successful stage outputs are not destroyed automatically on downstream failure
- in Phase 1, reruns are keyed only by request fingerprint and overwrite prior state for that same fingerprint
- leaf-tool failures must surface the underlying command, exit code, and relevant log path

Phase 1 failure classification:

- manifest syntax, unknown arguments, and unsupported kernel/arch/profile combinations are `validation` errors before stage execution
- invalid or unsupported `mirror_region` values are `validation` errors before stage execution
- zero or multiple `default_compat` manifests in a family are `validation` errors before stage execution
- `validate` is purely static and performs no host checks
- Phase 1 `prepare` checks only local prerequisites needed to emit state, such as writable build paths and a usable Python runtime
- Phase 1 does not probe network reachability, mirror health, cross-compilers, debootstrap, or QEMU availability
- later phases may add stage-specific prerequisite failures when those stages begin performing real work

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

- known kernel manifests map to the right placeholder patch references and supported profile sets
- arch manifests expose the expected toolchain and QEMU settings
- image manifests cover supported release defaults and mirror overrides
- profile manifests map to the right placeholder fragment references, tool groups, and example groups

#### Stage Tests

- fetch and patch placeholder metadata emission against fixture manifests
- configure stage placeholder planning
- tool, image, and boot placeholder artifact planning
- prepare-stage local prerequisite checks for writable state output

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
| `prepare` | implemented | check only local prerequisites required for state emission and record results |
| `patch` | placeholder | record referenced patch-manifest metadata without requiring patch files on disk |
| `configure` | placeholder | record referenced config and profile metadata without requiring real fragments |
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
- schema-valid stub manifests exist for the supported Phase 1 kernel, arch, image, and profile keys
- the stage graph and build-state recording exist
- the Phase 1 stage table above is implemented exactly as the minimum supported behavior
- root integration touchpoints are present for Makefile, pytest, README, and ignore coverage
- tests cover manifest validation, request resolution, stage planning, and `ulk` argument compatibility
- no existing `qos` subsystem behavior is regressed by the introduction of `linux-lab/`

## Planning Boundary

The implementation plan that follows this spec must target **Phase 1: scaffold and orchestrator bootstrap** only. The later parity phases are intentionally deferred until the scaffold exists and its interfaces are stable.
