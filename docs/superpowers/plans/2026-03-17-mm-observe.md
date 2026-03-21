# MM Observe Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add four progressive experiments that make VA→PA translation concrete: a userspace pagemap reader (`va_to_pa`), a kernel module page-table walker (`mm_walk`), a QEMU monitor guide + script, and compile-time debug prints in the c-os and rust-os VMM.

**Architecture:** Four independent features committed separately. `va_to_pa` joins the existing `mm-experiments` catalog group (no orchestrator changes). `mm_walk` gets its own group (mirrors `mm-probe` pattern exactly). QEMU guide is pure documentation + shell script. c-os/rust-os changes are `#ifdef`/`#[cfg]`-gated with no runtime effect when disabled.

**Tech Stack:** C (userspace + kernel module), kprobes/proc API, Bash, Python (catalog/tests), YAML (catalog/profile), Rust (rust-os kernel)

---

## Task 1: `va_to_pa` userspace experiment

**Files:**
- Create: `linux-lab/experiments/mm/va_to_pa/va_to_pa.c`
- Create: `linux-lab/catalog/examples/mm-va-to-pa.yaml`
- Modify: `linux-lab/orchestrator/core/examples.py:42` — append `"mm-va-to-pa"` to `mm-experiments` priority list
- Modify: `tests/test_linux_lab_example_catalog.py` — add `test_mm_va_to_pa_catalog_entry_is_valid`
- Modify: `tests/test_linux_lab_example_assets.py:208` — add `"mm-va-to-pa"` to hardcoded key set

- [ ] **Step 1: Write the two failing tests**

Append to `tests/test_linux_lab_example_catalog.py`:

```python
def test_mm_va_to_pa_catalog_entry_is_valid() -> None:
    module = _load_module("linux_lab_example_catalog_mm_va_to_pa", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)
    assert "mm-va-to-pa" in catalog
    entry = catalog["mm-va-to-pa"]
    assert entry["enabled"] is True
    assert entry["kind"] == "userspace"
    assert entry["category"] == "memory"
    assert entry["origin"] == "qos-native"
    assert entry["build_mode"] == "gcc-userspace"
    assert entry["source"] == "linux-lab/experiments/mm/va_to_pa"
    assert "mm-experiments" in entry.get("groups", [])
```

- [ ] **Step 2: Run to confirm failure**

```bash
cd /home/ubuntu/work/qos
python -m pytest tests/test_linux_lab_example_catalog.py::test_mm_va_to_pa_catalog_entry_is_valid -v
```

Expected: FAIL — `"mm-va-to-pa" not in catalog`.

- [ ] **Step 3: Create `linux-lab/catalog/examples/mm-va-to-pa.yaml`**

```yaml
key: mm-va-to-pa
kind: userspace
category: memory
source: linux-lab/experiments/mm/va_to_pa
origin: qos-native
build_mode: gcc-userspace
tags:
  - userspace
  - mm
  - page-fault
  - pagemap
groups:
  - mm-experiments
enabled: true
notes: >
  Reads /proc/self/pagemap to show VA->PA translation for a stack variable,
  a heap page before/after first touch (demand paging), and a fork (CoW).
  Must be run with sudo — PFN field in pagemap requires CAP_SYS_ADMIN since
  kernel 4.0.
```

- [ ] **Step 4: Create `linux-lab/experiments/mm/va_to_pa/va_to_pa.c`**

