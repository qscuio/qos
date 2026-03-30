# Linux Lab Userspace Bulk Port Design

**Date:** 2026-03-30
**Status:** User-approved design draft

## Overview

Port the full top-level userspace app corpus from `/home/ubuntu/work/qulk/userspace/app` into this repo's `linux-lab`, along with the dependent in-tree shared library layer from `/home/ubuntu/work/qulk/userspace/lib` and the vendored third-party dependency trees required to make the imported app set build under one bulk target.

The target outcome is not a curated subset. `linux-lab` should gain a self-contained userspace sample subtree that can be built in one shot from inside this repo, with compile failures fixed in-tree as part of the port.

## User-Confirmed Constraints

- Port all top-level apps from `/home/ubuntu/work/qulk/userspace/app`.
- Port the dependent libraries required by those apps.
- Build everything under one bulk target.
- Fix compile problems as part of the port instead of leaving a partially broken import.

## Current-State Observations

The source corpus is much larger than the current `linux-lab` userspace support:

- `/home/ubuntu/work/qulk/userspace/app` contains 398 top-level `*.c` app files.
- `/home/ubuntu/work/qulk/userspace/lib` contains 184 top-level `*.c`, `*.h`, and `*.S` files, plus nested vendored/project subtrees.
- The original app `Makefile` links against non-system targets such as `plthook`, `funchook`, `injector`, `pulp`, `userspace-rcu`, `unicorn`, `libelfmaster`, `libuv`, `libev`, and `libssh`.
- A meaningful subset of apps include headers from the shared `userspace/lib` layer such as `vlog.h`, `ovs-thread.h`, `poll-loop.h`, `util.h`, `hmap.h`, and related OVS-style support headers.

The current `linux-lab` integration is intentionally tiny:

- `linux-lab/examples/userspace/app` currently contains only `char_file.c`, `netlink_demo.c`, and `poll.c`.
- `linux-lab/catalog/examples/userspace-app.yaml` points at that three-file curated subset.
- `linux-lab/orchestrator/core/examples.py` currently treats userspace examples as `gcc-userspace`, which compiles each `*.c` file in a directory independently.

That existing model cannot represent a large userspace corpus with shared libraries, vendored dependencies, and per-app link requirements.

## Goals

- Import the full top-level app corpus from `qulk/userspace/app` into `linux-lab`.
- Import the dependent in-tree shared library layer from `qulk/userspace/lib`.
- Import the vendored third-party dependency trees needed to support the bulk userspace build in-repo.
- Add one bulk userspace build target that builds the imported support libraries first and then builds the imported apps.
- Integrate that bulk target with the `linux-lab` catalog and orchestrator.
- Fix compile failures required to get the imported userspace corpus building under the new target.

## Non-Goals

- Guarantee that every binary is runnable inside the guest as part of this phase.
- Rewrite the imported userspace source tree into a new house style.
- Normalize every third-party component into a shared repo-wide package manager workflow.
- Port every non-app subtree under `qulk/userspace/`; only the app corpus and the dependency closure needed to build it are in scope.

## Recommended Repository Layout

The import should preserve the recognizable source layout under a dedicated subtree:

```text
linux-lab/
├── catalog/
│   └── examples/
│       └── userspace-qulk-bulk.yaml
├── examples/
│   └── userspace/
│       ├── app/
│       │   └── ...
│       └── qulk/
│           ├── README.md
│           ├── Makefile
│           ├── app/
│           │   └── ...
│           ├── lib/
│           │   └── ...
│           ├── vendor/
│           │   ├── funchook/
│           │   ├── injector/
│           │   ├── libelfmaster/
│           │   ├── libev/
│           │   ├── libuv/
│           │   ├── plthook/
│           │   ├── pulp/
│           │   ├── unicorn/
│           │   └── userspace-rcu/
│           ├── build/
│           │   ├── include/
│           │   ├── lib/
│           │   ├── obj/
│           │   └── bin/
│           └── tools/
│               ├── build-manifest.py
│               └── scan-deps.py
└── orchestrator/
    └── ...
```

Notes:

- `linux-lab/examples/userspace/app/` may remain as compatibility wrappers or curated helpers, but the bulk port should live under `linux-lab/examples/userspace/qulk/`.
- The imported source should preserve the original `app/` and `lib/` split.
- Third-party source should live under a dedicated `vendor/` directory to make the dependency closure explicit.
- Build outputs should stay out of source subtrees.

## Import Strategy

