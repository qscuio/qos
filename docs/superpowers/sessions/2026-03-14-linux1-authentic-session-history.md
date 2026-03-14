# Linux 1.0.0 Session History

Date: 2026-03-14
Primary checkout: `/home/ubuntu/work/qos`
Primary feature worktree: `/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic`
Primary feature branch: `feature/linux1-authentic`
Primary boot commit: `849d282` (`linux1: boot Linux 1.0.0 to init and shell`)

## Purpose

The earlier session note was too compressed to be useful for review. This version preserves a much denser record of what happened during the Linux 1.0.0 work:

- exact visible user prompts from the session transcript
- exact visible assistant final responses that are still recoverable from the transcript
- a reconstructed progress log for the intermediate debugging work
- the debugging method used throughout the boot investigation
- the hypothesis log, including failed ideas and corrected assumptions
- the command and evidence log used to prove the final state
- the branch, worktree, merge, and verification history

## Cached Transcript Export

The full sanitized cache-derived event stream for this thread is now saved in:

- [2026-03-14-linux1-authentic-cache-sanitized.jsonl](/home/ubuntu/work/qos/docs/superpowers/sessions/2026-03-14-linux1-authentic-cache-sanitized.jsonl)
- [2026-03-14-linux1-authentic-cache-manifest.md](/home/ubuntu/work/qos/docs/superpowers/sessions/2026-03-14-linux1-authentic-cache-manifest.md)

Those files were exported from the local Codex rollout cache at:

- `~/.codex/sessions/2026/03/13/rollout-2026-03-13T07-48-46-019ce62b-57f2-77d1-ae4d-b7ddb927c336.jsonl`

What they contain:

- every cached `message:user`
- every cached `message:assistant`
- every cached `function_call` and `function_call_output`
- every cached `custom_tool_call` and `custom_tool_call_output`

What they intentionally omit:

- developer/system prompt material
- encrypted reasoning records
- session metadata containing base instructions
- internal event-plumbing records that are not part of the visible or operational session

## Limits Of This Record

This document is intentionally fuller than the first version, but it still has one hard limit: it cannot contain raw private chain-of-thought or hidden internal analysis that is not exposed in the session transcript. Where the transcript did not preserve every intermediate assistant progress update verbatim, this document marks the record as reconstructed from:

- the visible chat transcript
- the commit graph
- the files changed
- verification output
- generated boot artifacts
- the preserved session summary from the Linux1 worktree

So this file contains:

- exact transcript where available
- explicit reconstruction where exact text was not recoverable

It does not pretend that reconstructed notes are exact verbatim logs.

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
- That branch was pushed to `origin/feature/linux1-authentic`.
- The work was later fast-forwarded into local `main` in `/home/ubuntu/work/qos`.
- This fuller history note was added afterwards on `main` as commit `22a4672`.

## Repository Locations

- Main checkout: `/home/ubuntu/work/qos`
- Linux1 feature worktree: `/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic`
- Feature branch: `feature/linux1-authentic`
- Feature commit: `849d282`
- Follow-up docs commit on `main`: `22a4672`

## Visible Session Transcript

### Visible User Prompts

The visible user prompts in this session, in order, are copied below.

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
32. `this is too simple. it's useless. I need all your response and analysis and debug method. And we should update gitignore for all the tmp files.`
33. `you can just copy the session history to our docs.`

### Visible Assistant Final Responses

The following assistant final responses are recoverable from the session transcript and are copied here verbatim.

#### Response: boot still not complete

```text
Correct: we still do not boot successfully end to end.

Current verified status:
- BIOS -> MBR -> LILO -> kernel does work in this repo when QEMU provides the serial device LILO is configured to use.
- The system still does not reach userspace shell. It panics later in the kernel with `Kernel panic: task[0] (swapper) doesn't support exec()`.

