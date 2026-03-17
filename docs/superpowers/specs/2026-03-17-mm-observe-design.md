# MM Observe Design

## Goal

Add four progressive experiments that make virtual-to-physical address translation
concrete and visible, building on the existing mm-experiments programs:

1. **`va_to_pa`** — userspace program that reads `/proc/self/pagemap` to translate
   VAs to PAs, demonstrating demand paging and CoW with real physical addresses.
2. **`mm_walk`** — kernel module with a `/proc/mm_walk` interface that walks the
   4-level hardware page table for any PID+VA, printing each level's address, raw
   value, and decoded flags.
3. **QEMU monitor guide** — markdown reference doc and shell helper script for
   inspecting hardware page-table state from the QEMU monitor while an experiment
   is paused under GDB.
4. **c-os / rust-os `MM_DEBUG`** — compile-time debug prints in `qos_vmm_map` (C)
   and `vmm_map_as` (Rust), showing VA, PA, PFN, and flags at the point a mapping
   is installed.

## Context

The four programs in `linux-lab/experiments/mm/` (anon_fault, cow_fault, zero_page,
uffd) and the `mm_probe` kprobe module show what happens at fault time from the
kernel's side. These four additions close the remaining observability gaps:
- `va_to_pa` shows the *outcome* from userspace (the actual PA after the fault)
- `mm_walk` shows the *page table structure* on demand, without waiting for a fault
- The QEMU guide shows the *hardware MMU state* directly from the monitor
- The c-os/rust-os prints show the *mapping install event* in the teaching kernels

## Architecture

### Experiment 1: `va_to_pa`

**Location:** `linux-lab/experiments/mm/va_to_pa/va_to_pa.c`

**What it does:**

Demonstrates three scenarios in sequence:

1. Stack variable — VA and PA printed immediately (always present).
2. Heap page before first touch — pagemap entry shows `present=0` (not yet faulted
   in by the kernel).
3. Heap page after first write — `do_anonymous_page()` fires; PA now resolvable.
4. Fork — parent and child share the same PA before the child writes; after the
   child writes (`do_wp_page()` fires), they map to different PAs.

**`/proc/self/pagemap` access:** The PFN field requires `CAP_SYS_ADMIN` since
kernel 4.0. The program detects `EPERM` on `pread` and prints a diagnostic rather
than silently returning zero. Must be run with `sudo`.

**Catalog wiring:**
- New file: `linux-lab/catalog/examples/mm-va-to-pa.yaml`
- `key: mm-va-to-pa`, `kind: userspace`, `build_mode: gcc-userspace`,
  `category: memory`, `origin: qos-native`
- `groups: [mm-experiments]` — added to the existing group, no orchestrator changes
  needed
- `notes:` flags `sudo` requirement

**`GROUP_ENTRY_PRIORITY`:** `mm-va-to-pa` appended to the `mm-experiments` priority
list in `examples.py`.

### Experiment 2: `mm_walk`

**Location:** `linux-lab/experiments/mm/mm_walk/mm_walk.c` and `Makefile`

**What it does:**

Walks the 4-level page table (PGD → P4D → PUD → PMD → PTE) for a target PID+VA
using the kernel's `pgd_offset`, `p4d_offset`, `pud_offset`, `pmd_offset`, and
`pte_offset_map` macros — the same walk the hardware MMU performs on every memory
access. At each level it prints the kernel virtual address of the descriptor, its
raw value, and the `present` bit. At the PTE level it additionally decodes P, RW,
US, Dirty, Accessed, and NX.

**`/proc/mm_walk` interface:**

- **Write:** `echo "<pid> <va>" > /proc/mm_walk` — triggers the walk for the given
  PID and VA. VA may be decimal or `0x`-prefixed hex.
- **Read:** `cat /proc/mm_walk` — returns the formatted result of the last walk.
- **Defaults:** PID=1, VA=`0x400000`. The walk runs once at `insmod` time with
  these defaults so `cat /proc/mm_walk` works immediately without a write.

