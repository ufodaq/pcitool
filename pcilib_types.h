#ifndef _PCILIB_TYPES_H
#define _PCILIB_TYPES_H

typedef enum {
    PCILIB_KMEM_TYPE_CONSISTENT = 0,
    PCILIB_KMEM_TYPE_PAGE,
} pcilib_kmem_type_t;

typedef enum {
    PCILIB_KMEM_USE_DMA = 1,
} pcilib_kmem_use_t;

typedef enum {
    PCILIB_KMEM_SYNC_TODEVICE = 1,
    PCILIB_KMEM_SYNC_FROMDEVICE = 2
} pcilib_kmem_sync_direction_t;


#define PCILIB_KMEM_USE(type, subtype) (((type) << 16)|(subtype))


//pcilib_alloc_kmem_buffer(pcilib_t *ctx, size_t size, size_t alignment)


#endif /* _PCILIB_TYPES_H */
