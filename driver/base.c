/**
 *
 * @file base.c
 * @author Guillermo Marcus
 * @date 2009-04-05
 * @brief Contains the main code which connects all the different parts and does
 * basic driver tasks like initialization.
 */


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
#include <asm/scatterlist.h>
#include <linux/vmalloc.h>
#include <linux/stat.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include "../pcilib/version.h"

#include "config.h"
#include "compat.h"
#include "pciDriver.h"
#include "common.h"
#include "base.h"
#include "int.h"
#include "kmem.h"
#include "umem.h"
#include "ioctl.h"
#include "build.h"

/*************************************************************************/
/* Module device table associated with this driver */
MODULE_DEVICE_TABLE(pci, pcidriver_ids);

/* Module init and exit points */
module_init(pcidriver_init);
module_exit(pcidriver_exit);

/* Module info */
MODULE_AUTHOR("Guillermo Marcus");
MODULE_DESCRIPTION("Simple PCI Driver");
MODULE_LICENSE("GPL v2");

/* Module class */
static struct class_compat *pcidriver_class;

#ifdef PCIDRIVER_DUMMY_DEVICE
pcidriver_privdata_t *pcidriver_dummydata = NULL;
#endif /* PCIDRIVER_DUMMY_DEVICE */

/**
 *
 * Called when loading the driver
 *
 */
