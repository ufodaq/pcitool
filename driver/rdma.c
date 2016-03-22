#include <linux/version.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/hugetlb.h>
#include <linux/cdev.h>

#include "base.h"

static unsigned long pcidriver_follow_pte(struct mm_struct *mm, unsigned long address)
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    spinlock_t *ptl;
    unsigned long pfn = 0;


    pgd = pgd_offset(mm, address);
    if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
        return 0;

    pud = pud_offset(pgd, address);
    if (pud_none(*pud) || unlikely(pud_bad(*pud)))
        return 0;

    pmd = pmd_offset(pud, address);
    if (pmd_none(*pmd))
        return 0;

    pte = pte_offset_map_lock(mm, pmd, address, &ptl);
    if (!pte_none(*pte))
        pfn = (pte_pfn(*pte) << PAGE_SHIFT);
    pte_unmap_unlock(pte, ptl);

    return pfn;
}

unsigned long pcidriver_resolve_bar(unsigned long bar_address) {
    int dev, bar;
    unsigned long pfn;
    unsigned long address;

    address = (bar_address >> PAGE_SHIFT) << PAGE_SHIFT;
    offset = bar_address - address;
    pfn = pcidriver_follow_pte(current->mm, address);

    for (dev = 0; dev < MAXDEVICES; dev++)
    {
        pcidriver_privdata_t *privdata =  pcidriver_get_privdata(dev);
        if (!privdata) continue;

        for (bar = 0; bar < 6; bar++)
        {
            unsigned long start = pci_resource_start(privdata->pdev, bar);
            unsigned long end = start + pci_resource_len(privdata->pdev, bar);
            if ((pfn >= start)&&(pfn < end))
                return pfn + offset;
        }
        pcidriver_put_privdata(privdata);
    }

    return 0;
}

EXPORT_SYMBOL(pcidriver_resolve_bar);
