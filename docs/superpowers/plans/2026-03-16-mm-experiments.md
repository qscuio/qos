# MM Experiments Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `linux-lab/experiments/mm/` with four C programs that trigger specific page-fault paths (anonymous, CoW, zero-page, userfaultfd), wired into the manifest system and enabled in `default-lab`.

**Architecture:** Each experiment is a self-contained C program in its own subdirectory under `linux-lab/experiments/mm/`. Four catalog entries in `catalog/examples/` register them under group `mm-experiments`. A new concrete profile `mm-experiments.yaml` exposes the group; `default-lab.yaml` expands to include it. The orchestrator's `examples.py` is extended with one new group entry, dispatching to the existing `_userspace_plan` planner.

**Tech Stack:** C (gcc), pytest, YAML manifests, Python orchestrator

---

## Chunk 1: Orchestrator wiring, profile, and default-lab update

### Task 1: Register `mm-experiments` in `examples.py`

**Files:**
- Modify: `linux-lab/orchestrator/core/examples.py`
- Test: `tests/test_linux_lab_example_catalog.py`

- [ ] **Step 1: Write the failing test**

Add to `tests/test_linux_lab_example_catalog.py`:

```python
def test_mm_experiments_group_is_registered_in_example_planner() -> None:
    module = _load_module("linux_lab_examples_mm_reg", EXAMPLES_MODULE_PATH)
    assert "mm-experiments" in module.EXAMPLE_GROUP_ORDER
    assert module.GROUP_KIND_MAP.get("mm-experiments") == "userspace"
    assert "mm-experiments" in module.GROUP_ENTRY_PRIORITY
```

- [ ] **Step 2: Run test to verify it fails**

```bash
pytest tests/test_linux_lab_example_catalog.py::test_mm_experiments_group_is_registered_in_example_planner -v
```

Expected: FAIL with `AssertionError` — `"mm-experiments"` not in `EXAMPLE_GROUP_ORDER`.

- [ ] **Step 3: Extend `examples.py`**

In `linux-lab/orchestrator/core/examples.py`:

Add `"mm-experiments"` to the end of `EXAMPLE_GROUP_ORDER`:

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
]
```

Add to `GROUP_KIND_MAP`:

```python
GROUP_KIND_MAP = {
    ...existing entries...,
    "mm-experiments": "userspace",
}
```

Add to `GROUP_ENTRY_PRIORITY`:

```python
GROUP_ENTRY_PRIORITY = {
    ...existing entries...,
    "mm-experiments": ["mm-anon-fault", "mm-cow-fault", "mm-zero-page", "mm-uffd"],
}
```

Add the planner lambda in `resolve_example_plan` inside the `planners` dict:

```python
"mm-experiments": lambda: _userspace_plan(
    "mm-experiments",
    linux_lab_root,
    build_root=build_root,
    kernel_tree=kernel_tree,
    arch=arch,
    toolchain_prefix=toolchain_prefix,
    kernel_version=kernel_version,
),
```

> **Warning:** All four changes (EXAMPLE_GROUP_ORDER, GROUP_KIND_MAP, GROUP_ENTRY_PRIORITY, planners lambda) must be made together in one edit. If the planners lambda is omitted, the dry-run in Task 3 will crash with `KeyError: 'mm-experiments'` rather than a test assertion failure.

- [ ] **Step 4: Run test to verify it passes**

```bash
pytest tests/test_linux_lab_example_catalog.py::test_mm_experiments_group_is_registered_in_example_planner -v
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add linux-lab/orchestrator/core/examples.py tests/test_linux_lab_example_catalog.py
git commit -m "feat(linux-lab): register mm-experiments group in example planner"
```

---

### Task 2: Create `mm-experiments` profile manifest

**Files:**
- Create: `linux-lab/manifests/profiles/mm-experiments.yaml`
- Test: `tests/test_linux_lab_phase1_resolution.py`

- [ ] **Step 1: Write the failing test**

Add to `tests/test_linux_lab_phase1_resolution.py`:

```python
def test_mm_experiments_profile_loads_and_has_correct_example_group() -> None:
    manifests_mod = _load_module("linux_lab_manifests_mm", MANIFESTS_MODULE)
    manifests = manifests_mod.load_manifests(LINUX_LAB_ROOT / "manifests")
    assert "mm-experiments" in manifests.profiles
    profile = manifests.profiles["mm-experiments"]
    assert profile.kind == "concrete"
    assert "mm-experiments" in profile.example_groups