**Output format (dmesg and proc read are identical):**
```
mm_walk: PID=<pid> VA=0x<va>
  PGD[<idx>] @ <kva>  val=0x<raw>  present=<0|1>
  P4D[<idx>] @ <kva>  val=0x<raw>  present=<0|1>
  PUD[<idx>] @ <kva>  val=0x<raw>  present=<0|1>
  PMD[<idx>] @ <kva>  val=0x<raw>  present=<0|1>
  PTE[<idx>] @ <kva>  val=0x<raw>  P=<n> RW=<n> US=<n> D=<n> A=<n> NX=<n>
  PFN=0x<pfn>  PA=0x<pa>
```
If a level is not present the walk stops with `  <LEVEL>: not present`.

**Result buffer:** A static `char result_buf[1024]` in the module holds the last
walk result. `proc_write` fills the buffer; `proc_read` returns it. No dynamic
allocation.

**Catalog and orchestrator wiring:**
- New file: `linux-lab/catalog/examples/mm-walk.yaml`
  - `key: mm-walk`, `kind: module`, `build_mode: kbuild-module`,
    `category: memory`, `origin: qos-native`, `groups: [mm-walk]`
- `examples.py`: add `"mm-walk"` to `EXAMPLE_GROUP_ORDER` (after `"mm-probe"`),
  `GROUP_KIND_MAP["mm-walk"] = "module"`,
  `GROUP_ENTRY_PRIORITY["mm-walk"] = ["mm-walk"]`,
  and a `"mm-walk"` lambda in the `planners` dict inside `resolve_example_plan`
  dispatching to `_modules_plan` — matching the mm-probe lambda pattern exactly
  (positional `build_root`, not keyword).
- `mm-experiments.yaml`: add `"mm-walk"` to `example_groups`.

### Experiment 3: QEMU monitor guide

**Doc:** `docs/linux-lab/qemu-mm-monitor.md`

Four sections:
1. **Setup** — how to start QEMU with a monitor socket:
   `-monitor unix:/tmp/qemu-monitor.sock,server,nowait`
2. **`info tlb`** — show all current TLB entries
3. **`info mem`** — show all mapped VA ranges and permissions
4. **`gva2gpa <va>`** — translate a guest virtual address to guest physical
5. **`xp /Nxb <pa>`** — read bytes from guest physical memory

Each section includes a worked example using `va_to_pa` as the scenario:
set a GDB breakpoint before `heap[0] = 0xAB`, run `gva2gpa <heap_va>` (fails —
page not present), step over the write, run `gva2gpa` again (succeeds), then
`xp /1xb <pa>` reads back `0xab`.

**Script:** `scripts/qemu-mm-walk.sh`

- Accepts a VA as `$1` (required)
- Connects to `/tmp/qemu-monitor.sock` using `socat` or `nc -U`
- Runs `gva2gpa <va>`, parses the PA from the response, then runs `xp /4xb <pa>`
- If the socket is not found, prints the QEMU flag needed to enable it and exits 1
- Requires `socat` or `nc` with Unix socket support; checks and errors clearly if
  neither is available

### Experiment 4: c-os / rust-os `MM_DEBUG`

**C (`c-os/kernel/mm/mm.c` — `qos_vmm_map_as`):**

The actual mapping writes (`g_vas[asid][idx] = page_va` and `g_pas[asid][idx] = pa`)
are in `qos_vmm_map_as`, not in the one-liner wrapper `qos_vmm_map`. The debug block
goes in `qos_vmm_map_as` immediately after those writes:

```c
#ifdef QOS_MM_DEBUG
    printf("[mm_debug] vmm_map_as: VA=0x%lx -> PA=0x%lx PFN=0x%lx flags=0x%x\n",
           (unsigned long)va, (unsigned long)pa,
           (unsigned long)(pa >> 12), flags);
#endif
```

Requires `#include <stdio.h>` added to `mm.c` (not currently present). Enabled by
`-DQOS_MM_DEBUG` in the build.

**Rust (`rust-os/kernel/src/lib.rs` — `vmm_map_as`):**

`rust-os` is `#![no_std]`, so `println!` is not available in the library. The debug
print is gated on both the feature flag *and* the test harness (which links `std`).
**Note:** this means the print fires during `cargo test --features mm-debug`, not
during actual bare-metal execution (where `std` is absent). This is the appropriate
scope for a `no_std` teaching kernel — the test environment is where you observe it.

