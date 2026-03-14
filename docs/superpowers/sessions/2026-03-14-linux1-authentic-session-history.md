# Linux 1.0.0 Session History

Date: 2026-03-14
Scope: Linux 1.0.0 integration, boot debugging, verification, feature-branch commit/push, and local merge back into `main`.

This document is a reconstructed session record based on the chat, git history, verification commands, and generated artifacts. It preserves the decisions, requests, outcomes, and proof points from the session. It is not a byte-for-byte dump of every shell line.

## Final Outcome

- Linux 1.0.0 is now present in the repository under `linux-1.0.0/`.
- The repo can build a historically-authentic Linux 1.0.0 kernel, a small custom userspace, a LILO-based disk image, and boot it in QEMU.
- The automated harness proves boot with these markers:
  - `LILO`
  - `Linux version 1.0.0`
  - `linux1-init: start`
  - `linux1-sh# `
- A manual interactive runner exists at `make linux1-curses`.
- The boot work was committed on branch `feature/linux1-authentic` as commit `849d282`.
- That branch was pushed to `origin`.
- The work was then merged locally into `/home/ubuntu/work/qos` `main` as a fast-forward.

## Repository Locations

- Main checkout: `/home/ubuntu/work/qos`
- Linux1 feature worktree: `/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic`
- Feature branch: `feature/linux1-authentic`
- Feature commit: `849d282` (`linux1: boot Linux 1.0.0 to init and shell`)

## Raw User Prompt Sequence

The visible user prompts in this session, in order:

1. `The current implementation is a test harness, not a bootable OS. The kernel subsystems compile as shared libraries exercised via Python/ctypes, and libc delegates to host glibc. There's no boot assembly, no hardware drivers, no standalone execution.

   This means:
   - It teaches OS API design and subsystem logic well
   - It does not teach boot sequences, hardware initialization, MMU setup, or real interrupt handling
   - Students won't see the "magic" of going from power-on to running code                                             Is this true ?`
2. `/new`
3. `please check the design and plan for xv6 and execute the plan for me.`
4. `So xv6 cannot bootup and be tested now ?`
5. `ok. commit all and push`
6. `I also want to build and run linux 1.0.0 version in this repo.`
7. `3`
8. `historical authenticity`
9. `1`
10. `1`
11. `I think a simple user space build by ourself is ok.`
12. `yes patch.`
13. `2`
14. `I wanto the keep the kernel code in local for learning and future developement.`
15. `yes`
16. `yes`
17. `yes`
18. `yes`
19. `approved`
20. `plesae continue`
21. `plesae continue`
22. `/status`
23. `please continue`
24. `please continue`
25. `please continue/please continue`
26. `you can install any tool you need to debug the problem.`
27. `no we never boot successfuly.`
28. `please continue`
29. `yes, please`
30. `[ubuntu@qscuio ~/work/qos] $ (main) ls
Makefile  README.md  c-os  docs  qemu  rust-os  scripts  tests  tools  xv6
[ubuntu@qscuio ~/work/qos] $ (main)  why I cannot find the code ?`
31. `yes. And please save all the content in this session into docs. I need review the full history.`

## Prompt And Action Timeline

1. User asked whether the repository was still a test harness rather than a bootable OS.
   Result: that statement was accepted as true for the original repository state.

2. User redirected the work toward xv6 design and execution.
   Result: xv6 planning was reviewed separately, but the work later shifted to Linux 1.0.0.

3. User asked whether xv6 could boot and be tested.
   Result: that question stayed separate from the Linux 1.0.0 work.

4. User asked for commits and push.
   Result: later fulfilled on the Linux1 feature branch.

5. User requested Linux 1.0.0 in this repository.
   Result: work started on a historically-authentic Linux 1.0.0 pipeline rather than another teaching stub.

6. User selected historical authenticity and accepted option-style prompts that favored:
   - keeping the original Linux 1.0.0 kernel source in-repo
   - using local patches rather than rewriting the codebase into a modernized imitation
   - building a minimal custom userspace in-repo

7. User explicitly approved patching and keeping kernel code local for learning and future development.
   Result: `linux-1.0.0/` was vendored into the repo and compatibility fixes were carried as source edits plus LILO patch files.

8. User repeatedly asked to continue and later approved installing debug tools.
   Result: the debugging loop continued through multiple boot failures until the system reached userspace.

9. User stated that the system had never booted successfully.
   Result: the debugging focus narrowed to genuine end-to-end boot proof rather than partial evidence.

10. After the successful Linux1 boot result, user asked for the work to be committed and then later asked why it was not visible in `main`.
    Result: the feature branch was explained as a separate worktree, then merged locally into `main`.

