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
gva2gpa 0x55a3f2b01000 -> 0x000000001a4f7000
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
   → gva2gpa 0x55a3f2b01000 -> 0x000000001a4f7000   ← real PA
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
