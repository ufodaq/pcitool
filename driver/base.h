#ifndef _PCIDRIVER_BASE_H
#define _PCIDRIVER_BASE_H

#include "sysfs.h"

/**
 *
 * This file contains prototypes and data structures for internal use of the pciDriver.
 *
 *
 */

/* prototypes for file_operations */
static struct file_operations pcidriver_fops;
int pcidriver_mmap( struct file *filp, struct vm_area_struct *vmap );
int pcidriver_open(struct inode *inode, struct file *filp );
int pcidriver_release(struct inode *inode, struct file *filp);

/* prototypes for device operations */
#ifndef PCIDRIVER_DUMMY_DEVICE
static struct pci_driver pcidriver_driver;
#endif /* ! PCIDRIVER_DUMMY_DEVICE */
static int __devinit pcidriver_probe(struct pci_dev *pdev, const struct pci_device_id *id);
static void __devexit pcidriver_remove(struct pci_dev *pdev);



/* prototypes for module operations */
static int __init pcidriver_init(void);
static void pcidriver_exit(void);

/*
 * This is the table of PCI devices handled by this driver by default
 * If you want to add devices dynamically to this list, do:
 *
 *   echo "vendor device" > /sys/bus/pci/drivers/pciDriver/new_id
 * where vendor and device are in hex, without leading '0x'.
 *
 * The IDs themselves can be found in common.h
 *
 * For more info, see <kernel-source>/Documentation/pci.txt
 *
 * __devinitdata is applied because the kernel does not need those
 * tables any more after boot is finished on systems which don't
 * support hotplug.
 *
 */

static const __devinitdata struct pci_device_id pcidriver_ids[] = {
	{ PCI_DEVICE( PCIE_XILINX_VENDOR_ID, PCIE_ML605_DEVICE_ID ) },          // PCI-E Xilinx ML605
	{ PCI_DEVICE( PCIE_XILINX_VENDOR_ID, PCIE_IPECAMERA_DEVICE_ID ) },      // PCI-E IPE Camera
	{ PCI_DEVICE( PCIE_XILINX_VENDOR_ID, PCIE_KAPTURE_DEVICE_ID ) },        // PCI-E KAPTURE board for HEB
	{0,0,0,0},
};

/* prototypes for internal driver functions */
int pcidriver_pci_read( pcidriver_privdata_t *privdata, pci_cfg_cmd *pci_cmd );
int pcidriver_pci_write( pcidriver_privdata_t *privdata, pci_cfg_cmd *pci_cmd );
int pcidriver_pci_info( pcidriver_privdata_t *privdata, pcilib_board_info_t *pci_info );

int pcidriver_mmap_pci( pcidriver_privdata_t *privdata, struct vm_area_struct *vmap , int bar );
int pcidriver_mmap_kmem( pcidriver_privdata_t *privdata, struct vm_area_struct *vmap );

/*************************************************************************/
/* Static data */
/* Hold the allocated major & minor numbers */
static dev_t pcidriver_devt;

/* Number of devices allocated */
static atomic_t pcidriver_deviceCount;

/* Sysfs attributes */
static DEVICE_ATTR(mmap_mode, 0664, pcidriver_show_mmap_mode, pcidriver_store_mmap_mode);
static DEVICE_ATTR(mmap_area, 0664, pcidriver_show_mmap_area, pcidriver_store_mmap_area);
static DEVICE_ATTR(kmem_count, S_IRUGO, pcidriver_show_kmem_count, NULL);
static DEVICE_ATTR(kbuffers, S_IRUGO, pcidriver_show_kbuffers, NULL);
static DEVICE_ATTR(kmem_alloc, 0220, NULL, pcidriver_store_kmem_alloc);
static DEVICE_ATTR(kmem_free, 0220, NULL, pcidriver_store_kmem_free);
static DEVICE_ATTR(umappings, S_IRUGO, pcidriver_show_umappings, NULL);
static DEVICE_ATTR(umem_unmap, 0220, NULL, pcidriver_store_umem_unmap);

#ifdef ENABLE_IRQ
static DEVICE_ATTR(irq_count, S_IRUGO, pcidriver_show_irq_count, NULL);
static DEVICE_ATTR(irq_queues, S_IRUGO, pcidriver_show_irq_queues, NULL);
#endif

#endif
