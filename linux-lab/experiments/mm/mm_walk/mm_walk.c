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
 *   echo "$(pgrep va_to_pa) 0x..." > /proc/mm_walk
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
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("On-demand page table walker via /proc/mm_walk");

#define RESULT_BUF_SIZE 1024

static char result_buf[RESULT_BUF_SIZE];
static DEFINE_MUTEX(mm_walk_lock);
static struct proc_dir_entry *proc_entry;

/* ── page table walk ───────────────────────────────────────────────── */

static void do_walk(pid_t target_pid, unsigned long va)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct pid *pid_struct;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    int len = 0;

    len += snprintf(result_buf + len, RESULT_BUF_SIZE - len,
                    "mm_walk: PID=%d VA=0x%lx\n", target_pid, va);
    pr_info("%s", result_buf);

    pid_struct = find_get_pid(target_pid);
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    put_pid(pid_struct);
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
    pte_unmap(pte);

out:
    mmap_read_unlock(mm);
    mmput(mm);
    put_task_struct(task);
}

/* ── /proc read ────────────────────────────────────────────────────── */

static ssize_t mm_walk_read(struct file *file, char __user *buf,
                            size_t count, loff_t *ppos)
{
    ssize_t ret;
    mutex_lock(&mm_walk_lock);
    ret = simple_read_from_buffer(buf, count, ppos,
                                  result_buf, strlen(result_buf));
    mutex_unlock(&mm_walk_lock);
    return ret;
}

/* ── /proc write ───────────────────────────────────────────────────── */

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

    mutex_lock(&mm_walk_lock);
    memset(result_buf, 0, sizeof(result_buf));
    do_walk(pid, va);
    mutex_unlock(&mm_walk_lock);
    return (ssize_t)count;
}

/* ── proc_ops / file_operations (kernel version compat) ─────────────── */

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