What I changed:
- Added a disk-geometry regression test: [tests/test_linux1_disk_geometry.py](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/tests/test_linux1_disk_geometry.py)
- Kept the kernel-entrypoint regression test: [tests/test_linux1_kernel_layout.py](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/tests/test_linux1_kernel_layout.py)
- Fixed MBR partition CHS rewriting in [scripts/mk_linux1_disk.sh](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/scripts/mk_linux1_disk.sh) so the DOS partition entry matches the LILO geometry contract instead of modern `sfdisk`’s default translation.

Important correction to earlier debugging:
- The `LI` VGA stalls were from running a serial-configured LILO build under QEMU with `-serial none`.
- With a real serial backend attached, the same disk image gets past LILO and reaches the later kernel panic.
- Evidence:
  - VGA capture: `/tmp/linux1-serialdev-12.png`
  - Serial log: `/tmp/linux1-serialdev.log`

Verification run:
```bash
pytest -q tests/test_linux1_kernel_layout.py tests/test_linux1_disk_geometry.py
```
Result: `2 passed`

So the honest state is:
- Disk boot chain is alive.
- Successful Linux 1.0.0 boot is still not achieved.
- The remaining blocker is the later kernel/task-switch/exec failure, not LILO handoff.

Next steps:
1. Continue on the kernel panic path around [kernel/fork.c](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/linux-1.0.0/kernel/fork.c) and [kernel/sched.c](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/linux-1.0.0/kernel/sched.c).
2. If you want, I can patch the Linux 1.0.0 harness/docs so the repo reports the current truthful status instead of implying shell boot works now.
```

#### Response: boot reaches init and shell

```text
Linux 1.0.0 now boots in this repo and reaches the custom init plus shell prompt.

What changed:
- Fixed the task-switch miscompile in [sched.h](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/linux-1.0.0/include/linux/sched.h).
- Switched userspace ELF linking to a Linux 1.0-friendly single `PT_LOAD` layout with [linux1_userspace.ld](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/scripts/linux1_userspace.ld) and [build_linux1_userspace.sh](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/scripts/build_linux1_userspace.sh).
- Kept custom init on `tty1` in [init.S](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/linux1-userspace/src/init.S).
- Added `/dev/tty1` to the image builder in [mk_linux1_disk.sh](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/scripts/mk_linux1_disk.sh).
- Reworked the boot runner in [run_linux1_qemu.sh](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/scripts/run_linux1_qemu.sh) to validate the VGA text console via screenshot + OCR instead of relying on serial output.
- Added regressions in [test_linux1_kernel_layout.py](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/tests/test_linux1_kernel_layout.py), [test_linux1_disk_geometry.py](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/tests/test_linux1_disk_geometry.py), and [test_linux1_userspace_layout.py](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/tests/test_linux1_userspace_layout.py).

Verified:
- [boot.log](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/build/linux1/logs/boot.log) now contains:
  - `LILO`
  - `Linux version 1.0.0`
  - `linux1-init: start`
  - `linux1-sh# `
- [boot-screen.png](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic/build/linux1/logs/boot-screen.png) shows the prompt on `tty1`.
- `pytest -q tests/test_linux1.py tests/test_linux1_userspace_layout.py tests/test_linux1_kernel_layout.py tests/test_linux1_disk_geometry.py`
  - `7 passed in 137.61s`

So the answer to the earlier question is now:
- `xv6`: separate topic
- `linux1`: yes, this repo now boots Linux 1.0.0 and the harness can test it

If you want, next I can:
1. commit the Linux 1.0.0 boot fixes
2. add a dedicated interactive `make linux1-curses` target for manual shell use
```

#### Response: why code was not visible in `main`

```text
Because the work was done in a separate Git worktree, not in your current `main` checkout.

Current state:
- Your shell is in `/home/ubuntu/work/qos`
- That checkout is on branch `main`
- The Linux 1.0.0 changes are on branch `feature/linux1-authentic`
- That branch lives in a different worktree: [linux1-authentic](/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic)

Verified:
- `main` at `/home/ubuntu/work/qos` -> `e35b972`
- `feature/linux1-authentic` at `/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic` -> `849d282`

So `ls` in `/home/ubuntu/work/qos` shows the old `main` tree, not the Linux1 branch contents.