```c
// SPDX-License-Identifier: GPL-2.0
/*
 * va_to_pa.c — translate virtual addresses to physical addresses via
 * /proc/self/pagemap, demonstrating demand paging and copy-on-write.
 *
 * Requires sudo (CAP_SYS_ADMIN) — the PFN field in pagemap entries is
 * restricted since kernel 4.0.
 *
 * Kernel paths exercised:
 *   heap touch before/after: do_anonymous_page()
 *   child write after fork:  do_wp_page()
 *
 * Usage:
 *   gcc -O0 -o va_to_pa va_to_pa.c && sudo ./va_to_pa
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define PAGE_SIZE  4096UL
#define PAGE_SHIFT 12

/* /proc/self/pagemap entry layout (64 bits per page) */
typedef union {
    uint64_t raw;
    struct {
        uint64_t pfn        : 55; /* bits 54:0 — PFN when present */
        uint64_t soft_dirty : 1;  /* bit 55 */
        uint64_t exclusive  : 1;  /* bit 56 */
        uint64_t _unused    : 4;  /* bits 60:57 */
        uint64_t file_mapped: 1;  /* bit 61 */
        uint64_t swapped    : 1;  /* bit 62 */
        uint64_t present    : 1;  /* bit 63 */
    };
} pagemap_entry_t;

/*
 * va_to_pa — translate a virtual address to a physical address.
 * Returns the physical address, or 0 if the page is not present.
 * Prints a diagnostic if the page is not yet faulted in or if
 * /proc/self/pagemap cannot be read (e.g., missing CAP_SYS_ADMIN).
 */
static uint64_t va_to_pa(const void *va)
{
    int fd;
    uint64_t vpn, offset;
    pagemap_entry_t entry;
    uint64_t pa;

    fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd < 0) {
        perror("  open /proc/self/pagemap");
        return 0;
    }

    vpn    = (uint64_t)(uintptr_t)va >> PAGE_SHIFT;
    offset = vpn * sizeof(uint64_t);

    if (pread(fd, &entry, sizeof(entry), (off_t)offset) != sizeof(entry)) {
        if (errno == EPERM) {
            fprintf(stderr, "  ERROR: /proc/self/pagemap PFN read requires "
                    "CAP_SYS_ADMIN — run with sudo\n");
        } else {
            perror("  pread pagemap");
        }
        close(fd);
        return 0;
    }
    close(fd);

    if (!entry.present) {
        printf("  page NOT present (not yet faulted in by kernel)\n");
        return 0;
    }

    pa = (entry.pfn << PAGE_SHIFT) | ((uint64_t)(uintptr_t)va & (PAGE_SIZE - 1));
    return pa;
}

int main(void)
{
    uint64_t pa, parent_pa, child_pa_before, child_pa_after;
    char *heap;
    pid_t pid;

    /* ── 1. Stack variable (always present) ───────────────────────── */
    int x = 42;
    printf("\n[1] stack variable x=%d\n", x);
    printf("    VA  = %p\n", (void *)&x);
    pa = va_to_pa(&x);
    if (pa)
        printf("    PA  = 0x%lx  PFN = 0x%lx\n", (unsigned long)pa,
               (unsigned long)(pa >> PAGE_SHIFT));

    /* ── 2. Heap page before first touch ──────────────────────────── */
    heap = malloc(PAGE_SIZE);
    if (!heap) { perror("malloc"); return 1; }

    printf("\n[2] heap page — before first touch\n");
    printf("    VA  = %p\n", (void *)heap);
    va_to_pa(heap); /* prints "NOT present" — do_anonymous_page not fired yet */

    /* ── 3. Heap page after first write — do_anonymous_page fires ─── */
    heap[0] = (char)0xAB; /* <─── THIS triggers do_anonymous_page() */

    printf("\n[3] heap page — after first write (do_anonymous_page fired)\n");
    printf("    VA  = %p\n", (void *)heap);
    pa = va_to_pa(heap);
    if (pa)
        printf("    PA  = 0x%lx  PFN = 0x%lx  byte@PA = 0x%02x\n",
               (unsigned long)pa, (unsigned long)(pa >> PAGE_SHIFT),
               (unsigned char)heap[0]);

    /* ── 4. Fork — same VA, same PA until child writes ────────────── */
    printf("\n[4] fork — CoW demonstration\n");
    heap[0] = (char)0x11;
    parent_pa = va_to_pa(heap);
    printf("    parent: VA=%p  PA=0x%lx  val=0x%02x\n",
           (void *)heap, (unsigned long)parent_pa, (unsigned char)heap[0]);

    pid = fork();
    if (pid < 0) { perror("fork"); free(heap); return 1; }

    if (pid == 0) {
        /* child: read before write — page still shared with parent */
        child_pa_before = va_to_pa(heap);
        printf("    child (before write): VA=%p  PA=0x%lx  same_as_parent=%s\n",
               (void *)heap, (unsigned long)child_pa_before,
               child_pa_before == parent_pa ? "YES" : "NO");

        heap[0] = (char)0x22; /* <─── THIS triggers do_wp_page() — CoW */

        child_pa_after = va_to_pa(heap);
        printf("    child (after  write): VA=%p  PA=0x%lx  new_page=%s\n",
               (void *)heap, (unsigned long)child_pa_after,
               child_pa_after != parent_pa ? "YES" : "NO");
        free(heap);
        _exit(0);
    }

    wait(NULL);
    free(heap);
    return 0;
}
```

- [ ] **Step 5: Update `GROUP_ENTRY_PRIORITY` in `examples.py` (line 42)**

Change:
```python
    "mm-experiments": ["mm-anon-fault", "mm-cow-fault", "mm-zero-page", "mm-uffd"],
```
To:
```python
    "mm-experiments": ["mm-anon-fault", "mm-cow-fault", "mm-zero-page", "mm-uffd", "mm-va-to-pa"],
```

