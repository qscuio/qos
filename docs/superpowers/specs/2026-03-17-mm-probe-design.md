# MM Probe Design

## Goal

Add a kernel module companion to the `mm-experiments` group. The module registers three kprobes that instrument the kernel fault handler functions triggered by the four existing userspace experiments. Loading the module before running an experiment makes the kernel side of each fault path visible in `dmesg`.

## Context

The four userspace programs in `linux-lab/experiments/mm/` trigger specific page-fault paths but show only the outcome. This module provides the kernel view: at the exact moment each fault handler fires, it prints the `vm_fault` fields — fault address, VMA start, VMA flags, and fault flags — so both ends of the fault are observable simultaneously.

## Architecture

### Source location

`linux-lab/experiments/mm/mm_probe/` — alongside the userspace experiments.

Two files:
- `mm_probe.c` — the module source
- `Makefile` — standard kbuild single-module Makefile (`obj-m := mm_probe.o`)

### Catalog entry

`linux-lab/catalog/examples/mm-probe.yaml`:

```yaml
key: mm-probe
kind: module
category: memory
source: linux-lab/experiments/mm/mm_probe
origin: qos-native
build_mode: kbuild-module
groups:
  - mm-experiments
enabled: true
notes: kprobe module that instruments do_anonymous_page, do_wp_page, and handle_userfault — the three kernel fault handlers triggered by the mm experiments.
```

No new profile or manifest change is needed. The `mm-experiments` group already exists and is enabled in `default-lab`.

### Three probes, four experiments

There are three distinct kernel entry points for the four userspace experiments:

| Probe target | Triggered by |
|---|---|
| `do_anonymous_page(struct vm_fault *vmf)` | `anon_fault` (write, `FAULT_FLAG_WRITE` set) and `zero_page` (read after `MADV_DONTNEED`, flag clear) |
| `do_wp_page(struct vm_fault *vmf)` | `cow_fault` |
| `handle_userfault(struct vm_fault *vmf, unsigned long reason)` | `uffd` |

All three functions take `struct vm_fault *vmf` as their first argument. On x86-64 this is in `rdi`, accessible via `regs->di` in the kprobe pre-handler.

### Handler output

Each pre-handler prints one `pr_info` line per fault:

```
mm_probe: do_anonymous_page addr=0x... vm_start=0x... vm_flags=0x... write=1
mm_probe: do_wp_page         addr=0x... vm_start=0x... vm_flags=0x... write=1
mm_probe: handle_userfault   addr=0x... vm_start=0x... vm_flags=0x... reason=0x...
```

The `do_anonymous_page` handler prints `write=1` when `FAULT_FLAG_WRITE` is set (anon_fault experiment) and `write=0` when clear (zero_page experiment after `MADV_DONTNEED`). This distinguishes the two experiments in `dmesg` output.

### Workflow

```bash
insmod mm_probe.ko
./anon_fault    # dmesg: do_anonymous_page write=1 lines (64 faults)
./zero_page     # dmesg: do_anonymous_page write=0 lines (64 re-faults)
./cow_fault     # dmesg: do_wp_page lines (64 CoW faults in child)
./uffd          # dmesg: handle_userfault lines (8 uffd faults)
rmmod mm_probe
```

## Components

### `mm_probe.c`

- Three `struct kprobe` instances, one per target function
- Each `pre_handler` casts `regs->di` to `struct vm_fault *` and calls `pr_info`
- `__init` registers all three probes; bails out cleanly if any registration fails (unregisters the ones that succeeded)
- `__exit` unregisters all three
- `MODULE_LICENSE("GPL")` — required for kprobe access

### `Makefile`

Standard kbuild out-of-tree module Makefile, same pattern as `linux-lab/examples/modules/debug/Makefile`.

## Testing

One new test in `tests/test_linux_lab_example_catalog.py`:

```python
def test_mm_probe_catalog_entry_is_valid() -> None:
    module = _load_module("linux_lab_example_catalog_mm_probe", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)
    assert "mm-probe" in catalog
    entry = catalog["mm-probe"]
    assert entry["enabled"] is True
    assert entry["kind"] == "module"
    assert entry["build_mode"] == "kbuild-module"
    assert "mm-experiments" in entry.get("groups", [])
```

No compilation test — kbuild modules require a kernel tree not present in CI.

## Error handling

If `register_kprobe` fails for any probe (e.g., the symbol is inlined or not present in the running kernel), the init function logs the error, unregisters any probes already registered, and returns the error code. The module load fails visibly rather than silently missing probes.