To see the code now:
```bash
cd /home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic
git branch --show-current
ls
```

To bring it into your current checkout, you have 3 reasonable options:
1. Switch your current repo to that branch
```bash
cd /home/ubuntu/work/qos
git fetch origin
git switch feature/linux1-authentic
```

2. Merge that branch into `main`
```bash
cd /home/ubuntu/work/qos
git fetch origin
git switch main
git merge origin/feature/linux1-authentic
```

3. Cherry-pick just the commit
```bash
cd /home/ubuntu/work/qos
git fetch origin
git switch main
git cherry-pick 849d282
```

If you want, I can merge `feature/linux1-authentic` into your `/home/ubuntu/work/qos` `main` checkout now.
```

#### Response: local main now contains Linux1 work

```text
`main` now contains the Linux 1.0.0 work in `/home/ubuntu/work/qos`.

What I did:
- Fast-forward merged `origin/feature/linux1-authentic` into local `main`
- Added the session history doc at [2026-03-14-linux1-authentic-session-history.md](/home/ubuntu/work/qos/docs/superpowers/sessions/2026-03-14-linux1-authentic-session-history.md)
- Committed the doc on `main` as `22a4672` (`docs: add linux1 session history`)

Current local history:
- `22a4672` `docs: add linux1 session history`
- `849d282` `linux1: boot Linux 1.0.0 to init and shell`

Verification on `main`:
- `pytest -q tests/test_linux1_make_targets.py tests/test_linux1.py tests/test_linux1_userspace_layout.py tests/test_linux1_kernel_layout.py tests/test_linux1_disk_geometry.py`
- Result: `9 passed in 161.34s (0:02:41)`

Boot proof on `main`:
- [boot.log](/home/ubuntu/work/qos/build/linux1/logs/boot.log)
- Contains:
  - `LILO`
  - `Linux version 1.0.0`
  - `linux1-init: start`
  - `linux1-sh# `

One detail:
- `main` is not pushed yet.
- The repo still has many untracked `linux-1.0.0/` build artifacts (`.o`, `.depend`, `zImage`, etc.). I did not commit them.

If you want, next I can:
1. push `main` to `origin`
2. clean or ignore the generated Linux 1.0.0 build outputs
```

## Reconstructed Progress Log

This section reconstructs the intermediate progress work that was not fully preserved as exact final responses.

1. The work began from the user's request to add Linux 1.0.0 to the repository and to prefer historical authenticity.
2. The direction settled on keeping real Linux 1.0.0 source in-repo, patching compatibility issues rather than rewriting the kernel into a modern educational facsimile, and building a minimal custom userspace locally.
3. A new isolated worktree was used for Linux1 development so the feature could progress without destabilizing the main checkout.
4. The early build phase established source provenance, source fetching, kernel build scripts, LILO build scripts, userspace build scripts, disk-image creation, and an initial QEMU boot path.
5. Early boot attempts did not reach a trustworthy userspace proof. The debug loop narrowed the problem into separate stages rather than treating all failures as one issue.
6. The first major correction was that `LI` stalls were not always evidence of a dead boot chain. They were often caused by running a serial-configured LILO under QEMU without a live serial device.
7. After that fix, the system reliably reached the Linux kernel, but later failed with `task[0] (swapper) doesn't support exec()`.
8. That later failure moved the investigation away from bootsector and disk layout problems and toward kernel scheduling and exec behavior.
9. The task-switch code in `include/linux/sched.h` was patched to avoid a miscompiled far pointer form generated by a modern toolchain.
10. Kernel-entrypoint and disk-geometry regressions were written so the same issues would not silently return.
11. The next major blocker came from Linux 1.0.0 ELF loader expectations. Modern default linking produced userspace binaries that were structurally valid ELF, but unfriendly to Linux 1.0.0 loader assumptions.
12. A dedicated userspace linker script was introduced so `init`, `sh`, and `echo` would use a single `PT_LOAD` segment and end on expected page boundaries.
13. Even after the kernel and userspace layout issues were corrected, proof of userspace execution was still incomplete because the visible console path was wrong.
14. The key discovery was that userspace was on VGA `tty1`, not the serial console, and the root filesystem image initially lacked `/dev/tty1`.
15. After creating `/dev/tty1` in the image and aligning the custom init with `tty1`, the boot path finally showed `linux1-init: start` and the `linux1-sh# ` prompt on the VGA text console.
16. A serial-only harness still could not prove this state, so the boot runner was reworked to capture a VGA screenshot and OCR the screen.
17. `tesseract-ocr` was installed for that purpose, and the headless boot runner was changed to combine serial evidence with OCR-derived boot markers.
18. Once the runner proved `LILO`, `Linux version 1.0.0`, `linux1-init: start`, and `linux1-sh# ` in order, the repo had a trustworthy automated Linux1 boot proof.
19. The user then requested commit-and-push plus a manual runner. A `linux1-curses` target and `run_linux1_curses.sh` script were added.
20. The feature branch was verified, committed, and pushed.
21. When the user later looked in `/home/ubuntu/work/qos` and could not find the code, the reason was that the Linux1 work lived in a separate worktree. The branch was then merged into local `main`.
22. A short session summary was initially written to docs, but the user correctly rejected it as too thin. This fuller document replaces that level of detail with a denser review record.