- [ ] **Step 6: Update `test_mm_experiments_entries_appear_in_default_lab_dry_run` (line 208)**

Change:
```python
    assert mm_keys == {"mm-anon-fault", "mm-cow-fault", "mm-zero-page", "mm-uffd"}
```
To:
```python
    assert mm_keys == {"mm-anon-fault", "mm-cow-fault", "mm-zero-page", "mm-uffd", "mm-va-to-pa"}
```

- [ ] **Step 7: Run the catalog test to confirm it passes**

```bash
cd /home/ubuntu/work/qos
python -m pytest tests/test_linux_lab_example_catalog.py::test_mm_va_to_pa_catalog_entry_is_valid -v
```

Expected: PASS.

- [ ] **Step 8: Run the full test suite to confirm no regressions**

```bash
cd /home/ubuntu/work/qos
python -m pytest tests/ -v 2>&1 | tail -20
```

Expected: all tests pass.

- [ ] **Step 9: Commit**

```bash
cd /home/ubuntu/work/qos
git add linux-lab/experiments/mm/va_to_pa/va_to_pa.c \
        linux-lab/catalog/examples/mm-va-to-pa.yaml \
        linux-lab/orchestrator/core/examples.py \
        tests/test_linux_lab_example_catalog.py \
        tests/test_linux_lab_example_assets.py
git commit -m "feat(mm-observe): add va_to_pa userspace experiment and catalog entry"
```

---

## Task 2: `mm_walk` kernel module

**Files:**
- Create: `linux-lab/experiments/mm/mm_walk/mm_walk.c`
- Create: `linux-lab/experiments/mm/mm_walk/Makefile`
- Create: `linux-lab/catalog/examples/mm-walk.yaml`
- Modify: `linux-lab/orchestrator/core/examples.py` — add mm-walk group, kind, priority, planners lambda
- Modify: `linux-lab/manifests/profiles/mm-experiments.yaml` — add `"mm-walk"` to `example_groups`
- Modify: `tests/test_linux_lab_example_catalog.py` — two new tests
- Modify: `tests/test_linux_lab_example_assets.py:119` — add `"mm-walk"` to dry-run group list

- [ ] **Step 1: Write the two failing catalog tests**

Append to `tests/test_linux_lab_example_catalog.py`:

```python
def test_mm_walk_catalog_entry_is_valid() -> None:
    module = _load_module("linux_lab_example_catalog_mm_walk", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)
    assert "mm-walk" in catalog
    entry = catalog["mm-walk"]
    assert entry["enabled"] is True
    assert entry["kind"] == "module"
    assert entry["category"] == "memory"
    assert entry["origin"] == "qos-native"
    assert entry["build_mode"] == "kbuild-module"
    assert entry["source"] == "linux-lab/experiments/mm/mm_walk"
    assert "mm-walk" in entry.get("groups", [])


def test_mm_walk_group_is_registered_in_example_planner() -> None:
    module = _load_module("linux_lab_examples_mm_walk_reg", EXAMPLES_MODULE_PATH)
    assert "mm-walk" in module.EXAMPLE_GROUP_ORDER
    assert module.GROUP_KIND_MAP.get("mm-walk") == "module"
    assert "mm-walk" in module.GROUP_ENTRY_PRIORITY
```

- [ ] **Step 2: Run to confirm both fail**

```bash
cd /home/ubuntu/work/qos
python -m pytest tests/test_linux_lab_example_catalog.py::test_mm_walk_catalog_entry_is_valid tests/test_linux_lab_example_catalog.py::test_mm_walk_group_is_registered_in_example_planner -v
```

Expected: both FAIL.

- [ ] **Step 3: Update `examples.py` — add mm-walk to all four data structures**

In `linux-lab/orchestrator/core/examples.py`:

**`EXAMPLE_GROUP_ORDER`** (line 19 — append after `"mm-probe"`):
```python
EXAMPLE_GROUP_ORDER = [
    "modules-core",
    "userspace-core",
    "rust-core",
    "bpf-core",
    "modules-all",
    "userspace-all",
    "rust-all",
    "bpf-all",
    "mm-experiments",
    "mm-probe",
    "mm-walk",
]
```

**`GROUP_KIND_MAP`** (after `"mm-probe": "module"`):
```python
    "mm-walk": "module",
```

**`GROUP_ENTRY_PRIORITY`** (after `"mm-probe": ["mm-probe"]`):
```python
    "mm-walk": ["mm-walk"],
```

**`planners` dict inside `resolve_example_plan`** (after the `"mm-probe"` lambda, before the closing `}`):
```python
        "mm-walk": lambda: _modules_plan(
            "mm-walk",
            linux_lab_root,
            build_root,
            kernel_tree=kernel_tree,
            arch=arch,
            toolchain_prefix=toolchain_prefix,
            kernel_version=kernel_version,
        ),
```

