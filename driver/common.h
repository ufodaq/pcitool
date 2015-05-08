#ifndef _PCIDRIVER_COMMON_H
#define _PCIDRIVER_COMMON_H

#include "../pcilib/kmem.h"
/*************************************************************************/
/* Private data types and structures */


/* Define an entry in the kmem list (this list is per device) */
/* This list keeps references to the allocated kernel buffers */
typedef struct {
	int id;
	enum dma_data_direction direction;
	
	struct list_head list;
	dma_addr_t dma_handle;
	unsigned long cpua;
	unsigned long size;
	unsigned long type;
	unsigned long align;

	unsigned long use;
	unsigned long item;

	spinlock_t lock;
	unsigned long mode;
	unsigned long refs;

	struct class_device_attribute sysfs_attr;	/* initialized when adding the entry */
} pcidriver_kmem_entry_t;

/* Define an entry in the umem list (this list is per device) */
/* This list keeps references to the SG lists for each mapped userspace region */
typedef struct {
	int id;
	struct list_head list;
	unsigned int nr_pages;		/* number of pages for this user memeory area */
	struct page **pages;		/* list of pointers to the pages */
	unsigned int nents;			/* actual entries in the scatter/gatter list (NOT nents for the map function, but the result) */
	struct scatterlist *sg;		/* list of sg entries */
	struct class_device_attribute sysfs_attr;	/* initialized when adding the entry */
} pcidriver_umem_entry_t;

/* Hold the driver private data */
typedef struct  {
	dev_t devno;						/* device number (major and minor) */
	struct pci_dev *pdev;				/* PCI device */
	struct class_device *class_dev;		/* Class device */
	struct cdev cdev;					/* char device struct */
	int mmap_mode;						/* current mmap mode */
	int mmap_area;						/* current PCI mmap area */

#ifdef ENABLE_IRQ
	int irq_enabled;					/* Non-zero if IRQ is enabled */
	int irq_count;						/* Just an IRQ counter */

	wait_queue_head_t irq_queues[ PCIDRIVER_INT_MAXSOURCES ];
										/* One queue per interrupt source */
	atomic_t irq_outstanding[ PCIDRIVER_INT_MAXSOURCES ];
										/* Outstanding interrupts per queue */
	volatile unsigned int *bars_kmapped[6];		/* PCI BARs mmapped in kernel space */

#endif
	
	spinlock_t kmemlist_lock;			/* Spinlock to lock kmem list operations */
	struct list_head kmem_list;			/* List of 'kmem_list_entry's associated with this device */
	pcidriver_kmem_entry_t *kmem_last_sync;		/* Last accessed kmem entry */
	atomic_t kmem_count;				/* id for next kmem entry */

	int kmem_cur_id;				/* Currently selected kmem buffer, for mmap */
	
	spinlock_t umemlist_lock;			/* Spinlock to lock umem list operations */
	struct list_head umem_list;			/* List of 'umem_list_entry's associated with this device */
	atomic_t umem_count;				/* id for next umem entry */

	int msi_mode;					/* Flag specifying if interrupt have been initialized in MSI mode */
	atomic_t refs;					/* Reference counter */
} pcidriver_privdata_t;


void pcidriver_module_get(pcidriver_privdata_t *privdata);
void pcidriver_module_put(pcidriver_privdata_t *privdata);

/*************************************************************************/
/* Some nice defines that make code more readable */
/* This is to print nice info in the log */

#ifdef DEBUG
 #define mod_info( args... ) \
    do { printk( KERN_INFO "%s - %s : ", MODNAME , __FUNCTION__ );\
    printk( args ); } while(0)
 #define mod_info_dbg( args... ) \
    do { printk( KERN_INFO "%s - %s : ", MODNAME , __FUNCTION__ );\
    printk( args ); } while(0)
#else
 #define mod_info( args... ) \
    do { printk( KERN_INFO "%s: ", MODNAME );\
    printk( args ); } while(0)
 #define mod_info_dbg( args... ) 
#endif

#define mod_crit( args... ) \
    do { printk( KERN_CRIT "%s: ", MODNAME );\
    printk( args ); } while(0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif
