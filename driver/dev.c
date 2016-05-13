#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sysfs.h>
#include <asm/atomic.h>
#include <linux/pagemap.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/scatterlist.h>
#include <linux/vmalloc.h>
#include <linux/stat.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include "base.h"


/**
 *
 * Called when an application open()s a /dev/fpga*, attaches the private data
 * with the file pointer.
 *
 */
static int pcidriver_open(struct inode *inode, struct file *filp)
{
    pcidriver_privdata_t *privdata;

    /* Set the private data area for the file */
    privdata = container_of( inode->i_cdev, pcidriver_privdata_t, cdev);
    filp->private_data = privdata;

    pcidriver_module_get(privdata);

    return 0;
}

/**
 *
 * Called when the application close()s the file descriptor. Does nothing at
 * the moment.
 *
 */
static int pcidriver_release(struct inode *inode, struct file *filp)
{
    pcidriver_privdata_t *privdata;

    /* Get the private data area */
    privdata = filp->private_data;

    pcidriver_module_put(privdata);

    return 0;
}


/*************************************************************************/
/* Internal driver functions */
static int pcidriver_mmap_bar(pcidriver_privdata_t *privdata, struct vm_area_struct *vmap, int bar)
{
#ifdef PCIDRIVER_DUMMY_DEVICE
    return -ENXIO;
#else /* PCIDRIVER_DUMMY_DEVICE */
    int ret = 0;
    unsigned long bar_addr;
    unsigned long bar_length, vma_size;
    unsigned long bar_flags;

    mod_info_dbg("Entering mmap_pci\n");


    /* Get info of the BAR to be mapped */
    bar_addr = pci_resource_start(privdata->pdev, bar);
    bar_length = pci_resource_len(privdata->pdev, bar);
    bar_flags = pci_resource_flags(privdata->pdev, bar);

    /* Check sizes */
    vma_size = (vmap->vm_end - vmap->vm_start);

    if ((vma_size != bar_length) &&
            ((bar_length < PAGE_SIZE) && (vma_size != PAGE_SIZE))) {
        mod_info( "mmap size is not correct! bar: %lu - vma: %lu\n", bar_length, vma_size );
        return -EINVAL;
    }

    if (bar_flags & IORESOURCE_IO) {
        /* Unlikely case, we will mmap a IO region */

        /* IO regions are never cacheable */
        vmap->vm_page_prot = pgprot_noncached(vmap->vm_page_prot);

        /* Map the BAR */
        ret = io_remap_pfn_range(vmap, vmap->vm_start, (bar_addr >> PAGE_SHIFT), bar_length, vmap->vm_page_prot);
    } else {
        /* Normal case, mmap a memory region */

        /* Ensure this VMA is non-cached, if it is not flaged as prefetchable.
         * If it is prefetchable, caching is allowed and will give better performance.
         * This should be set properly by the BIOS, but we want to be sure. */
        /* adapted from drivers/char/mem.c, mmap function. */

        /* Setting noncached disables MTRR registers, and we want to use them.
         * So we take this code out. This can lead to caching problems if and only if
         * the System BIOS set something wrong. Check LDDv3, page 425.
         */

//                if (!(bar_flags & IORESOURCE_PREFETCH))
//			vmap->vm_page_prot = pgprot_noncached(vmap->vm_page_prot);


        /* Map the BAR */
        ret = remap_pfn_range(vmap, vmap->vm_start, (bar_addr >> PAGE_SHIFT), bar_length, vmap->vm_page_prot);
    }

    if (ret) {
        mod_info("remap_pfn_range failed\n");
        return -EAGAIN;
    }

    return 0;	/* success */
#endif /* PCIDRIVER_DUMMY_DEVICE */
}


static int pcidriver_mmap_area(pcidriver_privdata_t *privdata, struct vm_area_struct *vmap)
{
    int ret = 0;

    unsigned long vma_size;
    unsigned long addr = vmap->vm_pgoff;

    mod_info_dbg("Entering mmap_addr\n");

    /* Check sizes */
    vma_size = (vmap->vm_end - vmap->vm_start);

    if (addr % PAGE_SIZE) {
	mod_info("mmap addr (0x%lx) is not aligned to page boundary\n", addr);
        return -EINVAL;
    }

    ret = remap_pfn_range(vmap, vmap->vm_start, (addr >> PAGE_SHIFT), vma_size, vmap->vm_page_prot);

    if (ret) {
        mod_info("remap_pfn_range failed\n");
        return -EAGAIN;
    }

    return 0;	/* success */
}


/**
 *
 * This function is the entry point for mmap() and calls either pcidriver_mmap_bar
 * or pcidriver_mmap_kmem
 *
 * @see pcidriver_mmap_bar
 * @see pcidriver_mmap_kmem
 *
 */
static int pcidriver_mmap(struct file *filp, struct vm_area_struct *vma)
{
    pcidriver_privdata_t *privdata;
    int ret = 0, bar;

    mod_info_dbg("Entering mmap\n");

    /* Get the private data area */
    privdata = filp->private_data;

    /* Check the current mmap mode */
    switch (privdata->mmap_mode) {
     case PCIDRIVER_MMAP_PCI:
	bar = privdata->mmap_area;
	if ((bar < 0)||(bar > 5)) {
            mod_info("Attempted to mmap a PCI area with the wrong mmap_area value: %d\n",privdata->mmap_area);
            return -EINVAL;
        }
        ret = pcidriver_mmap_bar(privdata, vma, bar);
        break;
    case PCIDRIVER_MMAP_AREA:
	ret = pcidriver_mmap_area(privdata, vma);
	break;
    case PCIDRIVER_MMAP_KMEM:
        ret = pcidriver_mmap_kmem(privdata, vma);
        break;
    default:
        mod_info( "Invalid mmap_mode value (%d)\n",privdata->mmap_mode );
        return -EINVAL;			/* Invalid parameter (mode) */
    }

    return ret;
}

static struct file_operations pcidriver_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = pcidriver_ioctl,
    .mmap = pcidriver_mmap,
    .open = pcidriver_open,
    .release = pcidriver_release,
};

const struct file_operations *pcidriver_get_fops(void)
{
    return &pcidriver_fops;
}


void pcidriver_module_get(pcidriver_privdata_t *privdata) {
    atomic_inc(&(privdata->refs));
//    mod_info("Ref: %i\n", atomic_read(&(privdata->refs)));
}

void pcidriver_module_put(pcidriver_privdata_t *privdata) {
    if (atomic_add_negative(-1, &(privdata->refs))) {
        atomic_inc(&(privdata->refs));
        mod_info("Reference counting error...");
    } else {
//	mod_info("Unref: %i\n", atomic_read(&(privdata->refs)));
    }
}
