#ifndef _FOLLOW_PTE_HELPER_H
#define _FOLLOW_PTE_HELPER_H

#include <linux/mm.h>
#include <linux/hugetlb.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
static inline int follow_pte(struct mm_struct *mm, unsigned long address,
                           pte_t **ptepp, spinlock_t **ptlp)
{
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *ptep;

    /* We don't support HugeTLB pages for now */
    if (is_vm_hugetlb_page(find_vma(mm, address)))
        return -EINVAL;

    pgd = pgd_offset(mm, address);
    if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
        return -EINVAL;

    p4d = p4d_offset(pgd, address);
    if (p4d_none(*p4d) || unlikely(p4d_bad(*p4d)))
        return -EINVAL;

    pud = pud_offset(p4d, address);
    if (pud_none(*pud) || unlikely(pud_bad(*pud)))
        return -EINVAL;

    pmd = pmd_offset(pud, address);
    if (pmd_none(*pmd) || unlikely(pmd_bad(*pmd)))
        return -EINVAL;

    ptep = pte_offset_map(pmd, address);
    if (!ptep)
        return -EINVAL;

    if (pte_none(*ptep)) {
        pte_unmap(ptep);
        return -EINVAL;
    }

    *ptepp = ptep;
    *ptlp = pte_lockptr(mm, pmd);
    spin_lock(*ptlp);

    return 0;
}
#endif

#endif /* _FOLLOW_PTE_HELPER_H */ 