Note: `build_root` is positional (third argument), not `build_root=build_root` — this matches the mm-probe lambda exactly. The mm-experiments lambda uses `build_root=build_root` as a keyword — do NOT copy that pattern for a module group.

- [ ] **Step 4: Run `test_mm_walk_group_is_registered_in_example_planner` to confirm pass**

```bash
cd /home/ubuntu/work/qos
python -m pytest tests/test_linux_lab_example_catalog.py::test_mm_walk_group_is_registered_in_example_planner -v
```

Expected: PASS.

- [ ] **Step 5: Update `mm-experiments.yaml` profile**

In `linux-lab/manifests/profiles/mm-experiments.yaml`, change:
```yaml
example_groups:
  - "mm-experiments"
  - "mm-probe"
```
To:
```yaml
example_groups:
  - "mm-experiments"
  - "mm-probe"
  - "mm-walk"
```

- [ ] **Step 6: Update `test_run_dry_run_emits_example_metadata` (line 113–120)**

Change:
```python
    assert [item["group"] for item in example_plans] == [
        "modules-core",
        "userspace-core",
        "rust-core",
        "bpf-core",
        "mm-experiments",
        "mm-probe",
    ]
```
To:
```python
    assert [item["group"] for item in example_plans] == [
        "modules-core",
        "userspace-core",
        "rust-core",
        "bpf-core",
        "mm-experiments",
        "mm-probe",
        "mm-walk",
    ]
```

- [ ] **Step 7: Create `linux-lab/catalog/examples/mm-walk.yaml`**

```yaml
key: mm-walk
kind: module
category: memory
source: linux-lab/experiments/mm/mm_walk
origin: qos-native
build_mode: kbuild-module
tags:
  - module
  - mm
  - pagetable
groups:
  - mm-walk
enabled: true
notes: >
  Kernel module that walks the 4-level page table (PGD->P4D->PUD->PMD->PTE)
  for any PID+VA via a /proc/mm_walk interface. Write "<pid> <va>" to trigger
  a walk; read to see the last result. Defaults: PID=1, VA=0x400000 (walked
  once at insmod).
```

- [ ] **Step 8: Create `linux-lab/experiments/mm/mm_walk/Makefile`**

```makefile
obj-m := mm_walk.o
```

- [ ] **Step 9: Create `linux-lab/experiments/mm/mm_walk/mm_walk.c`**