## Debugging Method

The Linux1 debugging method followed a fairly strict pattern. This section states that method explicitly so the work is reviewable as engineering rather than as trial and error.

### 1. Split the boot path into stages

The work treated boot as a chain of boundaries:

- BIOS and MBR handoff
- LILO execution
- kernel decompression and entry
- scheduler and exec viability
- userspace ELF loader compatibility
- root filesystem and device nodes
- console routing
- proof collection in the harness

This mattered because a symptom at one stage could easily be misread as a failure at another stage.

### 2. Verify each claim with an artifact, not with intuition

Examples:

- `LI` on screen was not enough to declare LILO dead; serial-device configuration had to be verified.
- A kernel panic meant the disk boot chain was already alive, which narrowed the search.
- A missing serial prompt did not mean userspace failed; VGA `tty1` had to be checked.
- The final success claim was not made until the harness proved the boot markers from artifacts.

### 3. Turn each discovered failure into a regression

The debugging work did not stop at a one-off fix. Each key class of failure was converted into a test or harness check:

- kernel entrypoint and `_schedule` machine-code expectations
- MBR geometry and partition metadata
- userspace ELF segment layout
- presence of the manual `linux1-curses` entrypoint
- ordered boot markers in the end-to-end boot log

### 4. Challenge assumptions about observability

A major error early in the process was assuming serial output alone represented the whole guest state. That assumption failed because:

- LILO needed a serial backend attached
- userspace later ran visibly on VGA `tty1`
- the shell prompt existed on screen before it existed in any reliable serial capture

The fix was not to keep reading the same logs harder. The fix was to change the observability model.

### 5. Prefer small, mechanical compatibility fixes over broad rewrites

Examples:

- patching `switch_to()` rather than redesigning Linux 1.0.0 scheduling
- adding a linker script for userspace rather than replacing the loader path
- rewriting partition CHS metadata rather than replacing the disk format
- carrying LILO compatibility patches as patch files rather than rewriting LILO into a modern loader

### 6. Keep boot evidence reproducible

The work always returned to a small set of repeatable proof commands:

- focused `pytest` runs
- `make linux1`
- `make linux1-curses`
- boot log inspection
- screenshot and OCR capture

The point was to make progress reviewable by rerun, not by memory.

## Hypothesis Log