```

- [ ] **Step 2: Run test to verify it fails**

```bash
pytest tests/test_linux_lab_phase1_resolution.py::test_mm_experiments_profile_loads_and_has_correct_example_group -v
```

Expected: FAIL — `"mm-experiments"` not in `manifests.profiles`.

- [ ] **Step 3: Create the profile**

Create `linux-lab/manifests/profiles/mm-experiments.yaml`:

```yaml
key: "mm-experiments"
kind: "concrete"
precedence: 60
replaces: []
expands_to: []
fragment_ref: null
compatible_kernels:
  - "*"
compatible_arches:
  - "*"
host_tools: []
tool_groups: []
example_groups:
  - "mm-experiments"
post_build_refs: []
```

- [ ] **Step 4: Run test to verify it passes**

```bash
pytest tests/test_linux_lab_phase1_resolution.py::test_mm_experiments_profile_loads_and_has_correct_example_group -v
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add linux-lab/manifests/profiles/mm-experiments.yaml tests/test_linux_lab_phase1_resolution.py
git commit -m "feat(linux-lab): add mm-experiments concrete profile"
```

---

### Task 3: Add `mm-experiments` to `default-lab` and fix broken tests

Adding `mm-experiments` to `default-lab`'s `expands_to` breaks two existing tests that assert exact profile lists. Fix those tests first (TDD: make them reflect the new expected state), then update the manifest.

**Files:**
- Modify: `linux-lab/manifests/profiles/default-lab.yaml`
- Modify: `tests/test_linux_lab_phase1_resolution.py` (line 56)
- Modify: `tests/test_linux_lab_example_assets.py` (line 113)

- [ ] **Step 1: Update the two affected tests to reflect the post-change expectation**

In `tests/test_linux_lab_phase1_resolution.py`, find:

```python
    assert request.profiles == ["debug", "bpf", "rust", "samples", "debug-tools"]
```

Replace with:

```python
    assert request.profiles == ["debug", "bpf", "rust", "samples", "debug-tools", "mm-experiments"]
```

In `tests/test_linux_lab_example_assets.py`, find:

```python
    assert [item["group"] for item in example_plans] == [
        "modules-core",
        "userspace-core",
        "rust-core",
        "bpf-core",
    ]
```

Replace with:

```python
    assert [item["group"] for item in example_plans] == [
        "modules-core",
        "userspace-core",
        "rust-core",
        "bpf-core",
        "mm-experiments",
    ]
```

- [ ] **Step 2: Run the two tests to confirm they now fail (pre-manifest change)**

```bash
pytest tests/test_linux_lab_phase1_resolution.py::test_manifest_defaults_and_schema_contract \
       tests/test_linux_lab_example_assets.py::test_run_dry_run_emits_example_metadata -v
```

Expected: both FAIL — `default-lab` still expands to the old list.

- [ ] **Step 3: Add `mm-experiments` to `supported_profiles` in all kernel manifests**

The orchestrator validates each resolved concrete profile against the kernel's `supported_profiles` list and raises an error if any profile is missing. Add `"mm-experiments"` to every kernel manifest so the profile is universally accepted.

In each of these files, add `- "mm-experiments"` to `supported_profiles`:

- `linux-lab/manifests/kernels/4.19.317.yaml`
- `linux-lab/manifests/kernels/6.4.3.yaml`
- `linux-lab/manifests/kernels/6.9.6.yaml`
- `linux-lab/manifests/kernels/6.9.8.yaml`
- `linux-lab/manifests/kernels/6.10.yaml`
- `linux-lab/manifests/kernels/6.18.4.yaml`

For each file, find the `supported_profiles:` list and append:
```yaml
  - "mm-experiments"
