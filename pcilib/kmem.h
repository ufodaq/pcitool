#ifndef _PCILIB_KMEM_H
#define _PCILIB_KMEM_H

typedef struct pcilib_s pcilib_t;
typedef struct pcilib_kmem_list_s pcilib_kmem_list_t;

#define PCILIB_KMEM_PAGE_SIZE	0x1000

typedef enum {
    PCILIB_TRISTATE_NO = 0,
    PCILIB_TRISTATE_PARTIAL = 1,
    PCILIB_TRISTATE_YES = 2
} pcilib_tristate_t;

#define PCILIB_KMEM_TYPE_MASK	0xFFFF0000
#define PCILIB_KMEM_USE(type, subtype) ((pcilib_kmem_use_t)(((type) << 16)|(subtype)))
#define PCILIB_KMEM_USE_TYPE(use) (use >> 16)
#define PCILIB_KMEM_USE_SUBTYPE(use) (use & 0xFFFF)

typedef enum {
    PCILIB_KMEM_TYPE_CONSISTENT = 0x00000,
    PCILIB_KMEM_TYPE_PAGE = 0x10000,
    PCILIB_KMEM_TYPE_DMA_S2C_PAGE = 0x10001,
    PCILIB_KMEM_TYPE_DMA_C2S_PAGE = 0x10002,
    PCILIB_KMEM_TYPE_REGION = 0x20000,
    PCILIB_KMEM_TYPE_REGION_S2C = 0x20001,
    PCILIB_KMEM_TYPE_REGION_C2S = 0x20002
} pcilib_kmem_type_t;

typedef enum {
    PCILIB_KMEM_USE_STANDARD = 0,
    PCILIB_KMEM_USE_DMA_RING = 1,
    PCILIB_KMEM_USE_DMA_PAGES = 2,
    PCILIB_KMEM_USE_SOFTWARE_REGISTERS = 3,
    PCILIB_KMEM_USE_LOCKS = 4,
    PCILIB_KMEM_USE_USER = 0x10
} pcilib_kmem_use_t;

typedef enum {
    PCILIB_KMEM_SYNC_BIDIRECTIONAL = 0,
    PCILIB_KMEM_SYNC_TODEVICE = 1,
    PCILIB_KMEM_SYNC_FROMDEVICE = 2
} pcilib_kmem_sync_direction_t;

typedef enum {
    PCILIB_KMEM_FLAG_REUSE = 1,				/**< Try to reuse existing buffer with the same use & item */
    PCILIB_KMEM_FLAG_EXCLUSIVE = 2,			/**< Allow only a single application accessing a specified use & item */
    PCILIB_KMEM_FLAG_PERSISTENT = 4,			/**< Sets persistent mode */
    PCILIB_KMEM_FLAG_HARDWARE = 8,			/**< The buffer may be accessed by hardware, the hardware access will not occur any more if passed to _free function */
    PCILIB_KMEM_FLAG_FORCE = 16,			/**< Force memory cleanup even if references are present */
    PCILIB_KMEM_FLAG_MASS = 32,				/**< Apply to all buffers of selected use */
    PCILIB_KMEM_FLAG_TRY =  64				/**< Do not allocate buffers, try to reuse and fail if not possible */
} pcilib_kmem_flags_t;


typedef enum {
    PCILIB_KMEM_REUSE_ALLOCATED = PCILIB_TRISTATE_NO,
    PCILIB_KMEM_REUSE_REUSED = PCILIB_TRISTATE_YES,
    PCILIB_KMEM_REUSE_PARTIAL = PCILIB_TRISTATE_PARTIAL,
    PCILIB_KMEM_REUSE_PERSISTENT = 0x100,
    PCILIB_KMEM_REUSE_HARDWARE = 0x200
} pcilib_kmem_reuse_state_t;


typedef struct {
    int handle_id;
    pcilib_kmem_reuse_state_t reused;
    
    uintptr_t pa;
//    uintptr_t va;
    void *ua;
    size_t size;
    
    size_t alignment_offset;
    size_t mmap_offset;
} pcilib_kmem_addr_t;

/**
 * single allocation - we set only addr, n_blocks = 0
 * multiple allocation - addr is not set, blocks are set, n_blocks > 0
 * sgmap allocation - addr contains ua, but pa's are set in blocks, n_blocks > 0
 */
typedef struct {
    pcilib_kmem_addr_t addr;
    
    pcilib_kmem_reuse_state_t reused;

    size_t n_blocks;
    pcilib_kmem_addr_t blocks[];
} pcilib_kmem_buffer_t;

typedef void pcilib_kmem_handle_t;


struct pcilib_kmem_list_s {
    pcilib_kmem_list_t *next, *prev;

    pcilib_kmem_buffer_t buf;	// variable size, should be last item in struct
};

#ifdef __cplusplus
extern "C" {
#endif

pcilib_kmem_handle_t *pcilib_alloc_kernel_memory(pcilib_t *ctx, pcilib_kmem_type_t type, size_t nmemb, size_t size, size_t alignment, pcilib_kmem_use_t use, pcilib_kmem_flags_t flags);
void pcilib_free_kernel_memory(pcilib_t *ctx, pcilib_kmem_handle_t *k, pcilib_kmem_flags_t flags);
//int pcilib_kmem_sync(pcilib_t *ctx, pcilib_kmem_handle_t *k, pcilib_kmem_sync_direction_t dir);
int pcilib_kmem_sync_block(pcilib_t *ctx, pcilib_kmem_handle_t *k, pcilib_kmem_sync_direction_t dir, size_t block);
void *pcilib_kmem_get_ua(pcilib_t *ctx, pcilib_kmem_handle_t *k);
uintptr_t pcilib_kmem_get_pa(pcilib_t *ctx, pcilib_kmem_handle_t *k);
uintptr_t pcilib_kmem_get_ba(pcilib_t *ctx, pcilib_kmem_handle_t *k);
void *pcilib_kmem_get_block_ua(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);
uintptr_t pcilib_kmem_get_block_pa(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);
uintptr_t pcilib_kmem_get_block_ba(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);
size_t pcilib_kmem_get_block_size(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);
pcilib_kmem_reuse_state_t pcilib_kmem_is_reused(pcilib_t *ctx, pcilib_kmem_handle_t *k);

int pcilib_clean_kernel_memory(pcilib_t *ctx, pcilib_kmem_use_t use, pcilib_kmem_flags_t flags);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_KMEM_H */
