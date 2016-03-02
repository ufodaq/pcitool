#ifndef _PCIDRIVER_UMEM_H
#define _PCIDRIVER_UMEM_H

#include <linux/sysfs.h>

#include "ioctl.h"

/* Define an entry in the umem list (this list is per device) */
/* This list keeps references to the SG lists for each mapped userspace region */
typedef struct {
    int id;
    struct list_head list;
    unsigned int nr_pages;		/* number of pages for this user memeory area */
    struct page **pages;		/* list of pointers to the pages */
    unsigned int nents;			/* actual entries in the scatter/gatter list (NOT nents for the map function, but the result) */
    struct scatterlist *sg;		/* list of sg entries */
    struct device_attribute sysfs_attr;	/* initialized when adding the entry */
} pcidriver_umem_entry_t;

int pcidriver_umem_sgmap( pcidriver_privdata_t *privdata, umem_handle_t *umem_handle );
int pcidriver_umem_sgunmap( pcidriver_privdata_t *privdata, pcidriver_umem_entry_t *umem_entry );
int pcidriver_umem_sgget( pcidriver_privdata_t *privdata, umem_sglist_t *umem_sglist );
int pcidriver_umem_sync( pcidriver_privdata_t *privdata, umem_handle_t *umem_handle );
pcidriver_umem_entry_t *pcidriver_umem_find_entry_id( pcidriver_privdata_t *privdata, int id );

#endif /* _PCIDRIVER_UMEM_H */