```c
// SPDX-License-Identifier: GPL-2.0
/*
 * mm_walk.c — on-demand page table walker via /proc/mm_walk
 *
 * Walks the 4-level page table (PGD→P4D→PUD→PMD→PTE) for a target
 * PID and virtual address, printing each descriptor's kernel virtual
 * address, raw value, and decoded flags — the same walk the hardware
 * MMU performs on every memory access.
 *
 * Interface:
 *   echo "<pid> <va>" > /proc/mm_walk   # trigger walk (va: hex or decimal)
 *   cat /proc/mm_walk                   # read last result
 *
 * Defaults: PID=1, VA=0x400000 (walked once at module load).
 *
 * Output example:
 *   mm_walk: PID=1234 VA=0x7fff12340000
 *     PGD[0x1ff] @ ffff... val=0x... present=1
 *     P4D[0x000] @ ffff... val=0x... present=1
 *     PUD[0x16c] @ ffff... val=0x... present=1
 *     PMD[0x158] @ ffff... val=0x... present=1
 *     PTE[0x101] @ ffff... val=0x... P=1 RW=1 US=1 D=1 A=1 NX=0
 *     PFN=0x1a4f7  PA=0x1a4f71000
 *
 * Usage:
 *   insmod mm_walk.ko
 *   cat /proc/mm_walk                   # default walk result
 *   echo "$(pgrep va_to_pa) 0x$(cat /proc/$(pgrep va_to_pa)/maps | \
 *       awk '/heap/{print $1}' | cut -d- -f1)" > /proc/mm_walk
 *   cat /proc/mm_walk
 *   rmmod mm_walk
 */
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("On-demand page table walker via /proc/mm_walk");

#define RESULT_BUF_SIZE 1024

static char result_buf[RESULT_BUF_SIZE];
static struct proc_dir_entry *proc_entry;

/* ── page table walk ───────────────────────────────────────────────── */

static void do_walk(pid_t target_pid, unsigned long va)
{
    struct task_struct *task;
    struct mm_struct *mm;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    int len = 0;

    /* header */
    len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                    "mm_walk: PID=%d VA=0x%lx\n", target_pid, va);
    pr_info("%s", result_buf);

    task = get_pid_task(find_get_pid(target_pid), PIDTYPE_PID);
    if (!task) {
        len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                        "  error: pid %d not found\n", target_pid);
        return;
    }

    mm = get_task_mm(task);
    if (!mm) {
        len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                        "  error: pid %d has no mm\n", target_pid);
        put_task_struct(task);
        return;
    }

    mmap_read_lock(mm);

    /* PGD */
    pgd = pgd_offset(mm, va);
    len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                    "  PGD[0x%lx] @ %px  val=0x%lx  present=%d\n",
                    pgd_index(va), pgd, pgd_val(*pgd), !pgd_none(*pgd));
    pr_info("  PGD[0x%lx] @ %px  val=0x%lx  present=%d\n",
            pgd_index(va), pgd, pgd_val(*pgd), !pgd_none(*pgd));
    if (pgd_none(*pgd) || pgd_bad(*pgd)) {
        len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                        "  PGD: not present\n");
        goto out;
    }

    /* P4D */
    p4d = p4d_offset(pgd, va);
    len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                    "  P4D[0x%lx] @ %px  val=0x%lx  present=%d\n",
                    p4d_index(va), p4d, p4d_val(*p4d), !p4d_none(*p4d));
    pr_info("  P4D[0x%lx] @ %px  val=0x%lx  present=%d\n",
            p4d_index(va), p4d, p4d_val(*p4d), !p4d_none(*p4d));
    if (p4d_none(*p4d) || p4d_bad(*p4d)) {
        len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                        "  P4D: not present\n");
        goto out;
    }

    /* PUD */
    pud = pud_offset(p4d, va);
    len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                    "  PUD[0x%lx] @ %px  val=0x%lx  present=%d\n",
                    pud_index(va), pud, pud_val(*pud), !pud_none(*pud));
    pr_info("  PUD[0x%lx] @ %px  val=0x%lx  present=%d\n",
            pud_index(va), pud, pud_val(*pud), !pud_none(*pud));
    if (pud_none(*pud) || pud_bad(*pud)) {
        len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                        "  PUD: not present\n");
        goto out;
    }

    /* PMD */
    pmd = pmd_offset(pud, va);
    len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                    "  PMD[0x%lx] @ %px  val=0x%lx  present=%d\n",
                    pmd_index(va), pmd, pmd_val(*pmd), !pmd_none(*pmd));
    pr_info("  PMD[0x%lx] @ %px  val=0x%lx  present=%d\n",
            pmd_index(va), pmd, pmd_val(*pmd), !pmd_none(*pmd));
    if (pmd_none(*pmd) || pmd_bad(*pmd)) {
        len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                        "  PMD: not present\n");
        goto out;
    }

    /* PTE */
    pte = pte_offset_map(pmd, va);
    if (!pte) {
        len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                        "  PTE: pte_offset_map returned NULL\n");
        goto out;
    }
    len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                    "  PTE[0x%lx] @ %px  val=0x%lx"
                    "  P=%d RW=%d US=%d D=%d A=%d NX=%d\n",
                    pte_index(va), pte, pte_val(*pte),
                    pte_present(*pte),
                    pte_write(*pte),
                    !!(pte_flags(*pte) & _PAGE_USER),
                    pte_dirty(*pte),
                    pte_young(*pte),
                    !pte_exec(*pte));
    pr_info("  PTE[0x%lx] @ %px  val=0x%lx  P=%d RW=%d US=%d D=%d A=%d NX=%d\n",
            pte_index(va), pte, pte_val(*pte),
            pte_present(*pte), pte_write(*pte),
            !!(pte_flags(*pte) & _PAGE_USER),
            pte_dirty(*pte), pte_young(*pte), !pte_exec(*pte));

    if (pte_present(*pte)) {
        unsigned long pfn = pte_pfn(*pte);
        unsigned long pa  = (pfn << PAGE_SHIFT) | (va & ~PAGE_MASK);
        len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                        "  PFN=0x%lx  PA=0x%lx\n", pfn, pa);
        pr_info("  PFN=0x%lx  PA=0x%lx\n", pfn, pa);
    } else {
        len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                        "  PTE: not present\n");
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
    pte_unmap(pte);
#endif

out:
    mmap_read_unlock(mm);
    mmput(mm);
    put_task_struct(task);
}

/* ── /proc read: return last result ───────────────────────────────── */

static ssize_t mm_walk_read(struct file *file, char __user *buf,
                            size_t count, loff_t *ppos)
{
    return simple_read_from_buffer(buf, count, ppos,
                                   result_buf, strlen(result_buf));
}

/* ── /proc write: parse "<pid> <va>", run walk ────────────────────── */

static ssize_t mm_walk_write(struct file *file, const char __user *buf,
                             size_t count, loff_t *ppos)
{
    char kbuf[64];
    pid_t pid;
    unsigned long va;

    if (count >= sizeof(kbuf))
        return -EINVAL;
    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;
    kbuf[count] = '\0';

    if (sscanf(kbuf, "%d %li", &pid, &va) != 2)
        return -EINVAL;

    memset(result_buf, 0, sizeof(result_buf));
    do_walk(pid, va);
    return (ssize_t)count;
}

/* ── proc_ops / file_operations (kernel version compat) ────────────── */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops mm_walk_fops = {
    .proc_read  = mm_walk_read,
    .proc_write = mm_walk_write,
};
#else
static const struct file_operations mm_walk_fops = {
    .read  = mm_walk_read,
    .write = mm_walk_write,
};
#endif

/* ── init / exit ───────────────────────────────────────────────────── */

static int __init mm_walk_init(void)
{
    proc_entry = proc_create("mm_walk", 0666, NULL, &mm_walk_fops);
    if (!proc_entry) {
        pr_err("mm_walk: failed to create /proc/mm_walk\n");
        return -ENOMEM;
    }

    /* run default walk so cat /proc/mm_walk works immediately */
    do_walk(1, 0x400000UL);
    pr_info("mm_walk: loaded — /proc/mm_walk ready (default PID=1 VA=0x400000)\n");
    return 0;
}

static void __exit mm_walk_exit(void)
{
    proc_remove(proc_entry);
    pr_info("mm_walk: unloaded\n");
}

module_init(mm_walk_init);
module_exit(mm_walk_exit);
```