```rust
// At crate root (top of lib.rs), after #![no_std]:
#[cfg(test)]
extern crate std;
```

```rust
// Inside vmm_map_as, after the mapping is written:
#[cfg(all(feature = "mm-debug", test))]
std::eprintln!("[mm_debug] vmm_map_as: asid={} VA={:#x} -> PA={:#x} PFN={:#x} flags={:#x}",
               asid, va, pa, pa >> 12, flags);
```

Added immediately after the mapping is written to `g_vas`/`g_pas`. Enabled by
adding `mm-debug = []` to `[features]` in `rust-os/kernel/Cargo.toml` and running
tests with `--features mm-debug`. In non-test (bare-metal) builds the block
compiles away entirely.

## File Inventory

| File | Action |
|------|--------|
| `linux-lab/experiments/mm/va_to_pa/va_to_pa.c` | Create |
| `linux-lab/catalog/examples/mm-va-to-pa.yaml` | Create |
| `linux-lab/orchestrator/core/examples.py` | Modify (priority list only) |
| `linux-lab/experiments/mm/mm_walk/mm_walk.c` | Create |
| `linux-lab/experiments/mm/mm_walk/Makefile` | Create |
| `linux-lab/catalog/examples/mm-walk.yaml` | Create |
| `linux-lab/orchestrator/core/examples.py` | Modify (group, kind, priority, planners lambda) |
| `linux-lab/manifests/profiles/mm-experiments.yaml` | Modify (add mm-walk group) |
| `docs/linux-lab/` | Create directory (git tracks via first file; no `.gitkeep` needed) |
| `docs/linux-lab/qemu-mm-monitor.md` | Create |
| `scripts/qemu-mm-walk.sh` | Create |
| `c-os/kernel/mm/mm.c` | Modify (MM_DEBUG block in qos_vmm_map_as + stdio include) |
| `rust-os/kernel/src/lib.rs` | Modify (extern crate std under cfg(test), mm-debug cfg block in vmm_map_as) |
| `rust-os/kernel/Cargo.toml` | Modify (add mm-debug feature) |

## Testing

**Catalog/orchestrator tests (existing file `tests/test_linux_lab_example_catalog.py`):**
- `test_mm_va_to_pa_catalog_entry_is_valid` — verifies key, kind, build_mode,
  category, origin, groups, enabled
- `test_mm_walk_catalog_entry_is_valid` — same checks for mm-walk
- `test_mm_walk_group_is_registered_in_example_planner` — verifies EXAMPLE_GROUP_ORDER,
  GROUP_KIND_MAP, GROUP_ENTRY_PRIORITY

**Assets/dry-run tests (`tests/test_linux_lab_example_assets.py`):**
- Update `test_run_dry_run_emits_example_metadata` expected group list to include
  `"mm-walk"` after `"mm-probe"`
- Update `test_mm_experiments_entries_appear_in_default_lab_dry_run` (line 208):
  the hardcoded set `{"mm-anon-fault", "mm-cow-fault", "mm-zero-page", "mm-uffd"}`
  must include `"mm-va-to-pa"` once that entry joins the `mm-experiments` group.
  This update must land in the same commit as the catalog entry and priority list
  change (commit 1), or the test suite breaks between commits.

**c-os / rust-os:** No new tests. The `#ifdef`/`#[cfg]` guards are inactive by
default and do not affect existing test suites.

## Commit Plan

1. `feat(mm-observe): add va_to_pa userspace experiment and catalog entry`
   — includes: `va_to_pa.c`, `mm-va-to-pa.yaml`, `GROUP_ENTRY_PRIORITY` update in
   `examples.py`, and both test updates in `test_linux_lab_example_assets.py`
   (`test_mm_experiments_entries_appear_in_default_lab_dry_run` and the catalog test
   for `mm-va-to-pa`)
2. `feat(mm-observe): add mm-walk kernel module with /proc interface`
3. `feat(mm-observe): add QEMU monitor guide and helper script`
4. `feat(mm-observe): add MM_DEBUG instrumentation to c-os and rust-os vmm_map`
