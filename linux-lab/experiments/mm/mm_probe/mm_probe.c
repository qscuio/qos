// SPDX-License-Identifier: GPL-2.0
/*
 * mm_probe.c — kprobe companion for the mm experiments.
 *
 * Registers three kprobes on the kernel fault handlers triggered by the
 * four userspace programs in linux-lab/experiments/mm/:
 *
 *   do_anonymous_page  — anon_fault (FAULT_FLAG_WRITE set)
 *                        zero_page  (FAULT_FLAG_WRITE clear, after MADV_DONTNEED)
 *   do_wp_page         — cow_fault
 *   handle_userfault   — uffd
 *
 * Usage:
 *   insmod mm_probe.ko
 *   ./anon_fault   # dmesg: do_anonymous_page write=1 (64 faults)
 *   ./zero_page    # dmesg: do_anonymous_page write=0 (64 re-faults)
 *   ./cow_fault    # dmesg: do_wp_page        write=1 (64 CoW faults in child)
 *   ./uffd         # dmesg: handle_userfault  reason=0x... (8 uffd faults)
 *   rmmod mm_probe
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/mm_types.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("kprobe companion for mm experiments");

/* ── do_anonymous_page ─────────────────────────────────────────────────── */

static int handler_anon(struct kprobe *kp, struct pt_regs *regs)
{
	struct vm_fault *vmf = (struct vm_fault *)regs->di;

	pr_info("mm_probe: do_anonymous_page addr=0x%lx vm_start=0x%lx vm_flags=0x%lx write=%d\n",
		vmf->address,
		vmf->vma->vm_start,
		vmf->vma->vm_flags,
		!!(vmf->flags & FAULT_FLAG_WRITE));
	return 0;
}

static struct kprobe kp_anon = {
	.symbol_name = "do_anonymous_page",
	.pre_handler = handler_anon,
};

/* ── do_wp_page ────────────────────────────────────────────────────────── */

static int handler_wp(struct kprobe *kp, struct pt_regs *regs)
{
	struct vm_fault *vmf = (struct vm_fault *)regs->di;

	pr_info("mm_probe: do_wp_page         addr=0x%lx vm_start=0x%lx vm_flags=0x%lx write=%d\n",
		vmf->address,
		vmf->vma->vm_start,
		vmf->vma->vm_flags,
		!!(vmf->flags & FAULT_FLAG_WRITE));
	return 0;
}

static struct kprobe kp_wp = {
	.symbol_name = "do_wp_page",
	.pre_handler = handler_wp,
};

/* ── handle_userfault ──────────────────────────────────────────────────── */

static int handler_uffd(struct kprobe *kp, struct pt_regs *regs)
{
	struct vm_fault *vmf = (struct vm_fault *)regs->di;
	unsigned long reason = regs->si;

	pr_info("mm_probe: handle_userfault   addr=0x%lx vm_start=0x%lx vm_flags=0x%lx reason=0x%lx\n",
		vmf->address,
		vmf->vma->vm_start,
		vmf->vma->vm_flags,
		reason);
	return 0;
}

static struct kprobe kp_uffd = {
	.symbol_name = "handle_userfault",
	.pre_handler = handler_uffd,
};

/* ── init / exit ───────────────────────────────────────────────────────── */

static int __init mm_probe_init(void)
{
	int ret;

	ret = register_kprobe(&kp_anon);
	if (ret < 0) {
		pr_err("mm_probe: register_kprobe(do_anonymous_page) failed: %d\n", ret);
		return ret;
	}

	ret = register_kprobe(&kp_wp);
	if (ret < 0) {
		pr_err("mm_probe: register_kprobe(do_wp_page) failed: %d\n", ret);
		unregister_kprobe(&kp_anon);
		return ret;
	}

	ret = register_kprobe(&kp_uffd);
	if (ret < 0) {
		pr_err("mm_probe: register_kprobe(handle_userfault) failed: %d\n", ret);
		unregister_kprobe(&kp_wp);
		unregister_kprobe(&kp_anon);
		return ret;
	}

	pr_info("mm_probe: loaded — probing do_anonymous_page, do_wp_page, handle_userfault\n");
	return 0;
}

static void __exit mm_probe_exit(void)
{
	unregister_kprobe(&kp_uffd);
	unregister_kprobe(&kp_wp);
	unregister_kprobe(&kp_anon);
	pr_info("mm_probe: unloaded\n");
}

module_init(mm_probe_init);
module_exit(mm_probe_exit);