- [ ] **Step 10: Run catalog test to confirm it passes**

```bash
cd /home/ubuntu/work/qos
python -m pytest tests/test_linux_lab_example_catalog.py::test_mm_walk_catalog_entry_is_valid -v
```

Expected: PASS.

- [ ] **Step 11: Run the full test suite**

```bash
cd /home/ubuntu/work/qos
python -m pytest tests/ -v 2>&1 | tail -20
```

Expected: all tests pass.

- [ ] **Step 12: Commit**

```bash
cd /home/ubuntu/work/qos
git add linux-lab/experiments/mm/mm_walk/mm_walk.c \
        linux-lab/experiments/mm/mm_walk/Makefile \
        linux-lab/catalog/examples/mm-walk.yaml \
        linux-lab/orchestrator/core/examples.py \
        linux-lab/manifests/profiles/mm-experiments.yaml \
        tests/test_linux_lab_example_catalog.py \
        tests/test_linux_lab_example_assets.py
git commit -m "feat(mm-observe): add mm-walk kernel module with /proc interface"
```

---

## Task 3: QEMU monitor guide and helper script

**Files:**
- Create: `docs/linux-lab/qemu-mm-monitor.md`
- Create: `scripts/qemu-mm-walk.sh`

No tests — these are documentation and tooling only.

- [ ] **Step 1: Create `docs/linux-lab/qemu-mm-monitor.md`**