static int __init pcidriver_init(void)
{
    int err = 0;

    /* Initialize the device count */
    atomic_set(&pcidriver_deviceCount, 0);

    memset(pcidriver_privdata, 0, sizeof(pcidriver_privdata));

    /* Allocate character device region dynamically */
    if ((err = alloc_chrdev_region(&pcidriver_devt, MINORNR, MAXDEVICES, NODENAME)) != 0) {
        mod_info("Couldn't allocate chrdev region. Module not loaded.\n");
        goto init_alloc_fail;
    }
    mod_info("Major %d allocated to nodename '%s'\n", MAJOR(pcidriver_devt), NODENAME);

    /* Register driver class */
    pcidriver_class = class_create(THIS_MODULE, NODENAME);

    if (IS_ERR(pcidriver_class)) {
        mod_info("No sysfs support. Module not loaded.\n");
        goto init_class_fail;
    }

    /* Register PCI driver. This function returns the number of devices on some
     * systems, therefore check for errors as < 0. */
#ifdef PCIDRIVER_DUMMY_DEVICE
    if ((err = pcidriver_probe(NULL, NULL)) < 0) {
#else /* PCIDRIVER_DUMMY_DEVICE */
    if ((err = pci_register_driver(&pcidriver_driver)) < 0) {
#endif /* PCIDRIVER_DUMMY_DEVICE */
        mod_info("Couldn't register PCI driver. Module not loaded.\n");
        goto init_pcireg_fail;
    }

    mod_info("pcidriver %u.%u.%u loaded\n", PCILIB_VERSION_GET_MAJOR(PCILIB_VERSION), PCILIB_VERSION_GET_MINOR(PCILIB_VERSION), PCILIB_VERSION_GET_MICRO(PCILIB_VERSION));
    mod_info("%s\n", PCIDRIVER_BUILD);
    mod_info("%s\n", PCIDRIVER_REVISION);
    if (strlen(PCIDRIVER_CHANGES)) {
        mod_info("Extra changes - %s\n", PCIDRIVER_CHANGES);
    }

    return 0;

init_pcireg_fail:
    class_destroy(pcidriver_class);
init_class_fail:
    unregister_chrdev_region(pcidriver_devt, MAXDEVICES);
init_alloc_fail:
    return err;
}

/**
 *
 * Called when unloading the driver
 *
 */
static void pcidriver_exit(void)
{
#ifdef PCIDRIVER_DUMMY_DEVICE
    pcidriver_remove(NULL);
#else
    pci_unregister_driver(&pcidriver_driver);
#endif /* PCIDRIVER_DUMMY_DEVICE */

    unregister_chrdev_region(pcidriver_devt, MAXDEVICES);

    if (pcidriver_class != NULL)
        class_destroy(pcidriver_class);

    mod_info("Module unloaded\n");
}

/*************************************************************************/
/* Driver functions */

/**
 *
 * This struct defines the PCI entry points.
 * Will be registered at module init.
 *
 */
#ifndef PCIDRIVER_DUMMY_DEVICE
static struct pci_driver pcidriver_driver = {
    .name = MODNAME,
    .id_table = pcidriver_ids,
    .probe = pcidriver_probe,
    .remove = pcidriver_remove,
};
#endif /* ! PCIDRIVER_DUMMY_DEVICE */

/**
 *
 * This function is called when installing the driver for a device
 * @param pdev Pointer to the PCI device
 *
 */
static int __devinit pcidriver_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int err = 0;
    int devno;
    pcidriver_privdata_t *privdata;
    int devid;

    /* At the moment there is no difference between these boards here, other than
     * printing a different message in the log.
     *
     * However, there is some difference in the interrupt handling functions.
     */
#ifdef PCIDRIVER_DUMMY_DEVICE
    mod_info("Emulated device\n");
#else /* PCIDRIVER_DUMMY_DEVICE */
    if (id->vendor == PCIE_XILINX_VENDOR_ID) {
        if (id->device == PCIE_ML605_DEVICE_ID) {
            mod_info("Found ML605 board at %s\n", dev_name(&pdev->dev));
        } else if (id->device == PCIE_IPECAMERA_DEVICE_ID) {
            mod_info("Found IPE Camera at %s\n", dev_name(&pdev->dev));
        } else if (id->device == PCIE_KAPTURE_DEVICE_ID) {
            mod_info("Found KAPTURE board at %s\n", dev_name(&pdev->dev));
        } else {
            mod_info("Found unknown Xilinx device (%x) at %s\n", id->device, dev_name(&pdev->dev));
        }
    } else {
        /* It is something else */
        mod_info("Found unknown board (%x:%x) at %s\n", id->vendor, id->device, dev_name(&pdev->dev));
    }

    /* Enable the device */
    if ((err = pci_enable_device(pdev)) != 0) {
        mod_info("Couldn't enable device\n");
        goto probe_pcien_fail;
    }

    /* Bus master & dma */
    pci_set_master(pdev);

    err = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
    if (err < 0) {
        printk(KERN_ERR "pci_set_dma_mask failed\n");
        goto probe_dma_fail;
    }

    /* Set Memory-Write-Invalidate support */
    if ((err = pci_set_mwi(pdev)) != 0)
        mod_info("MWI not supported. Continue without enabling MWI.\n");
#endif /* PCIDRIVER_DUMMY_DEVICE */

    /* Get / Increment the device id */
    devid = atomic_inc_return(&pcidriver_deviceCount) - 1;
    if (devid >= MAXDEVICES) {
        mod_info("Maximum number of devices reached! Increase MAXDEVICES.\n");
        err = -ENOMSG;
        goto probe_maxdevices_fail;
    }

    /* Allocate and initialize the private data for this device */
    if ((privdata = kcalloc(1, sizeof(*privdata), GFP_KERNEL)) == NULL) {
        err = -ENOMEM;
        goto probe_nomem;
    }

    privdata->devid = devid;

    INIT_LIST_HEAD(&(privdata->kmem_list));
    spin_lock_init(&(privdata->kmemlist_lock));
    atomic_set(&privdata->kmem_count, 0);

    INIT_LIST_HEAD(&(privdata->umem_list));
    spin_lock_init(&(privdata->umemlist_lock));
    atomic_set(&privdata->umem_count, 0);

#ifdef PCIDRIVER_DUMMY_DEVICE
    pcidriver_dummydata = privdata;
#else /* PCIDRIVER_DUMMY_DEVICE */
    pci_set_drvdata(pdev, privdata);
    privdata->pdev = pdev;
#endif /* PCIDRIVER_DUMMY_DEVICE */

    /* Device add to sysfs */
    devno = MKDEV(MAJOR(pcidriver_devt), MINOR(pcidriver_devt) + devid);
    privdata->devno = devno;

    /* FIXME: some error checking missing here */
#ifdef PCIDRIVER_DUMMY_DEVICE
    privdata->class_dev = class_device_create(pcidriver_class, NULL, devno, NULL, NODENAMEFMT, MINOR(pcidriver_devt) + devid, privdata);
#else /* PCIDRIVER_DUMMY_DEVICE */
    privdata->class_dev = class_device_create(pcidriver_class, NULL, devno, &(pdev->dev), NODENAMEFMT, MINOR(pcidriver_devt) + devid, privdata);
#endif /* PCIDRIVER_DUMMY_DEVICE */
    class_set_devdata( privdata->class_dev, privdata );
    mod_info("Device /dev/%s%d added\n",NODENAME,MINOR(pcidriver_devt) + devid);

#ifndef PCIDRIVER_DUMMY_DEVICE
    /* Setup mmaped BARs into kernel space */
    if ((err = pcidriver_probe_irq(privdata)) != 0)
        goto probe_irq_probe_fail;
#endif /* ! PCIDRIVER_DUMMY_DEVICE */

    /* Populate sysfs attributes for the class device */
    /* TODO: correct errorhandling. ewww. must remove the files in reversed order :-( */
#define sysfs_attr(name) do { \
			if (class_device_create_file(sysfs_attr_def_pointer, &sysfs_attr_def_name(name)) != 0) \
				goto probe_device_create_fail; \
			} while (0)
