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

#include "../pcilib/version.h"
#include "build.h"
#include "base.h"



/* Module info */
MODULE_AUTHOR("Guillermo Marcus");
MODULE_DESCRIPTION("Simple PCI Driver");
MODULE_LICENSE("GPL v2");

/*
 * This is the table of PCI devices handled by this driver by default
 * If you want to add devices dynamically to this list, do:
 *
 *   echo "vendor device" > /sys/bus/pci/drivers/pciDriver/new_id
 * where vendor and device are in hex, without leading '0x'.
 */

static const __devinitdata struct pci_device_id pcidriver_ids[] = {
    { PCI_DEVICE( PCIE_XILINX_VENDOR_ID, PCIE_ML605_DEVICE_ID ) },          // PCI-E Xilinx ML605
    { PCI_DEVICE( PCIE_XILINX_VENDOR_ID, PCIE_IPECAMERA_DEVICE_ID ) },      // PCI-E IPE Camera
    { PCI_DEVICE( PCIE_XILINX_VENDOR_ID, PCIE_KAPTURE_DEVICE_ID ) },        // PCI-E KAPTURE board for HEB
    {0,0,0,0},
};

MODULE_DEVICE_TABLE(pci, pcidriver_ids);

/* Module class */
static struct class *pcidriver_class;

#ifdef PCIDRIVER_DUMMY_DEVICE
pcidriver_privdata_t *pcidriver_dummydata = NULL;
#else /* PCIDRIVER_DUMMY_DEVICE */
static struct pci_driver pcidriver_driver;
#endif /* PCIDRIVER_DUMMY_DEVICE */

/* Hold the allocated major & minor numbers */
static dev_t pcidriver_devt;

/* Number of devices allocated */
static atomic_t pcidriver_deviceCount;

/* Private data for probed devices */
static pcidriver_privdata_t* pcidriver_privdata[MAXDEVICES];


pcidriver_privdata_t *pcidriver_get_privdata(int devid) {
    if (devid >= MAXDEVICES)
        return NULL;

    return pcidriver_privdata[devid];
}

void pcidriver_put_privdata(pcidriver_privdata_t *privdata) {

}

/**
 * This function is called when installing the driver for a device
 * @param pdev Pointer to the PCI device
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
    privdata->class_dev = device_create(pcidriver_class, NULL, devno, privdata, NODENAMEFMT, MINOR(pcidriver_devt) + devid);
    dev_set_drvdata(privdata->class_dev, privdata);
    mod_info("Device /dev/%s%d added\n",NODENAME,MINOR(pcidriver_devt) + devid);

#ifndef PCIDRIVER_DUMMY_DEVICE
    /* Setup mmaped BARs into kernel space */
    if ((err = pcidriver_probe_irq(privdata)) != 0)
        goto probe_irq_probe_fail;
#endif /* ! PCIDRIVER_DUMMY_DEVICE */

    /* TODO: correct errorhandling. ewww. must remove the files in reversed order :-( */
    if (pcidriver_create_sysfs_attributes(privdata) != 0)
	goto probe_device_create_fail;

    /* Register character device */
    cdev_init(&(privdata->cdev), pcidriver_get_fops());
    privdata->cdev.owner = THIS_MODULE;
    privdata->cdev.ops = pcidriver_get_fops();
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
    pcidriver_remove_sysfs_attributes(privdata);

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
    device_destroy(pcidriver_class, privdata->devno);

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

#ifndef PCIDRIVER_DUMMY_DEVICE
static struct pci_driver pcidriver_driver = {
    .name = MODNAME,
    .id_table = pcidriver_ids,
    .probe = pcidriver_probe,
    .remove = pcidriver_remove,
};
#endif /* ! PCIDRIVER_DUMMY_DEVICE */

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

module_init(pcidriver_init);
module_exit(pcidriver_exit);