| Hypothesis | How it was tested | Result | Consequence |
|---|---|---|---|
| LILO handoff is fundamentally broken | VGA `LI` symptom plus later reruns with proper serial backend | False as a root cause | Boot chain was alive when COM1 existed |
| The disk image geometry is acceptable to old LILO assumptions | Partition entry inspection and geometry regression | Initially false | Added CHS rewrite in `scripts/mk_linux1_disk.sh` |
| The kernel never reaches meaningful execution | Later panic text and marker progression | False | Investigation moved to scheduler and exec path |
| Scheduler/exec failure comes from task-switch machine code emitted by the modern toolchain | Inspect `_schedule` layout, patch `switch_to()`, add regression | True | Patched `include/linux/sched.h` |
| Userspace binaries are fine because they are valid ELF files | Read ELF program headers, compare with Linux 1.0 expectations | False | Added `scripts/linux1_userspace.ld` and layout regression |
| Lack of serial shell output means userspace did not start | Compare serial behavior with VGA console evidence | False | Console routing and device-node investigation started |
| The custom init should target serial for proof | Tried serial-centric proof paths, compared with visible VGA behavior | Wrong for final proof | Switched to `tty1` and provided `/dev/tty1` |
| The repo harness can prove Linux1 boot from serial output alone | Serial logs compared against visible screen | False | Harness reworked to use screen capture plus OCR |
| The new manual target exists and is enough without verification | Added TDD for target existence and runner script presence | Inadequate without test | Added `tests/test_linux1_make_targets.py` |
| The feature branch code is visible in local `main` | Inspected `/home/ubuntu/work/qos` branch and worktree list | False before merge | Merged `feature/linux1-authentic` into local `main` |

## Command And Evidence Log

The list below records the main command paths and the evidence they produced. This is not every shell line from the session. It is the curated command log that mattered to the investigation.

### Branch and worktree inspection

Commands:

```bash
git worktree list
git branch -vv
git rev-parse --abbrev-ref HEAD
```

Evidence:

- `main` checkout at `/home/ubuntu/work/qos`
- Linux1 feature worktree at `/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic`
- feature branch `feature/linux1-authentic`

### Focused regression verification during the partial-boot phase

Command:

```bash
pytest -q tests/test_linux1_kernel_layout.py tests/test_linux1_disk_geometry.py
```

Evidence:

- `2 passed`
- proved the kernel layout and disk geometry fixes in the partial-boot phase

### Full Linux1 boot verification after reaching shell prompt

Command:

```bash
pytest -q tests/test_linux1.py tests/test_linux1_userspace_layout.py \
  tests/test_linux1_kernel_layout.py tests/test_linux1_disk_geometry.py
```

Evidence:

- `7 passed in 137.61s`
- proved boot markers, userspace layout, kernel layout, and disk geometry

### TDD for manual curses target

Red phase:

```bash
pytest -q tests/test_linux1_make_targets.py
```

Evidence:

- failed because `linux1-curses` target and `scripts/run_linux1_curses.sh` did not yet exist

Green phase after implementation:

```bash
pytest -q tests/test_linux1_make_targets.py
```

Evidence:

- `2 passed in 0.02s`

### Full feature-branch verification before commit

Command:

```bash
pytest -q tests/test_linux1_make_targets.py tests/test_linux1.py \
  tests/test_linux1_userspace_layout.py tests/test_linux1_kernel_layout.py \
  tests/test_linux1_disk_geometry.py
```

Evidence:

- `9 passed in 137.47s (0:02:17)`

### Manual curses smoke proof

Command:

```bash
script -qfec 'timeout 35s bash scripts/run_linux1_curses.sh' /tmp/linux1-curses-runner.log
```

Evidence:

- typescript contained `linux1-init: start`
- process ended with timeout as expected for an interactive guest runner

### Boot artifact proof

Key artifact:

- `build/linux1/logs/boot.log`

Recovered markers:

- `LILO`
- `Linux version 1.0.0`
- `linux1-init: start`
- `linux1-sh# `

Supporting image artifact:

- `build/linux1/logs/boot-screen.png`

### Commit and push

Commands:

```bash
git commit -m "linux1: boot Linux 1.0.0 to init and shell"
git push -u origin feature/linux1-authentic
```

Evidence:

- feature commit `849d282`
- remote branch `origin/feature/linux1-authentic`

### Local main merge and post-merge verification

Commands:

```bash
git -C /home/ubuntu/work/qos merge --ff-only origin/feature/linux1-authentic
pytest -q tests/test_linux1_make_targets.py tests/test_linux1.py \
  tests/test_linux1_userspace_layout.py tests/test_linux1_kernel_layout.py \
  tests/test_linux1_disk_geometry.py
```