11. User asked to save the full session into docs.
    Result: this document was created.

## Major Technical Decisions

### Historical Build Strategy

- Chosen target: historical authenticity, not a Linux-inspired rewrite.
- Kernel source kept in-repo at `linux-1.0.0/`.
- Compatibility work carried with targeted edits and LILO patch files under `patches/lilo/`.
- Userspace kept intentionally simple and self-built in `linux1-userspace/`.

### Boot Pipeline

The final Linux1 build and boot path is:

1. Fetch and verify source provenance.
2. Build Linux 1.0.0 kernel.
3. Build tiny Linux1 userspace binaries.
4. Build a patched LILO.
5. Create a disk image and root filesystem.
6. Boot in QEMU and validate boot markers.

Key entrypoints:

- `make linux1`
- `make linux1-curses`
- `scripts/build_linux1_kernel.sh`
- `scripts/build_linux1_userspace.sh`
- `scripts/build_linux1_lilo.sh`
- `scripts/mk_linux1_disk.sh`
- `scripts/run_linux1_qemu.sh`
- `scripts/run_linux1_curses.sh`

## Debugging History

### Early State

At the start of Linux1 debugging, the repository did not have a successful Linux 1.0.0 boot path. The work had to establish:

- source acquisition and provenance checks
- a build pipeline compatible with modern host toolchains
- a disk image layout acceptable to old LILO assumptions
- a userspace layout acceptable to the Linux 1.0.0 ELF loader
- a trustworthy end-to-end boot proof

### Failure: LILO `LI` Stall

Observed symptom:

- VGA stalled at `LI` in some runs

Finding:

- LILO had been configured to use COM1 serial output, but some QEMU runs used `-serial none`.
- The stall was not a true kernel handoff failure in every case. It was partly an execution mismatch between LILO's configured serial expectations and the QEMU device setup.

Correction:

- QEMU runs kept a live serial backend attached with `-serial pty`.

### Failure: Kernel Panic Before Userspace

Observed symptom:

- Later runs reached the kernel and panicked with:
  - `Kernel panic: task[0] (swapper) doesn't support exec()`

Finding:

- This was a later-stage kernel and exec path problem, not a BIOS or LILO problem.

Key fix:

- The task-switch path in `include/linux/sched.h` was patched to avoid a bad far jump operand pattern that modern toolchains emitted for the old `switch_to()` macro.

Regression proof:

- `tests/test_linux1_kernel_layout.py` asserts the corrected `_schedule` layout and guards against the earlier bad jump form.

### Failure: Linux 1.0.0 ELF Loader Expectations

Observed symptom:

- Userspace binaries built on a modern toolchain did not match Linux 1.0.0 loader assumptions.

Finding:

- The Linux 1.0.0 ELF loader expects a simple single-load-segment layout and is sensitive to segment-tail zeroing assumptions.

Fix:

- A dedicated linker script was introduced in `scripts/linux1_userspace.ld`.
- Userspace is now linked into a Linux 1.0-friendly single `PT_LOAD` layout.

Regression proof:

- `tests/test_linux1_userspace_layout.py`

### Failure: Disk Geometry and Partition Metadata

Observed symptom:

- Modern partition tooling did not emit the CHS geometry that the LILO path assumed.

Fix:

- `scripts/mk_linux1_disk.sh` rewrites the MBR partition CHS fields to match the LILO geometry contract.

Regression proof:

- `tests/test_linux1_disk_geometry.py`

### Failure: No Visible Userspace Console

Observed symptom:

- The kernel progressed farther than first assumed, but visible shell proof was missing.

Findings:

- The custom init was using VGA `tty1`, not the serial console.
- The root filesystem did not initially provide `/dev/tty1`.

Fixes:

- Added `/dev/tty1` during image creation in `scripts/mk_linux1_disk.sh`.
- Updated `linux1-userspace/src/init.S` to use `tty1`.

Result:

- The userspace banner and shell prompt became visible on the VGA console.

### Harness Problem: Serial Logs Alone Were Not Enough

Observed symptom:

- A serial-only harness could not prove the visible shell prompt because userspace was on VGA `tty1`.

Fix:

- Installed `tesseract-ocr`.
- Reworked `scripts/run_linux1_qemu.sh` to:
  - keep COM1 alive for LILO
  - drive QEMU headlessly
  - use the QEMU monitor `screendump`
  - OCR the screen image
  - synthesize canonical boot markers into `build/linux1/logs/boot.log`

Result:

- The harness now proves the visible prompt rather than inferring it.

## Files Added Or Changed

The key Linux1 files created or changed in this session include:

