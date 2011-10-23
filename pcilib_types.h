#ifndef _PCILIB_TYPES_H
#define _PCILIB_TYPES_H

#define KMEM_REF_HW 		0x80000000	/**< Special reference to indicate hardware access */
#define KMEM_REF_COUNT		0x0FFFFFFF	/**< Mask of reference counter (mmap/munmap), couting in mmaped memory pages */

#define KMEM_MODE_REUSABLE	0x80000000	/**< Indicates reusable buffer */
#define KMEM_MODE_EXCLUSIVE	0x40000000	/**< Only a single process is allowed to mmap the buffer */
#define KMEM_MODE_PERSISTENT	0x20000000	/**< Persistent mode instructs kmem_free to preserve buffer in memory */
#define KMEM_MODE_COUNT		0x0FFFFFFF	/**< Mask of reuse counter (alloc/free) */

#define PCILIB_KMEM_TYPE_MASK	0xFFFF0000

typedef enum {
    PCILIB_KMEM_TYPE_CONSISTENT = 0x00000,
    PCILIB_KMEM_TYPE_PAGE = 0x10000,
    PCILIB_KMEM_TYPE_DMA_S2C_PAGE = 0x10001,
    PCILIB_KMEM_TYPE_DMA_C2S_PAGE = 0x10002
} pcilib_kmem_type_t;

typedef enum {
    PCILIB_KMEM_USE_STANDARD = 0,
    PCILIB_KMEM_USE_DMA_RING = 1,
    PCILIB_KMEM_USE_DMA_PAGES = 2
} pcilib_kmem_use_t;

typedef enum {
    PCILIB_KMEM_SYNC_BIDIRECTIONAL = 0,
    PCILIB_KMEM_SYNC_TODEVICE = 1,
    PCILIB_KMEM_SYNC_FROMDEVICE = 2
} pcilib_kmem_sync_direction_t;


#define PCILIB_KMEM_USE(type, subtype) (((type) << 16)|(subtype))


//pcilib_alloc_kmem_buffer(pcilib_t *ctx, size_t size, size_t alignment)


#endif /* _PCILIB_TYPES_H */