Use a preserved-layout import with a small amount of adaptation at the boundary:

1. Copy the top-level app corpus into `linux-lab/examples/userspace/qulk/app/`.
2. Copy the shared `qulk/userspace/lib` layer into `linux-lab/examples/userspace/qulk/lib/`.
3. Copy only the vendored third-party trees that are actually needed to satisfy headers and link targets referenced by the imported apps or the shared lib layer.
4. Add a new top-level build system for the imported subtree instead of trying to reuse the original app directory's one-file-at-a-time build directly.

This approach preserves source relationships while still giving `linux-lab` a maintainable bulk build entrypoint.

## Build Model

The new userspace import should use one explicit bulk build entrypoint, exposed as a `custom-make` userspace example in the catalog.

The build should proceed in layers:

1. prepare the build root and generated include/config headers
2. build internal support libraries from `lib/`
3. build vendored third-party libraries needed by the app set
4. compile and link every top-level app from `app/`
5. emit a manifest summarizing built binaries, skipped binaries, and any remaining unsupported cases

The build entrypoint should support:

- deterministic output directories
- parallel `make -j`
- centralized compiler and linker flags
- generated include/library search paths
- a strict mode that fails on the first compile error
- a report mode that records which binary came from which source file

## Compatibility Layer

Compile-fix work should be organized through a shared compatibility/build layer instead of scattered command-line exceptions.

That layer should cover:

- normalized include search paths for `lib/` and vendored dependencies
- centralized `CPPFLAGS`, `CFLAGS`, `LDFLAGS`, and per-lib link groups
- generated config shims for projects that expect Autotools/CMake-produced headers
- compatibility macros for glibc/Linux feature toggles
- targeted patch points for source files that rely on stale prototypes, duplicate globals, or host-specific assumptions

The purpose is to keep per-app special cases visible and limited.

## Dependency Policy

The dependency closure should be in-repo. Building the bulk target must not depend on the developer having preinstalled copies of the required third-party libraries.

Policy:

- prefer vendoring source already present under `qulk/userspace/`
- build those dependencies inside the imported subtree
- avoid introducing new network-fetch steps
- use system libc and standard Linux toolchain components, but not system copies of the imported third-party libs

If a third-party subtree is too large or too build-system-specific to reuse directly, the port may extract the minimal headers/sources needed for the current app corpus as long as the origin and licensing remain documented.

## Catalog And Orchestrator Integration

The current `gcc-userspace` behavior in the example planner is too simple for this port. The bulk import should appear as a catalog entry with `build_mode: custom-make`.

Required changes:

- add a new catalog entry representing the bulk userspace import
- point its `source` to `linux-lab/examples/userspace/qulk`
- provide `build_commands` or a `custom-make` workdir so the orchestrator runs the bulk build entrypoint
- keep the current curated userspace helpers only if they do not conflict with the new bulk target

The orchestrator should consume the bulk build as one userspace example group entry, not as hundreds of independent `gcc` calls.

## Compile-Fix Policy

The port should fix compile failures required for the bulk target to succeed. The fixes should follow this order:

1. build-system fixes and include-path corrections
2. missing generated config/header shims
3. vendored dependency integration problems
4. source-level compile fixes for stale or non-portable code
5. targeted exclusions only if a file is not a Linux userspace app in practice or is impossible to build without out-of-scope toolchains

Any exclusion must be explicit and recorded in the generated manifest or README. The default expectation remains that all top-level app sources are built.

## Verification

The port is complete when:

- the imported subtree exists entirely inside this repo
- the catalog and orchestrator can invoke the bulk userspace build
- the bulk target builds the imported dependency closure and app set
- the verification run produces a manifest of built binaries
- automated tests cover the catalog entry, import presence, and build integration surface

## Risks

- The app corpus is heterogeneous and includes code that may assume older toolchains or non-Linux host environments.
- Some shared-lib headers may depend on generated config files that do not yet exist in `linux-lab`.
- Some third-party projects may not expose a trivial static-library build path and may need small in-tree adaptation.
- The current userspace example tests are curated-subset oriented and will need to change to avoid fighting the new bulk-build model.

## Decision Summary

The accepted design is:

- preserve the imported `qulk` userspace app/lib structure under `linux-lab/examples/userspace/qulk/`
- vendor the dependency closure into that subtree
- expose one bulk build target through the `linux-lab` catalog/orchestrator
- fix compile failures through a shared compatibility/build layer and targeted source patches