- `Makefile`
- `README.md`
- `docs/superpowers/plans/2026-03-13-linux1-authentic-build.md`
- `docs/superpowers/specs/2026-03-13-linux1-authentic-build-design.md`
- `linux-1.0.0/include/linux/sched.h`
- `linux1-userspace/src/common.inc`
- `linux1-userspace/src/syscalls.S`
- `linux1-userspace/src/init.S`
- `linux1-userspace/src/sh.S`
- `linux1-userspace/src/echo.S`
- `scripts/build_linux1_kernel.sh`
- `scripts/build_linux1_lilo.sh`
- `scripts/build_linux1_userspace.sh`
- `scripts/linux1_userspace.ld`
- `scripts/linux1_zsystem.ld`
- `scripts/mk_linux1_disk.sh`
- `scripts/run_linux1_qemu.sh`
- `scripts/run_linux1_curses.sh`
- `patches/lilo/0001-use-linux-page-header.patch`
- `patches/lilo/0002-linux-config-fallback.patch`
- `patches/lilo/0003-genhd-compat.patch`
- `patches/lilo/0004-partition-type-compat.patch`
- `patches/lilo/0005-partition-c-compat.patch`
- `patches/lilo/0006-lp64-ondisk-layout.patch`
- `tests/test_linux1.py`
- `tests/test_linux1_kernel_layout.py`
- `tests/test_linux1_disk_geometry.py`
- `tests/test_linux1_userspace_layout.py`
- `tests/test_linux1_make_targets.py`

## Verification History

### Feature-Branch Verification

Verified in the Linux1 worktree before commit:

```bash
pytest -q tests/test_linux1.py \
  tests/test_linux1_userspace_layout.py \
  tests/test_linux1_kernel_layout.py \
  tests/test_linux1_disk_geometry.py
```

Result:

- `7 passed in 137.61s`

After adding the interactive target:

```bash
pytest -q tests/test_linux1_make_targets.py \
  tests/test_linux1.py \
  tests/test_linux1_userspace_layout.py \
  tests/test_linux1_kernel_layout.py \
  tests/test_linux1_disk_geometry.py
```

Result:

- `9 passed in 137.47s`

Manual smoke proof:

```bash
timeout 35s bash scripts/run_linux1_curses.sh
```

Observed in the captured typescript:

- `linux1-init: start`

Boot artifact proof:

- `build/linux1/logs/boot.log`
- `build/linux1/logs/boot-screen.png`

Expected markers present:

- `LILO`
- `Linux version 1.0.0`
- `linux1-init: start`
- `linux1-sh# `

### Main-Branch Post-Merge Verification

The same focused Linux1 verification suite was re-run from `/home/ubuntu/work/qos` after merging `feature/linux1-authentic` into `main`.

Command:

```bash
pytest -q tests/test_linux1_make_targets.py \
  tests/test_linux1.py \
  tests/test_linux1_userspace_layout.py \
  tests/test_linux1_kernel_layout.py \
  tests/test_linux1_disk_geometry.py
```

Result:

- `9 passed in 161.34s (0:02:41)`

## Git History Created In This Session

Important visible branch history:

```text
849d282 linux1: boot Linux 1.0.0 to init and shell
3f7adf3 chore: vendor linux 1.0.0 kernel source with minimal compatibility patches
008aa15 build: add linux1 fetch and provenance verification scripts
0532a71 test: add failing linux1 build and boot smoke tests
67c8cf2 chore: add linux1 source manifest and userspace scaffolding
aae13fe docs: add linux1 authentic build implementation plan
```

Feature-branch push status:

- Branch pushed to `origin/feature/linux1-authentic`

Main merge status:

- `/home/ubuntu/work/qos` `main` was fast-forwarded from `e35b972` to `849d282`

## Why The Code Was Not Visible In `main`

The Linux1 work was done in a separate Git worktree:

- `/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic`

The user later inspected:

- `/home/ubuntu/work/qos`

That checkout was still on `main`, so it did not yet show the feature branch contents. After the local fast-forward merge, the Linux1 code became visible in the main checkout.

## Current State

- Linux 1.0.0 source and build pipeline are in the repository.
- Local `main` contains the Linux1 work.
- The feature branch still exists on the remote as `origin/feature/linux1-authentic`.
- The separate Linux1 worktree also still exists locally.
- The Linux1 worktree contains many untracked build byproducts under `linux-1.0.0/`. Those files were intentionally left out of commits.

## Remaining Follow-Up Options

- Push `main` to `origin` if the repository owner wants the merge published.
- Remove the Linux1 worktree after confirming it is no longer needed.
- Add more shell commands or a fuller userspace if the learning target expands beyond the current minimal environment.