#ifdef ENABLE_IRQ
    sysfs_attr(irq_count);
    sysfs_attr(irq_queues);
#endif

    sysfs_attr(mmap_mode);
    sysfs_attr(mmap_area);
    sysfs_attr(kmem_count);
    sysfs_attr(kmem_alloc);
    sysfs_attr(kmem_free);
    sysfs_attr(kbuffers);
    sysfs_attr(umappings);
    sysfs_attr(umem_unmap);
#undef sysfs_attr

    /* Register character device */
    cdev_init( &(privdata->cdev), &pcidriver_fops );
    privdata->cdev.owner = THIS_MODULE;
    privdata->cdev.ops = &pcidriver_fops;
    err = cdev_add( &privdata->cdev, devno, 1 );
    if (err) {
        mod_info( "Couldn't add character device.\n" );
        goto probe_cdevadd_fail;
    }

    pcidriver_privdata[devid] = privdata;

    return 0;

probe_device_create_fail:
probe_cdevadd_fail:
#ifndef PCIDRIVER_DUMMY_DEVICE
probe_irq_probe_fail:
    pcidriver_irq_unmap_bars(privdata);
#endif /* ! PCIDRIVER_DUMMY_DEVICE */
    kfree(privdata);
probe_nomem:
    atomic_dec(&pcidriver_deviceCount);
probe_maxdevices_fail:
#ifndef PCIDRIVER_DUMMY_DEVICE
probe_dma_fail:
    pci_disable_device(pdev);
probe_pcien_fail:
#endif /* ! PCIDRIVER_DUMMY_DEVICE */
    return err;
}

/**
 *
 * This function is called when disconnecting a device
 *
 */
static void __devexit pcidriver_remove(struct pci_dev *pdev)
{
    pcidriver_privdata_t *privdata;

#ifdef PCIDRIVER_DUMMY_DEVICE
    privdata = pcidriver_dummydata;
    pcidriver_dummydata = NULL;
#else /* PCIDRIVER_DUMMY_DEVICE */
    /* Get private data from the device */
    privdata = pci_get_drvdata(pdev);
#endif /* PCIDRIVER_DUMMY_DEVICE */

    // Theoretically we should lock here and when using...
    pcidriver_privdata[privdata->devid] = NULL;

    /* Removing sysfs attributes from class device */
#define sysfs_attr(name) do { \
			class_device_remove_file(sysfs_attr_def_pointer, &sysfs_attr_def_name(name)); \
			} while (0)
#ifdef ENABLE_IRQ
    sysfs_attr(irq_count);
    sysfs_attr(irq_queues);
#endif

    sysfs_attr(mmap_mode);
    sysfs_attr(mmap_area);
    sysfs_attr(kmem_count);
    sysfs_attr(kmem_alloc);
    sysfs_attr(kmem_free);
    sysfs_attr(kbuffers);
    sysfs_attr(umappings);
    sysfs_attr(umem_unmap);
#undef sysfs_attr

    /* Free all allocated kmem buffers before leaving */
    pcidriver_kmem_free_all( privdata );

#ifndef PCIDRIVER_DUMMY_DEVICE
# ifdef ENABLE_IRQ
    pcidriver_remove_irq(privdata);
