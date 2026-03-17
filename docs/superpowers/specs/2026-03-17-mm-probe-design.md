# MM Probe Design

## Goal

Add a kernel module companion to the mm experiments. The module registers three kprobes that instrument the kernel fault handler functions triggered by the four existing userspace experiments. Loading the module before running an experiment makes the kernel side of each fault path visible in `dmesg`.

## Context

The four userspace programs in `linux-lab/experiments/mm/` trigger specific page-fault paths but show only the outcome. This module provides the kernel view: at the exact moment each fault handler fires, it prints the `vm_fault` fields — fault address, VMA start, VMA flags, and fault flags — so both ends of the fault are observable simultaneously.

## Architecture

### Source location

`linux-lab/experiments/mm/mm_probe/` — alongside the userspace experiments.

Two files:
- `mm_probe.c` — the module source
- `Makefile` — standard kbuild single-module Makefile (`obj-m := mm_probe.o`)

### Catalog entry and orchestrator wiring

`linux-lab/catalog/examples/mm-probe.yaml`:

```yaml
key: mm-probe
kind: module
category: memory
source: linux-lab/experiments/mm/mm_probe
origin: qos-native
build_mode: kbuild-module
tags:
  - module
  - mm
  - kprobe
groups:
  - mm-probe
enabled: true
notes: kprobe module that instruments do_anonymous_page, do_wp_page, and handle_userfault — the three kernel fault handlers triggered by the mm experiments.
```

The `mm-experiments` group in `examples.py` maps to `kind: userspace`; a `kind: module` entry in that group is silently excluded by the planner. The module therefore uses a dedicated group `mm-probe` with `kind: module`.

**Orchestrator changes required** in `linux-lab/orchestrator/core/examples.py`:
- Add `"mm-probe"` to `EXAMPLE_GROUP_ORDER` immediately after `"mm-experiments"`
- Add `"mm-probe": "module"` to `GROUP_KIND_MAP`
- Add `"mm-probe": ["mm-probe"]` to `GROUP_ENTRY_PRIORITY`
- Add a `"mm-probe"` lambda in `resolve_example_plan` dispatching to `_modules_plan`. Use the same positional `build_root` pattern as `modules-core`/`modules-all` (not the keyword `build_root=build_root` pattern used by `_userspace_plan` lambdas):
  ```python
  "mm-probe": lambda: _modules_plan(
      "mm-probe",
      linux_lab_root,
      build_root,
      kernel_tree=kernel_tree,
      arch=arch,
      toolchain_prefix=toolchain_prefix,
      kernel_version=kernel_version,
  ),
  ```

**Profile change required** in `linux-lab/manifests/profiles/mm-experiments.yaml`:
- Add `"mm-probe"` to `example_groups`

**Commit dependency**: the `examples.py` orchestrator changes and the `mm-experiments.yaml` profile change must land in the same commit (or `examples.py` must not follow `mm-experiments.yaml`). If the profile references `mm-probe` but the orchestrator does not yet have it registered, the planner will raise `KeyError: 'mm-probe'` at runtime.

Note: `mm-probe` is a catalog group, not a profile. No new profile manifest is needed and no kernel manifest change is required — `mm-probe` is a new group within the existing `mm-experiments` profile, not a new profile key.

**Commit dependency**: the catalog YAML and the source directory `linux-lab/experiments/mm/mm_probe/` must land in the same commit. The existing test `test_catalog_covers_all_qulk_module_roots` asserts that every catalog entry's `source` path exists on disk.

### Three probes, four experiments

There are three distinct kernel entry points for the four userspace experiments:

| Probe target | Triggered by |
|---|---|
| `do_anonymous_page(struct vm_fault *vmf)` | `anon_fault` (write, `FAULT_FLAG_WRITE` set) and `zero_page` (read after `MADV_DONTNEED`, flag clear) |
| `do_wp_page(struct vm_fault *vmf)` | `cow_fault` |
| `handle_userfault(struct vm_fault *vmf, unsigned long reason)` | `uffd` |

All three functions take `struct vm_fault *vmf` as their first argument. On x86-64 this is in `rdi`, accessible via `regs->di` in the kprobe pre-handler.

`do_anonymous_page` and `do_wp_page` are non-exported (static) symbols. The kprobe subsystem can probe them via kallsyms as long as `CONFIG_KPROBES=y` and they are not inlined. If the kernel is built with LTO or aggressive inlining, `register_kprobe` returns `-EINVAL` and the module init fails with a logged error.

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
- Each `pre_handler` casts `(struct vm_fault *)regs->di` to access vmf fields and calls `pr_info`
- `__init` registers all three probes; if any registration fails, it unregisters the ones that succeeded and returns the error code
- `__exit` unregisters all three
- `MODULE_LICENSE("GPL")` — required for kprobe access to non-exported symbols

### `Makefile`

Standard kbuild out-of-tree module Makefile, same pattern as `linux-lab/examples/modules/debug/Makefile`.

## Testing

**Test update required** in `tests/test_linux_lab_example_assets.py`: the `test_run_dry_run_emits_example_metadata` assertion has a hard-coded exact ordered list of expected groups. After these changes the list becomes `["modules-core", "userspace-core", "rust-core", "bpf-core", "mm-experiments", "mm-probe"]` — add `"mm-probe"` at the end.

Two new tests in `tests/test_linux_lab_example_catalog.py`:

```python
def test_mm_probe_catalog_entry_is_valid() -> None:
    module = _load_module("linux_lab_example_catalog_mm_probe", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)
    assert "mm-probe" in catalog
    entry = catalog["mm-probe"]
    assert entry["enabled"] is True
    assert entry["kind"] == "module"
    assert entry["category"] == "memory"
    assert entry["origin"] == "qos-native"
    assert entry["build_mode"] == "kbuild-module"
    assert entry["source"] == "linux-lab/experiments/mm/mm_probe"
    assert "mm-probe" in entry.get("groups", [])


def test_mm_probe_group_is_registered_in_example_planner() -> None:
    module = _load_module("linux_lab_examples_mm_probe_reg", EXAMPLES_MODULE_PATH)
    assert "mm-probe" in module.EXAMPLE_GROUP_ORDER
    assert module.GROUP_KIND_MAP.get("mm-probe") == "module"
    assert "mm-probe" in module.GROUP_ENTRY_PRIORITY
```

No compilation test — kbuild modules require a kernel tree not present in CI.

Tests confirmed unaffected (no changes needed):
- `test_full_samples_profile_emits_broad_example_metadata` — `mm-probe` is not in the `full-samples` profile
- `test_mm_experiments_entries_appear_in_default_lab_dry_run` — checks only the `mm-experiments` group entries by key, not the full ordered group list
- `test_custom_make_entries_are_explicitly_wired_and_enabled` — `mm-probe` uses `kbuild-module`, not `custom-make`

## Error handling

If `register_kprobe` fails for any probe (e.g., the symbol is inlined or absent in the running kernel), the init function logs the error, unregisters any probes already registered, and returns the error code. The module load fails visibly rather than silently skipping probes.
