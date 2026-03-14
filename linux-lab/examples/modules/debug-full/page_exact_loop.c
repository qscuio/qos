/*
 * ch8/page_exact_loop/page_exact_loop.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming
 *
 * From: Ch 8 : Kernel Memory Allocation for Module Authors, Part 1
 ****************************************************************
 * Brief Description:
 * A quick demo of using the alloc_pages_exact() and it's counterpart
 * the free_pages_exact() APIs; they're more optimized when allocating
 * larger memory chunks via the page allocator.
 * (But pl read the rest of the chapter and Ch 9 as well!).
 *
 * For details, please refer the book, Ch 8.
 */
#include <asm/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#define OURMODNAME   "page_exact_loop"

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION
("LKP ch8/page_exact_loop: demo using the superior [alloc|free]_pages_exact() APIs");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

#define MAXTIMES    3 	/* the higher you make this, the more the chance of the
			 * alloc failing, as we only free in the cleanup code path...
			 */
static void *gptr[MAXTIMES];
/* Lets ask for 33 KB; the 'regular' PA/BSA APIs will end up giving
 * us 64 KB; the alloc_pages_exact() will optimize it
 */
static size_t gsz = 33*1024;

/* 
 * show_phy_pages - show the virtual, physical addresses and PFNs of the memory
 *            range provided on a per-page basis.
 *
 * ! NOTE   NOTE   NOTE !
 * The starting kernel address MUST be a 'linear' address, i.e., an adrress
 * within the 'lowmem' direct-mapped region of the kernel segment, else this
 * will NOT work and can possibly crash the system.
 *
 * @kaddr: the starting kernel virtual address; MUST be a 'lowmem' region addr
 * @len: length of the memory piece (bytes)
 * @contiguity_check: if True, check for physical contiguity of pages
 *
 * 'Walk' the virtually contiguous 'array' of pages one by one (i.e. page by
 * page), printing the virt and physical address (& PFN- page frame number).
 * This way, we can see if the memory really is *physically* contiguous or not.
 */
static void show_phy_pages(void *kaddr, size_t len, bool contiguity_check)
{
        void *vaddr = kaddr;
#if(BITS_PER_LONG == 64)
        const char *hdr = "-pg#-  -------va-------     --------pa--------   --PFN--\n";
#else             // 32-bit
        const char *hdr = "-pg#-  ----va----   --------pa--------   -PFN-\n";
#endif
        phys_addr_t pa;
        int loops = len/PAGE_SIZE, i;
        long pfn, prev_pfn = 1;

#ifdef CONFIG_X86
        if (!virt_addr_valid(vaddr)) {
                pr_info("%s(): invalid virtual address (0x%px)\n", __func__, vaddr);
                return;
        }
        /* Worry not, the ARM implementation of virt_to_phys() performs an internal
         * validity check
         */
#endif

        pr_info("%s(): start kaddr %px, len %zu, contiguity_check is %s\n",
                       __func__, vaddr, len, contiguity_check?"on":"off");
        pr_info("%s", hdr);
        if (len % PAGE_SIZE)
                loops++;
        for (i = 0; i < loops; i++) {
                pa = virt_to_phys(vaddr+(i*PAGE_SIZE));
                pfn = PHYS_PFN(pa);

                if (!!contiguity_check) {
                /* what's with the 'if !!(<cond>) ...' ??
                 * a 'C' trick: ensures that the if condition always evaluates
                 * to a boolean - either 0 or 1
                 */
                        if (i && pfn != prev_pfn + 1) {
                                pr_notice(" *** physical NON-contiguity detected (i=%d) ***\n", i);
                                break;
                        }
                }

                /* Below we show the actual virt addr and not a hashed value by
                 * using the 0x%[ll]x format specifier instead of the %pK as we
                 * should for security */
                /* if(!(i%100)) */
                pr_info("%05d  0x%px   %pa   %ld\n",
                        i, vaddr+(i*PAGE_SIZE), &pa, pfn);
                if (!!contiguity_check)
                        prev_pfn = pfn;
        }
}

static int __init page_exact_loop_init(void)
{
	int i, j;

	pr_info("%s: inserted\n", OURMODNAME);

	for (i = 0; i < MAXTIMES; i++) {
		gptr[i] = alloc_pages_exact(gsz, GFP_KERNEL);
		if (!gptr[i]) {
			pr_warn("%s: alloc_pages_exact() failed! (loop index %d)\n",
				OURMODNAME, i);
			/* it failed; don't leak, ensure we free the memory taken so far! */
			for (j = i; j >= 0; j--)
				free_pages_exact(gptr[j], gsz);
			return -ENOMEM;
		}
		pr_info("%s:%d: alloc_pages_exact() alloc'ed %zu bytes memory (%zu pages + rem %lu bytes)"
			" from the BSA @ %pK (actual=%px)\n",
			OURMODNAME, i, gsz, gsz / PAGE_SIZE, gsz % PAGE_SIZE, gptr[i], gptr[i]);
		// lets 'poison' it..
		memset(gptr[i], 'x', gsz);

		show_phy_pages(gptr[i], gsz, 1);
		msleep(100);
	}

	return 0;		/* success */
}

static void __exit page_exact_loop_exit(void)
{
	int i;

	for (i = 0; i < MAXTIMES; i++)
		free_pages_exact(gptr[i], gsz);
	pr_info("%s: mem freed, removed\n", OURMODNAME);
}

module_init(page_exact_loop_init);
module_exit(page_exact_loop_exit);