# endif
#endif /* ! PCIDRIVER_DUMMY_DEVICE */

    /* Removing Character device */
    cdev_del(&(privdata->cdev));

    /* Removing the device from sysfs */
    class_device_destroy(pcidriver_class, privdata->devno);

    /* Releasing privdata */
    kfree(privdata);

#ifdef PCIDRIVER_DUMMY_DEVICE
    mod_info("Device at " NODENAMEFMT " removed\n", 0);
#else /* PCIDRIVER_DUMMY_DEVICE */
    /* Disabling PCI device */
    pci_disable_device(pdev);
    mod_info("Device at %s removed\n", dev_name(&pdev->dev));
#endif /* PCIDRIVER_DUMMY_DEVICE */

}

/*************************************************************************/
/* File operations */
/*************************************************************************/

/**
 * This struct defines the file operation entry points.
 *
 * @see pcidriver_ioctl
 * @see pcidriver_mmap
 * @see pcidriver_open
 * @see pcidriver_release
 *
 */
static struct file_operations pcidriver_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = pcidriver_ioctl,
    .mmap = pcidriver_mmap,
    .open = pcidriver_open,
    .release = pcidriver_release,
};

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

/**
 *
 * Called when an application open()s a /dev/fpga*, attaches the private data
 * with the file pointer.
 *
 */
int pcidriver_open(struct inode *inode, struct file *filp)
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
int pcidriver_release(struct inode *inode, struct file *filp)
{
    pcidriver_privdata_t *privdata;

    /* Get the private data area */
    privdata = filp->private_data;

    pcidriver_module_put(privdata);

    return 0;
}

/**
 *
 * This function is the entry point for mmap() and calls either pcidriver_mmap_pci
 * or pcidriver_mmap_kmem
 *
 * @see pcidriver_mmap_pci
 * @see pcidriver_mmap_kmem
 *
 */
int pcidriver_mmap(struct file *filp, struct vm_area_struct *vma)
{
    pcidriver_privdata_t *privdata;
    int ret = 0, bar;

    mod_info_dbg("Entering mmap\n");

    /* Get the private data area */
    privdata = filp->private_data;

    /* Check the current mmap mode */
    switch (privdata->mmap_mode) {
    case PCIDRIVER_MMAP_PCI:
        /* Mmap a PCI region */
        switch (privdata->mmap_area) {
        case PCIDRIVER_BAR0:
            bar = 0;
            break;
        case PCIDRIVER_BAR1:
            bar = 1;
            break;
        case PCIDRIVER_BAR2:
            bar = 2;
            break;
        case PCIDRIVER_BAR3:
            bar = 3;
            break;
        case PCIDRIVER_BAR4:
            bar = 4;
            break;
        case PCIDRIVER_BAR5:
            bar = 5;
            break;
        default:
            mod_info("Attempted to mmap a PCI area with the wrong mmap_area value: %d\n",privdata->mmap_area);
            return -EINVAL;			/* invalid parameter */
            break;
        }
        ret = pcidriver_mmap_pci(privdata, vma, bar);
        break;
    case PCIDRIVER_MMAP_KMEM:
        /* mmap a Kernel buffer */
        ret = pcidriver_mmap_kmem(privdata, vma);
        break;
    default:
        mod_info( "Invalid mmap_mode value (%d)\n",privdata->mmap_mode );
        return -EINVAL;			/* Invalid parameter (mode) */
    }

    return ret;
}

/*************************************************************************/
/* Internal driver functions */
int pcidriver_mmap_pci(pcidriver_privdata_t *privdata, struct vm_area_struct *vmap, int bar)
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
        ret = io_remap_pfn_range_compat(vmap, vmap->vm_start, bar_addr, bar_length, vmap->vm_page_prot);
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
        ret = remap_pfn_range_compat(vmap, vmap->vm_start, bar_addr, bar_length, vmap->vm_page_prot);
    }

    if (ret) {
        mod_info("remap_pfn_range failed\n");
        return -EAGAIN;
    }

    return 0;	/* success */
#endif /* PCIDRIVER_DUMMY_DEVICE */
}

pcidriver_privdata_t *pcidriver_get_privdata(int devid) {
    if (devid >= MAXDEVICES)
        return NULL;

    return pcidriver_privdata[devid];
}

void pcidriver_put_privdata(pcidriver_privdata_t *privdata) {

}