```markdown
# QEMU Monitor — MM Inspection Commands

The QEMU monitor exposes the hardware MMU state of your guest directly,
without needing a kernel module or userspace program. These commands are
most useful when an mm experiment is paused under GDB.

## Setup: Enable the monitor socket

Start QEMU with:

```
-monitor unix:/tmp/qemu-monitor.sock,server,nowait
```

or use the `scripts/qemu-mm-walk.sh` helper (see below).

Switch to the monitor from the QEMU window with `Ctrl+Alt+2`, or connect
from a second terminal:

```bash
socat - UNIX-CONNECT:/tmp/qemu-monitor.sock
# or
nc -U /tmp/qemu-monitor.sock
```

---

## `info tlb` — dump all TLB entries

Shows every translation cached in the guest TLB right now. Entries flush
after context switches and `mmap`/`munmap` calls.

```
(qemu) info tlb
```

Sample output:
```
0x00007fff12340000: 0x000000001a4f7003 ----A--UW
```
Format: `<va>: <pa+flags> <decoded-flags>`. The `U` = user, `W` = writable,
`A` = accessed.

---

## `info mem` — dump all mapped VA ranges

Shows every VA range currently in the guest page tables with permissions.
Useful to confirm a page is mapped before trying `gva2gpa`.

```
(qemu) info mem
```

Sample output:
```
0000000000400000-0000000000401000 0000000000001000 -r-
00007fff12340000-00007fff12341000 0000000000001000 urw
```

---

## `gva2gpa <va>` — translate guest virtual → guest physical

QEMU does the full 4-level page table walk in software and returns the PA.
If the page is not present (not yet faulted in), the command fails.

```
(qemu) gva2gpa 0x55a3f2b01000
```

Success:
```
0x00055a3f2b01000 -> 0x000000001a4f7000
```

Failure (page not present):
```
gva2gpa: Translate address failed
```

### Worked example with `va_to_pa`

This shows `do_anonymous_page()` made visible at the hardware level:

1. Compile and start `va_to_pa` under GDB inside the guest:
   ```
   gcc -O0 -g -o va_to_pa va_to_pa.c
   gdb ./va_to_pa
   ```

2. Set a breakpoint just before the first heap write:
   ```
   (gdb) break va_to_pa.c:93   # the line: heap[0] = (char)0xAB;
   (gdb) run
   (gdb) print heap            # note the VA, e.g. 0x55a3f2b01000
   ```

3. In the QEMU monitor, translate before the write:
   ```
   (qemu) gva2gpa 0x55a3f2b01000
   → gva2gpa: Translate address failed   ← page not present
   ```

4. Step over the write in GDB:
   ```
   (gdb) next
   ```
   `do_anonymous_page()` has just fired in the kernel.

5. Translate again:
   ```
   (qemu) gva2gpa 0x55a3f2b01000
   → 0x00055a3f2b01000 -> 0x000000001a4f7000   ← real PA
   ```

6. Read the byte back from physical memory:
   ```
   (qemu) xp /1xb 0x1a4f7000
   → 00000000 1a4f7000: 0xab
   ```

The byte `0xab` — written to a virtual address that didn't exist a moment
ago — is sitting at a real physical address in guest RAM.

---

## `xp /Nxb <pa>` — read bytes from guest physical memory

Reads `N` bytes from guest physical address `pa`, regardless of whether
any VA currently maps it. Format: `/N` = count, `x` = hex, `b` = byte.

```
(qemu) xp /4xb 0x1a4f7000
00000000 1a4f7000: 0xab 0x00 0x00 0x00
```

---

## Helper script

`scripts/qemu-mm-walk.sh <va>` automates the `gva2gpa` + `xp` sequence.
It connects to `/tmp/qemu-monitor.sock`, translates the VA, and reads 4
bytes from the resulting PA.

```bash
./scripts/qemu-mm-walk.sh 0x55a3f2b01000
```
```

- [ ] **Step 2: Create `scripts/qemu-mm-walk.sh`**

```bash
#!/usr/bin/env bash
# qemu-mm-walk.sh — translate a guest VA to PA via the QEMU monitor,
# then read 4 bytes from the physical address.
#
# Usage: ./scripts/qemu-mm-walk.sh <va>
#   va: guest virtual address (hex with 0x prefix, e.g. 0x55a3f2b01000)
#
# Requires: socat OR nc with Unix socket support
# Requires: QEMU started with -monitor unix:/tmp/qemu-monitor.sock,server,nowait

set -euo pipefail

SOCK="/tmp/qemu-monitor.sock"
VA="${1:-}"

if [[ -z "$VA" ]]; then
    echo "Usage: $0 <va>"
    echo "  va: guest virtual address (e.g. 0x55a3f2b01000)"
    exit 1
fi

# Verify socket exists
if [[ ! -S "$SOCK" ]]; then
    echo "Error: QEMU monitor socket not found at $SOCK"
    echo ""
    echo "Start QEMU with:"
    echo "  -monitor unix:${SOCK},server,nowait"
    exit 1
fi

# Pick a monitor client tool
if command -v socat &>/dev/null; then
    send_cmd() { printf '%s\n' "$1" | socat - "UNIX-CONNECT:${SOCK}" 2>/dev/null; }
elif nc -U /dev/null 2>/dev/null; then
    send_cmd() { printf '%s\n' "$1" | nc -U "$SOCK" 2>/dev/null; }
else
    echo "Error: neither socat nor nc (with -U) found"
    echo "  Install socat: apt install socat"
    exit 1
fi

echo "==> gva2gpa $VA"
gva_output=$(send_cmd "gva2gpa $VA")
echo "$gva_output"

# Parse PA from "0x<va> -> 0x<pa>" line
PA=$(echo "$gva_output" | grep -oP '-> \K0x[0-9a-fA-F]+' || true)
if [[ -z "$PA" ]]; then
    echo "gva2gpa failed — page is not present (not yet faulted in)"
    exit 1
fi

echo ""
echo "==> xp /4xb $PA"
send_cmd "xp /4xb $PA"
```

- [ ] **Step 3: Make script executable**

```bash
chmod +x /home/ubuntu/work/qos/scripts/qemu-mm-walk.sh
```

- [ ] **Step 4: Commit**

