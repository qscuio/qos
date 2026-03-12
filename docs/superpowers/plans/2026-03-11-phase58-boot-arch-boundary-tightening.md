# Phase 58 Boot Arch Boundary Tightening Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Tighten architecture boundary evidence for x86_64 and aarch64 boot flows against the design spec.

**Architecture:** Validate launcher command contracts in dry-run mode and extend serial smoke markers with explicit arch-boundary fields (paging/long-mode/kernel VA for x86, EL/MMU/TTBR/DTB-discovery for aarch64).

**Tech Stack:** Bash build scripts, pytest, QEMU smoke

---

## Chunk 1: TDD Contract

### Task 1: Add failing boot boundary tests

**Files:**
- Create: `tests/test_boot_arch_boundary.py`

- [x] **Step 1: Add dry-run launcher contract checks for x86_64 and aarch64 scripts**
- [x] **Step 2: Add smoke-log marker checks for explicit arch boundary fields**
- [x] **Step 3: Run `pytest -q tests/test_boot_arch_boundary.py` and confirm red**

## Chunk 2: Implementation

### Task 2: Add missing boundary markers in boot artifact builders

**Files:**
- Modify: `scripts/build_x86_boot_img.sh`
- Modify: `scripts/build_aarch64_kernel.sh`

- [x] **Step 1: Add x86 markers (`paging=pae`, `long_mode=1`, `kernel_va_high=1`)**
- [x] **Step 2: Add aarch64 markers (`el=el1`, `mmu=on`, `ttbr_split=user-kernel`, `virtio_discovery=dtb`)**
- [x] **Step 3: Keep existing marker set and launch behavior intact**

## Chunk 3: Verification

### Task 3: Validate phase behavior

**Files:**
- Modify: `docs/superpowers/plans/2026-03-11-phase58-boot-arch-boundary-tightening.md`

- [x] **Step 1: Run `pytest -q tests/test_boot_arch_boundary.py`**