```

- [ ] **Step 4: Update `default-lab.yaml`**

In `linux-lab/manifests/profiles/default-lab.yaml`, add `mm-experiments` to `expands_to`:

```yaml
key: "default-lab"
kind: "meta"
precedence: 0
replaces: []
expands_to:
  - "debug"
  - "bpf"
  - "rust"
  - "samples"
  - "debug-tools"
  - "mm-experiments"
fragment_ref: null
compatible_kernels:
  - "*"
compatible_arches:
  - "*"
host_tools: []
tool_groups: []
example_groups: []
post_build_refs: []
```

- [ ] **Step 5: Run both tests to verify they pass**

```bash
pytest tests/test_linux_lab_phase1_resolution.py::test_manifest_defaults_and_schema_contract \
       tests/test_linux_lab_example_assets.py::test_run_dry_run_emits_example_metadata -v
```

Expected: both PASS.

- [ ] **Step 6: Run all linux-lab resolution tests to confirm no regressions**

```bash
pytest tests/test_linux_lab_phase1_resolution.py tests/test_linux_lab_example_catalog.py -v
```

Expected: all PASS.

- [ ] **Step 7: Commit**

```bash
git add linux-lab/manifests/profiles/default-lab.yaml \
        linux-lab/manifests/kernels/ \
        tests/test_linux_lab_phase1_resolution.py \
        tests/test_linux_lab_example_assets.py
git commit -m "feat(linux-lab): enable mm-experiments in default-lab profile"
```

---

## Chunk 2: Catalog entries and C source files

### Task 4: Write the four C experiment programs

Each program lives in its own subdirectory so the catalog can reference it independently and `uffd` can specify its own build command with `-lpthread`.

**Files:**
- Create: `linux-lab/experiments/mm/anon_fault/anon_fault.c`
- Create: `linux-lab/experiments/mm/cow_fault/cow_fault.c`
- Create: `linux-lab/experiments/mm/zero_page/zero_page.c`
- Create: `linux-lab/experiments/mm/uffd/uffd.c`
- Test: `tests/test_linux_lab_example_assets.py`

- [ ] **Step 1: Write the failing source-file existence test**

Add to `tests/test_linux_lab_example_assets.py`:

```python
def test_mm_experiment_source_files_exist() -> None:
    expected = [
        ROOT / "linux-lab" / "experiments" / "mm" / "anon_fault" / "anon_fault.c",
        ROOT / "linux-lab" / "experiments" / "mm" / "cow_fault" / "cow_fault.c",
        ROOT / "linux-lab" / "experiments" / "mm" / "zero_page" / "zero_page.c",
        ROOT / "linux-lab" / "experiments" / "mm" / "uffd" / "uffd.c",
    ]
    missing = [str(p) for p in expected if not p.is_file()]
    assert missing == [], f"missing mm experiment source files: {missing}"
```

- [ ] **Step 2: Run test to verify it fails**

```bash
pytest tests/test_linux_lab_example_assets.py::test_mm_experiment_source_files_exist -v
```

Expected: FAIL — directories don't exist yet.

- [ ] **Step 3: Write `anon_fault.c`**

Create `linux-lab/experiments/mm/anon_fault/anon_fault.c`:

```c
/*
 * anon_fault.c — trigger anonymous page faults via mmap + first write
 *
 * Kernel path: handle_mm_fault -> do_anonymous_page
 * GDB:         break do_anon_fault
 * perf:        perf stat -e minor-faults ./anon_fault
 */
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGES     64
#define PAGE_SIZE 4096

static volatile char sink;