```bash
cd /home/ubuntu/work/qos
git add docs/linux-lab/qemu-mm-monitor.md scripts/qemu-mm-walk.sh
git commit -m "feat(mm-observe): add QEMU monitor guide and helper script"
```

---

## Task 4: c-os and rust-os `MM_DEBUG` instrumentation

**Files:**
- Modify: `c-os/kernel/mm/mm.c:1,209-236` — add `#include <stdio.h>` and `MM_DEBUG` block
- Modify: `rust-os/kernel/src/lib.rs:1,1627-1629` — add `extern crate std` and `mm-debug` block
- Modify: `rust-os/kernel/Cargo.toml` — add `[features]` section

No new tests — the guards are inactive by default and do not affect existing test suites.

- [ ] **Step 1: Add `#include <stdio.h>` and debug block to `c-os/kernel/mm/mm.c`**

Add `#include <stdio.h>` as the third include (after `<string.h>`):

At the top of `c-os/kernel/mm/mm.c`, the current includes are:
```c
#include <stddef.h>
#include <string.h>
```
Change to:
```c
#include <stddef.h>
#include <stdio.h>
#include <string.h>
```

In `qos_vmm_map_as`, after line 235 (`g_vflags[asid][(uint32_t)idx] = flags;`) and before `return 0;`:

```c
    g_pas[asid][(uint32_t)idx] = pa;
    g_vflags[asid][(uint32_t)idx] = flags;
#ifdef QOS_MM_DEBUG
    printf("[mm_debug] vmm_map_as: asid=%u VA=0x%lx -> PA=0x%lx PFN=0x%lx flags=0x%x\n",
           asid, (unsigned long)va, (unsigned long)pa,
           (unsigned long)(pa >> 12), flags);
#endif
    return 0;
```

Enable with `-DQOS_MM_DEBUG` added to `CFLAGS` in the c-os build.

- [ ] **Step 2: Add `extern crate std` and debug block to `rust-os/kernel/src/lib.rs`**

After the `#![no_std]` line at the top of the file, add:
```rust
#[cfg(test)]
extern crate std;
```

In `vmm_map_as` (around line 1627), after `state.flags[idx] = flags;` and before `Ok(())`:

Current code in the closure:
```rust
        state.pas[idx] = pa;
        state.flags[idx] = flags;
        Ok(())
```

Change to:
```rust
        state.pas[idx] = pa;
        state.flags[idx] = flags;
        #[cfg(all(feature = "mm-debug", test))]
        std::eprintln!(
            "[mm_debug] vmm_map_as: asid={} VA={:#x} -> PA={:#x} PFN={:#x} flags={:#x}",
            asid, va, pa, pa >> 12, flags
        );
        Ok(())
```

- [ ] **Step 3: Add `mm-debug` feature to `rust-os/kernel/Cargo.toml`**

After the `[dependencies]` section, add:
```toml
[features]
mm-debug = []
```

Enable with `cargo test --features mm-debug` in the rust-os kernel directory.

- [ ] **Step 4: Verify existing c-os tests still pass**

```bash
cd /home/ubuntu/work/qos
python -m pytest tests/ -k "c_os or cos or kernel" -v 2>&1 | tail -20
```

If no c-os specific pytest tests exist, verify the full suite:
```bash
python -m pytest tests/ -v 2>&1 | tail -20
```

Expected: all tests pass (the `#ifdef` block is inactive without `-DQOS_MM_DEBUG`).

- [ ] **Step 5: Verify existing rust-os tests still pass**

```bash
cd /home/ubuntu/work/qos/rust-os/kernel
cargo test 2>&1 | tail -20
```

Expected: all tests pass. The `#[cfg(all(feature = "mm-debug", test))]` block is
inactive without `--features mm-debug`.

- [ ] **Step 6: Verify rust-os tests pass with feature enabled**

```bash
cd /home/ubuntu/work/qos/rust-os/kernel
cargo test --features mm-debug 2>&1 | tail -30
```

Expected: all tests pass. Debug lines like `[mm_debug] vmm_map_as: ...` appear in
the test output for every VMM map call made during tests.

- [ ] **Step 7: Run the full python test suite one final time**

```bash
cd /home/ubuntu/work/qos
python -m pytest tests/ -v 2>&1 | tail -20
```

Expected: all tests pass.

- [ ] **Step 8: Commit**

```bash
cd /home/ubuntu/work/qos
git add c-os/kernel/mm/mm.c \
        rust-os/kernel/src/lib.rs \
        rust-os/kernel/Cargo.toml
git commit -m "feat(mm-observe): add MM_DEBUG instrumentation to c-os and rust-os vmm_map"
```
