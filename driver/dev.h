#ifndef _PCIDRIVER_DEV_H
#define _PCIDRIVER_DEV_H

typedef struct pcidriver_privdata_s pcidriver_privdata_t;

#include "kmem.h"
#include "umem.h"


/* Hold the driver private data */
struct pcidriver_privdata_s {
    int devid;                                  /* the device id */
    dev_t devno;				/* device number (major and minor) */
    struct pci_dev *pdev;			/* PCI device */
    struct device *class_dev;                 	/* Class device */
    struct cdev cdev;				/* char device struct */
    int mmap_mode;				/* current mmap mode */
    int mmap_area;				/* current PCI mmap area */

#ifdef ENABLE_IRQ
    int irq_enabled;				/* Non-zero if IRQ is enabled */
    int irq_count;				/* Just an IRQ counter */

    wait_queue_head_t irq_queues[ PCIDRIVER_INT_MAXSOURCES ];       /* One queue per interrupt source */
    atomic_t irq_outstanding[ PCIDRIVER_INT_MAXSOURCES ];           /* Outstanding interrupts per queue */
    volatile unsigned int *bars_kmapped[6];		            /* PCI BARs mmapped in kernel space */
#endif

    spinlock_t kmemlist_lock;			/* Spinlock to lock kmem list operations */
    struct list_head kmem_list;			/* List of 'kmem_list_entry's associated with this device */
    pcidriver_kmem_entry_t *kmem_last_sync;	/* Last accessed kmem entry */
    atomic_t kmem_count;			/* id for next kmem entry */

    int kmem_cur_id;				/* Currently selected kmem buffer, for mmap */

    spinlock_t umemlist_lock;			/* Spinlock to lock umem list operations */
    struct list_head umem_list;			/* List of 'umem_list_entry's associated with this device */
    atomic_t umem_count;			/* id for next umem entry */

    int msi_mode;				/* Flag specifying if interrupt have been initialized in MSI mode */
    atomic_t refs;				/* Reference counter */
};

const struct file_operations *pcidriver_get_fops(void);

void pcidriver_module_get(pcidriver_privdata_t *privdata);
void pcidriver_module_put(pcidriver_privdata_t *privdata);

long pcidriver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

#endif /* _PCIDRIVER_DEV_H */