static void do_anon_fault(char *map, size_t len)
{
    /* Writing each page for the first time faults in a new anonymous page. */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        map[i] = (char)i;
    sink = map[0];
}

int main(void)
{
    size_t len = (size_t)PAGES * PAGE_SIZE;

    printf("anon_fault: mmap %zu bytes, write each page\n", len);
    printf("  kernel path : handle_mm_fault -> do_anonymous_page\n");
    printf("  GDB         : break do_anon_fault\n");
    printf("  perf        : perf stat -e minor-faults ./anon_fault\n\n");

    char *map = mmap(NULL, len, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    do_anon_fault(map, len);

    printf("touched %d pages — check minor-faults count with perf\n", PAGES);
    munmap(map, len);
    return 0;
}
```

- [ ] **Step 4: Write `cow_fault.c`**

Create `linux-lab/experiments/mm/cow_fault/cow_fault.c`:

```c
/*
 * cow_fault.c — trigger copy-on-write faults via fork + child write
 *
 * Kernel path: handle_mm_fault -> do_wp_page
 * GDB:         break do_cow_write
 * perf:        perf stat -e minor-faults ./cow_fault
 */
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define PAGES     64
#define PAGE_SIZE 4096

static volatile char sink;

static void do_cow_write(char *map, size_t len)
{
    /* Each write breaks the shared CoW mapping, triggering do_wp_page. */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        map[i] = 0xAB;
    sink = map[0];
}

int main(void)
{
    size_t len = (size_t)PAGES * PAGE_SIZE;

    printf("cow_fault: mmap, fill in parent, fork, child writes all pages\n");
    printf("  kernel path : handle_mm_fault -> do_wp_page\n");
    printf("  GDB         : break do_cow_write\n");
    printf("  perf        : perf stat -e minor-faults ./cow_fault\n\n");

    char *map = mmap(NULL, len, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    /* Materialize pages in the parent so they are shared after fork. */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        map[i] = 0x01;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        do_cow_write(map, len);
        printf("child: wrote %d pages (CoW fault per page)\n", PAGES);
        return 0;
    }

    int status;
    waitpid(pid, &status, 0);
    printf("parent: child exited — check minor-faults count with perf\n");
    munmap(map, len);
    return 0;
}
```

- [ ] **Step 5: Write `zero_page.c`**

Create `linux-lab/experiments/mm/zero_page/zero_page.c`:

```c
/*
 * zero_page.c — trigger zero-page faults via madvise(MADV_DONTNEED) + re-read
 *
 * Kernel path: handle_mm_fault -> do_anonymous_page (zero page)
 * GDB:         break do_zero_page_read
 * perf:        perf stat -e minor-faults ./zero_page
 */
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGES     64
#define PAGE_SIZE 4096

static volatile char sink;

static void do_zero_page_read(char *map, size_t len)
{
    /*
     * After MADV_DONTNEED the kernel drops physical pages. Re-reading
     * each page faults in the shared zero page.
     */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        sink = map[i];
}

int main(void)
{
    size_t len = (size_t)PAGES * PAGE_SIZE;

    printf("zero_page: mmap, write, madvise(MADV_DONTNEED), re-read\n");
    printf("  kernel path : handle_mm_fault -> do_anonymous_page (zero page)\n");
    printf("  GDB         : break do_zero_page_read\n");
    printf("  perf        : perf stat -e minor-faults ./zero_page\n\n");

    char *map = mmap(NULL, len, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    /* Materialize pages so MADV_DONTNEED has something to release. */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        map[i] = 0xFF;

    printf("releasing pages with MADV_DONTNEED...\n");
    if (madvise(map, len, MADV_DONTNEED) < 0) {
        perror("madvise");
        return 1;
    }

    printf("re-reading (faults in zero pages)...\n");
    do_zero_page_read(map, len);

    printf("re-read %d pages — check minor-faults count with perf\n", PAGES);
    munmap(map, len);
    return 0;
}
```

- [ ] **Step 6: Write `uffd.c`**

Create `linux-lab/experiments/mm/uffd/uffd.c`:

```c
/*
 * uffd.c — register a mapping with userfaultfd; handler thread serves faults
 *
 * Kernel path: handle_mm_fault -> handle_userfault
 * GDB:         break do_uffd_access
 * perf:        perf stat -e minor-faults ./uffd
 * build:       gcc -g -O0 -Wall -Wextra -o uffd uffd.c -lpthread
 * note:        uffd fd is blocking; handler thread reads() until close()
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define PAGES     8
#define PAGE_SIZE 4096

static volatile char sink;
static int    g_uffd;
static size_t g_len;

static void *fault_handler(void *arg)
{
    (void)arg;
    static char zero_page[PAGE_SIZE];   /* zero-filled by BSS */

    for (;;) {
        struct uffd_msg msg;
        ssize_t n = read(g_uffd, &msg, sizeof(msg));
        if (n == 0)
            break;
        if (n < 0) {
            perror("read uffd");
            break;
        }
        if (msg.event != UFFD_EVENT_PAGEFAULT)
            continue;

        unsigned long addr = msg.arg.pagefault.address & ~((unsigned long)PAGE_SIZE - 1);
        printf("  uffd fault at 0x%lx\n", addr);

        struct uffdio_copy copy = {
            .dst  = addr,
            .src  = (unsigned long)zero_page,
            .len  = PAGE_SIZE,
            .mode = 0,
        };
        if (ioctl(g_uffd, UFFDIO_COPY, &copy) < 0) {
            perror("UFFDIO_COPY");
            break;
        }
    }
    return NULL;
}

static void do_uffd_access(char *map, size_t len)
{
    /* Each read faults through the userfaultfd handler. */
    for (size_t i = 0; i < len; i += PAGE_SIZE)
        sink = map[i];
}

int main(void)
{
    printf("uffd: register mapping, read triggers UFFD_EVENT_PAGEFAULT\n");
    printf("  kernel path : handle_mm_fault -> handle_userfault\n");
    printf("  GDB         : break do_uffd_access\n");
    printf("  perf        : perf stat -e minor-faults ./uffd\n\n");

    long fd = syscall(SYS_userfaultfd, O_CLOEXEC);
    if (fd < 0) {
        perror("userfaultfd");
        return 1;
    }
    g_uffd = (int)fd;

    struct uffdio_api api = { .api = UFFD_API };
    if (ioctl(g_uffd, UFFDIO_API, &api) < 0) {
        perror("UFFDIO_API");
        return 1;
    }

    size_t len  = (size_t)PAGES * PAGE_SIZE;
    g_len = len;
    char *map   = mmap(NULL, len, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    struct uffdio_register reg = {
        .range = { .start = (unsigned long)map, .len = len },
        .mode  = UFFDIO_REGISTER_MODE_MISSING,
    };
    if (ioctl(g_uffd, UFFDIO_REGISTER, &reg) < 0) {
        perror("UFFDIO_REGISTER");
        return 1;
    }

    pthread_t thr;
    pthread_create(&thr, NULL, fault_handler, NULL);

    printf("accessing %d registered pages...\n", PAGES);
    do_uffd_access(map, len);

    close(g_uffd);
    pthread_join(thr, NULL);

    printf("done: %d uffd faults handled\n", PAGES);
    munmap(map, len);
    return 0;
}
```

- [ ] **Step 7: Run source-file existence test to verify it passes**

```bash
pytest tests/test_linux_lab_example_assets.py::test_mm_experiment_source_files_exist -v
```

Expected: PASS.

- [ ] **Step 8: Commit**

```bash
git add linux-lab/experiments/mm/ tests/test_linux_lab_example_assets.py
git commit -m "feat(linux-lab): add mm experiment C sources (anon_fault, cow_fault, zero_page, uffd)"
```

---

### Task 5: Write catalog entries and wire them end-to-end

**Files:**
- Create: `linux-lab/catalog/examples/mm-anon-fault.yaml`
- Create: `linux-lab/catalog/examples/mm-cow-fault.yaml`
- Create: `linux-lab/catalog/examples/mm-zero-page.yaml`
- Create: `linux-lab/catalog/examples/mm-uffd.yaml`
- Test: `tests/test_linux_lab_example_catalog.py`
- Test: `tests/test_linux_lab_example_assets.py`

- [ ] **Step 1: Update `test_custom_make_entries_are_explicitly_wired_and_enabled` to include `mm-uffd`**

`mm-uffd.yaml` uses `build_mode: custom-make`. An existing test in `tests/test_linux_lab_example_catalog.py` asserts a hardcoded sorted list of all `custom-make` keys. Adding `mm-uffd` inserts it between `linux_kernel_hacking` and `sBPF` (lexicographic order). Update the assertion before creating the YAML:

In `tests/test_linux_lab_example_catalog.py`, find:

```python
    assert custom_keys == [
        "LiME",
        "afxdp",
        "bds_lkm_ftrace",
        "kernel-hook-framework",
        "linux_kernel_hacking",
        "sBPF",
        "xdp_ipv6_filter",
    ]
```

Replace with:

```python
    assert custom_keys == [
        "LiME",
        "afxdp",
        "bds_lkm_ftrace",
        "kernel-hook-framework",
        "linux_kernel_hacking",
        "mm-uffd",
        "sBPF",
        "xdp_ipv6_filter",
    ]
```

- [ ] **Step 2: Run the updated test to confirm it now fails (pre-catalog-entry)**

```bash
pytest tests/test_linux_lab_example_catalog.py::test_custom_make_entries_are_explicitly_wired_and_enabled -v
```

Expected: FAIL — `mm-uffd` not yet in the catalog.

- [ ] **Step 3: Write the failing catalog-entry test**

Add to `tests/test_linux_lab_example_catalog.py`:

```python
def test_mm_experiments_catalog_entries_are_valid_and_enabled() -> None:
    module = _load_module("linux_lab_example_catalog_mm", MODULE_PATH)
    catalog = module.load_example_catalog(CATALOG_ROOT)

    mm_keys = {"mm-anon-fault", "mm-cow-fault", "mm-zero-page", "mm-uffd"}
    assert mm_keys.issubset(catalog), f"missing mm catalog entries: {mm_keys - catalog.keys()}"

    for key in mm_keys:
        entry = catalog[key]
        assert entry["enabled"] is True
        assert entry["kind"] == "userspace"
        assert entry["category"] == "memory"
        assert entry["build_mode"] in {"gcc-userspace", "custom-make"}
        assert "mm-experiments" in entry.get("groups", [])
```

- [ ] **Step 4: Write a failing end-to-end dry-run test for `mm-experiments`**

Add to `tests/test_linux_lab_example_assets.py`:

```python
def test_mm_experiments_entries_appear_in_default_lab_dry_run() -> None:
    result = _run(
        [
            str(BIN_LINUX_LAB),
            "run",
            "--kernel", "6.18.4",
            "--arch", "x86_64",
            "--image", "noble",
            "--profile", "default-lab",
            "--dry-run",
        ]
    )
    assert result.returncode == 0, result.stderr

    request_root = _latest_request_root()
    example_state = json.loads(
        (request_root / "state" / "build-examples.json").read_text(encoding="utf-8")
    )
    mm_plan = next(
        (item for item in example_state["metadata"]["example_plans"]
         if item["group"] == "mm-experiments"),
        None,
    )
    assert mm_plan is not None, "mm-experiments group missing from default-lab dry-run"
    mm_keys = {entry["key"] for entry in mm_plan["entries"]}
    assert mm_keys == {"mm-anon-fault", "mm-cow-fault", "mm-zero-page", "mm-uffd"}
```

- [ ] **Step 5: Run both tests to verify they fail**

```bash
pytest tests/test_linux_lab_example_catalog.py::test_mm_experiments_catalog_entries_are_valid_and_enabled \
       tests/test_linux_lab_example_assets.py::test_mm_experiments_entries_appear_in_default_lab_dry_run -v
```

Expected: both FAIL — catalog entries don't exist yet.

- [ ] **Step 6: Create `mm-anon-fault.yaml`**

Create `linux-lab/catalog/examples/mm-anon-fault.yaml`:

```yaml
key: mm-anon-fault
kind: userspace
category: memory
source: linux-lab/experiments/mm/anon_fault
origin: qos-native
build_mode: gcc-userspace
tags:
  - userspace
  - mm
  - page-fault
groups:
  - mm-experiments
enabled: true
notes: Triggers anonymous page fault path (handle_mm_fault -> do_anonymous_page).
```

- [ ] **Step 7: Create `mm-cow-fault.yaml`**

Create `linux-lab/catalog/examples/mm-cow-fault.yaml`:

```yaml
key: mm-cow-fault
kind: userspace
category: memory
source: linux-lab/experiments/mm/cow_fault
origin: qos-native
build_mode: gcc-userspace
tags:
  - userspace
  - mm
  - cow
groups:
  - mm-experiments
enabled: true
notes: Triggers copy-on-write fault path (handle_mm_fault -> do_wp_page).
```

- [ ] **Step 8: Create `mm-zero-page.yaml`**

Create `linux-lab/catalog/examples/mm-zero-page.yaml`:

```yaml
key: mm-zero-page
kind: userspace
category: memory
source: linux-lab/experiments/mm/zero_page
origin: qos-native
build_mode: gcc-userspace
tags:
  - userspace
  - mm
  - zero-page
  - madvise
groups:
  - mm-experiments
enabled: true
notes: Triggers zero-page re-fault path via madvise(MADV_DONTNEED) + re-read.
```

- [ ] **Step 9: Create `mm-uffd.yaml`**

`uffd.c` links against `pthread`, so this entry uses `build_commands` to supply `-lpthread` explicitly.

Create `linux-lab/catalog/examples/mm-uffd.yaml`:

```yaml
key: mm-uffd
kind: userspace
category: memory
source: linux-lab/experiments/mm/uffd
origin: qos-native
build_mode: custom-make
build_commands:
  - ["gcc", "-g", "-O0", "-Wall", "-Wextra",
     "-o", "{entry_root}/uffd",
     "{entry_root}/uffd.c",
     "-lpthread"]
tags:
  - userspace
  - mm
  - userfaultfd
groups:
  - mm-experiments
enabled: true
notes: Registers a mapping with userfaultfd; handler thread serves UFFD_EVENT_PAGEFAULT.
```

- [ ] **Step 10: Run both tests to verify they pass**

```bash
pytest tests/test_linux_lab_example_catalog.py::test_mm_experiments_catalog_entries_are_valid_and_enabled \
       tests/test_linux_lab_example_assets.py::test_mm_experiments_entries_appear_in_default_lab_dry_run -v
```

Expected: both PASS.

- [ ] **Step 11: Run the full linux-lab test suite to confirm no regressions**

```bash
pytest tests/test_linux_lab_example_catalog.py \
       tests/test_linux_lab_example_assets.py \
       tests/test_linux_lab_phase1_resolution.py \
       tests/test_linux_lab_phase1_repo.py -v
```

Expected: all PASS.

- [ ] **Step 12: Commit**

```bash
git add linux-lab/catalog/examples/mm-anon-fault.yaml \
        linux-lab/catalog/examples/mm-cow-fault.yaml \
        linux-lab/catalog/examples/mm-zero-page.yaml \
        linux-lab/catalog/examples/mm-uffd.yaml \
        tests/test_linux_lab_example_catalog.py \
        tests/test_linux_lab_example_assets.py
git commit -m "feat(linux-lab): add mm-experiments catalog entries and end-to-end tests"
```
