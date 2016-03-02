#ifndef _PCIDRIVER_KMEM_H
#define _PCIDRIVER_KMEM_H

#include <linux/sysfs.h>

#include "../pcilib/kmem.h"
#include "ioctl.h"

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

    struct device_attribute sysfs_attr;	/* initialized when adding the entry */
} pcidriver_kmem_entry_t;


int pcidriver_kmem_alloc( pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
int pcidriver_kmem_free(  pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
int pcidriver_kmem_sync_entry( pcidriver_privdata_t *privdata, pcidriver_kmem_entry_t *kmem_entry, int direction );
int pcidriver_kmem_sync(  pcidriver_privdata_t *privdata, kmem_sync_t *kmem_sync );
int pcidriver_kmem_free_all(  pcidriver_privdata_t *privdata );
pcidriver_kmem_entry_t *pcidriver_kmem_find_entry( pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
pcidriver_kmem_entry_t *pcidriver_kmem_find_entry_id( pcidriver_privdata_t *privdata, int id );
pcidriver_kmem_entry_t *pcidriver_kmem_find_entry_use(pcidriver_privdata_t *privdata, unsigned long use, unsigned long item);
int pcidriver_kmem_free_entry( pcidriver_privdata_t *privdata, pcidriver_kmem_entry_t *kmem_entry );

int pcidriver_mmap_kmem( pcidriver_privdata_t *privdata, struct vm_area_struct *vmap );

#endif /* _PCIDRIVER_KMEM_H */