Evidence:

- local `main` fast-forwarded to `849d282`
- post-merge verification result: `9 passed in 161.34s (0:02:41)`

### Proof copied from the main checkout after merge

Artifact:

- `build/linux1/logs/boot.log`

Observed contents:

```text
LILO boot:
Loading linux.....
LILO
Linux version 1.0.0
linux1-init: start
linux1-sh#
```

## Detailed Debugging Narrative

### Design choice and scope

The user did not ask for a Linux-like teaching harness. The user asked to build and run Linux 1.0.0 in this repository, and later selected historical authenticity. That drove three key choices:

- keep original Linux 1.0.0 source local in the repo
- accept compatibility patching where modern hosts differed from the old environment
- keep userspace simple and local so the kernel remained the main historical study object

### Why the work needed a feature worktree

The repository already had a separate ongoing OS workspace. Linux1 integration was large enough that it needed isolation. That avoided mixing Linux1 changes with the rest of the project while the build and boot path were still unstable.

### Why the early `LI` interpretation was misleading

Seeing `LI` on the VGA path strongly suggests LILO trouble, but in this case the environment itself was wrong for a serial-configured LILO. Once QEMU preserved the serial device that LILO expected, the boot chain progressed farther. The practical lesson was simple: do not debug a bootloader in an environment that violates the bootloader's configured device assumptions.

### Why the kernel panic was progress, not failure regression

The `swapper` panic looked worse than an `LI` stall at first glance, but it actually proved more of the path worked:

- BIOS worked
- the MBR worked
- LILO worked
- the kernel image loaded
- early kernel execution happened

That allowed the investigation to stop blaming the bootloader and start inspecting schedule and exec behavior.

### Why userspace layout mattered

A modern linker will happily generate ELF files that are correct for modern Linux and still unsuitable for Linux 1.0.0 loader assumptions. The fix was not to make the binaries less ELF-like. The fix was to make them more specifically compatible with what Linux 1.0.0 expects.

### Why the visible shell was missing from the early proof

The shell existed on VGA `tty1` before the harness could prove it. That gap between reality and observability was central to the whole session. Once `/dev/tty1` existed in the image and init targeted `tty1`, the shell became visible in the right place. Then the harness had to be upgraded to look there.

### Why OCR was necessary

A pure serial harness would have kept missing the decisive evidence. QEMU screenshot plus OCR was a pragmatic answer. It is not historically elegant, but it is operationally correct for this repo's test harness. It provided the automation needed to convert a human-visible shell prompt into a machine-checkable boot proof.

## Files Added Or Changed

The main Linux1 files added or changed during the session are:

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

## Git History Created During The Work

Visible branch history around the Linux1 effort:

```text
aae13fe docs: add linux1 authentic build implementation plan
67c8cf2 chore: add linux1 source manifest and userspace scaffolding
0532a71 test: add failing linux1 build and boot smoke tests
008aa15 build: add linux1 fetch and provenance verification scripts
3f7adf3 chore: vendor linux 1.0.0 kernel source with minimal compatibility patches
849d282 linux1: boot Linux 1.0.0 to init and shell
22a4672 docs: add linux1 session history
```

## Why The Code Was Not Visible In `main`

The Linux1 feature was developed in a separate Git worktree:

- `/home/ubuntu/.config/superpowers/worktrees/qos/linux1-authentic`

The user later inspected:

- `/home/ubuntu/work/qos`

That second path was still on `main` at the time, so it did not yet show the Linux1 feature branch contents. After the local fast-forward merge, Linux1 source and scripts became visible in the main checkout.

## Current State

- Linux 1.0.0 source and build pipeline are in the repository.
- Local `main` contains the Linux1 work.
- The feature branch still exists as `origin/feature/linux1-authentic`.
- The Linux1 worktree still exists locally.
- Linux1 generated build byproducts under `linux-1.0.0/` are local scratch outputs and should remain ignored rather than committed.
