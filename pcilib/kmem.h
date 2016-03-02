#ifndef _PCILIB_KMEM_H
#define _PCILIB_KMEM_H

typedef struct pcilib_s pcilib_t;
typedef struct pcilib_kmem_list_s pcilib_kmem_list_t;

#define PCILIB_KMEM_PAGE_SIZE	0x1000			/**< Default pages size is 4096 bytes */

typedef enum {
    PCILIB_TRISTATE_NO = 0,				/**< All values are evaluated as false */
    PCILIB_TRISTATE_PARTIAL = 1,			/**< Some values are evaluated as false and others as true */
    PCILIB_TRISTATE_YES = 2				/**< All values are evaluated as true */
} pcilib_tristate_t;

#define PCILIB_KMEM_TYPE_MASK	0xFFFF0000		/**< Mask selecting the general type in \ref pcilib_kmem_type_t. The subtype is may specify DMA mapping and other minor details of allocation */
#define PCILIB_KMEM_USE_TYPE(use) (use >> 16)		/**< Returns general use of the kernel buffers, for instance DMA buffer (see \ref pcilib_kmem_use_t) */
#define PCILIB_KMEM_USE_SUBTYPE(use) (use & 0xFFFF)	/**< Additional information about the use of the kernel buffers. The actual meaning depends on general use, for instance in case of DMA buffers the subtype specifies DMA engine */
#define PCILIB_KMEM_USE(type, subtype) ((pcilib_kmem_use_t)(((type) << 16)|(subtype)))		/**< Constructs full use from use type and use subtype */

typedef enum {
    PCILIB_KMEM_TYPE_CONSISTENT = 0x00000,		/**< Consistent memory can be simultaneously accessed by the device and applications and is normally used for DMA descriptors */
    PCILIB_KMEM_TYPE_PAGE = 0x10000,			/**< A number of physically consequitive pages of memory */
    PCILIB_KMEM_TYPE_DMA_S2C_PAGE = 0x10001,		/**< Memory pages mapped for S2C DMA operation (device reads) */
    PCILIB_KMEM_TYPE_DMA_C2S_PAGE = 0x10002,		/**< Memory pages mapped for C2S DMA operation (device writes) */
    PCILIB_KMEM_TYPE_REGION = 0x20000,			/**< Just map buffers to the contiguous user-supplied memory region (normally reserved during the boot with option memmap=512M$2G) */
    PCILIB_KMEM_TYPE_REGION_S2C = 0x20001,		/**< Memory region mapped for S2C DMA operation (device reads) */
    PCILIB_KMEM_TYPE_REGION_C2S = 0x20002		/**< Memory region mapped for C2S DMA operation (device writes */
} pcilib_kmem_type_t;

typedef enum {
    PCILIB_KMEM_USE_STANDARD = 0,			/**< Undefined use, buffers of standard use are not grouped together and can be used individually */
    PCILIB_KMEM_USE_DMA_RING = 1,			/**< The kmem used for DMA descriptor, the sub type specifies the id of considered DMA engine */
    PCILIB_KMEM_USE_DMA_PAGES = 2,			/**< The kmem used for DMA buffers, the sub type specifies the address of considered DMA engine (can be engine specific) */
    PCILIB_KMEM_USE_SOFTWARE_REGISTERS = 3,		/**< The kmem used to store software registers, the sub type holds the bank address */
    PCILIB_KMEM_USE_LOCKS = 4,				/**< The kmem used to hold locks, the sub type is not used */
    PCILIB_KMEM_USE_USER = 0x10				/**< Further uses can be defined by event engines */
} pcilib_kmem_use_t;

typedef enum {
    PCILIB_KMEM_SYNC_BIDIRECTIONAL = 0,			/**< Two-way synchronization (should not be used) */
    PCILIB_KMEM_SYNC_TODEVICE = 1,			/**< Allow device to read the data in the DMA-able buffer */
    PCILIB_KMEM_SYNC_FROMDEVICE = 2			/**< Allow application to read the data written by device in the DMA-able buffer */
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
    PCILIB_KMEM_REUSE_ALLOCATED = PCILIB_TRISTATE_NO,	/**< The kernel memory was allocated anew */
    PCILIB_KMEM_REUSE_REUSED = PCILIB_TRISTATE_YES,	/**< Already allocated kernel memory was re-used */
    PCILIB_KMEM_REUSE_PARTIAL = PCILIB_TRISTATE_PARTIAL,/**< The kernel memory was partially re-used and partially allocated */
    PCILIB_KMEM_REUSE_PERSISTENT = 0x100,		/**< Flag indicating that persistent kmem was allocated/reused (will not be cleaned on pcilib_free_kernel_memory() unless forced with PCILIB_KMEM_FLAG_PERSISTENT */
    PCILIB_KMEM_REUSE_HARDWARE = 0x200			/**< Flag indicating that kmem may be used by hardware device */
} pcilib_kmem_reuse_state_t;


typedef struct {
    int handle_id;					/**< handled id is used to indentify buffer to kernel module */
    pcilib_kmem_reuse_state_t reused;			/**< indicates if the buffer was allocated or used */

    uintptr_t pa;					/**< physical address of buffer */
    uintptr_t ba;					/**< bus address of buffer (if it is mapped for DMA operations) */
    void* volatile ua;					/**< pointer to buffer in the process address space */
    size_t size;					/**< size of the buffer in bytes */

    size_t alignment_offset;				/**< we may request alignment of allocated buffers. To enusre proper alignment the larger buffer will be allocated and the offset will specify the first position in the buffer fullfilling alignment request */
    size_t mmap_offset;					/**< mmap always maps pages, if physical address is not aligned to page boundary, this is the offset of the buffer relative to the pointer returned by mmap (and stored in \a ua) */
} pcilib_kmem_addr_t;

/**
 * single allocation - we set only addr, n_blocks = 0
 * multiple allocation - addr is not set, blocks are set, n_blocks > 0
 * sgmap allocation - addr contains ua, but pa's are set in blocks, n_blocks > 0
 */
typedef struct {
    pcilib_kmem_type_t type;				/**< The type of kernel memory (how it is allocated) */
    pcilib_kmem_use_t use;				/**< The purpose of kernel memory (how it will be used) */
    pcilib_kmem_reuse_state_t reused;			/**< Indicates if kernel memory was allocated anew, reused, or partially re-used. The additional flags will provide information about persistance and the hardware access */

    size_t n_blocks;					/**< Number of allocated/re-used buffers in kmem */
    pcilib_kmem_addr_t addr;				/**< Information about the buffer of single-buffer kmem */
    pcilib_kmem_addr_t blocks[];			/**< Information about all the buffers in kmem (variable size) */
} pcilib_kmem_buffer_t;

typedef void pcilib_kmem_handle_t;


struct pcilib_kmem_list_s {
    pcilib_kmem_list_t *next, *prev;			/**< List storring all allocated/re-used kmem's in application */
    pcilib_kmem_buffer_t buf;				/**< The actual kmem context (it is of variable size due to \a blocks and, hence, should be last item in this struct */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function either allocates new buffers in the kernel space or just re-uses existing buffers.
 *
 * Sometimes kernel memory is allocated for the specific use-case, for example to provide buffers for DMA engine.
 * It is possible to specify intended use using \a use parameter. 
 * The kernel memory with the specified use (i.e. \a use != 0) can be declared persistent (\a flags include 
 * PCILIB_KMEM_FLAG_PERSISTENT). Such memory will not be de-allocated on clean-up and will be maintained by 
 * the kernel module across the runs of various pcilib applications. To re-use persistent kernel memory,
 * PCILIB_KMEM_FLAG_REUSE should be set. In this case, the pcilib will either allocate new kernel memory 
 * or re-use the already allocated kernel buffers with the specified \a use. If PCILIB_KMEM_FLAG_REUSE
 * is not set, but the kernel memory with the specified use is already allocated, an error will be returned.
 *
 * It is also possible to allocate persistent kernel memory which can be execlusively used by a single process.
 * The PCILIB_KMEM_FLAG_EXCLUSIVE flag have to be provided to pcilib_alloc_kernel_memory() function in this
 * case. Only a single process may call hold a reference to kernel memory. Before next process would be able
 * to obtain it, the process holding the reference should return it using pcilib_free_kernel_memory()
 * call. For example, DMA engine uses exclusive kernel memory to guarantee that exactly one process is 
 * controlling DMA operations.
 *
 * To clean up allocated persistent kernel memory, pcilib_free_kernel_memory() have to be called with
 * PCILIB_KMEM_FLAG_PERSISTENT flag set.
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] type		- specifies type of allocation (simple pages, DMA pages, consistent memory, etc.)
 * @param[in] nmemb		- specifies number of buffers to allocated
 * @param[in] size		- specifies the size of each buffer in bytes
 * @param[in] alignment		- specifies required alignment, the bus addresses are aligned for PCI-mapped buffers and physical addresses are aligned for the rest (if bounce buffers are used, the physical and bus addresses are not always have the same alignment)
 * @param[in] use		- specifies how the memory will be used (to store software registers, locks, DMA buffers, etc.)
 * @param[in] flags		- various flags defining how buffers should be re-used/allocated and, that, should happen on pcilib_free_kernel_memory()
 * @return 			- pointer on context with allocated kernel memory or NULL in case of error
 */
pcilib_kmem_handle_t *pcilib_alloc_kernel_memory(pcilib_t *ctx, pcilib_kmem_type_t type, size_t nmemb, size_t size, size_t alignment, pcilib_kmem_use_t use, pcilib_kmem_flags_t flags);

/**
 * This function either frees the allocated kernel memory or just releases some of the references.
 *
 * Only reference tracking is performed If the PCILIB_KMEM_FLAG_REUSE flag is passed to the 
 * function. Otherwise, non-persistent memory is released when pcilib_free_kernel_memory() called. 
 * The persistent memory is released if PCILIB_KMEM_FLAG_PERSISTENT is passed in \a flags parameter 
 * unless the hold references are preventing us from releasing this memory. The error is reported
 * in this case. The kernel memory can be freed irrespective of hold references if PCILIB_KMEM_FLAG_FORCE 
 * flag is specified.
 *
 * There are several types of references:
 * - The hardware reference - indicates that the memory may be used by DMA engine of the device. It is set
 * if pcilib_alloc_kernel_memory() is executed with PCILIB_KMEM_FLAG_HARDWARE flag. The reference can 
 * be released if the same flag is passed to pcilib_free_kernel_memory()
 * - The software references - are obtained when the memory is reused with pcilib_alloc_kernel_memory(). 
 * They are released when corresponding pcilib_free_kernel_memory() is called. 
 * - The mmap references - are obtained when the kernel memory is mapped to the virtual memory of application
 * which is normally happen on pcilib_alloc_kernel_memory() and fork of the process. The references are
 * returned when memory is unmapped. This happens on pcilib_free_kernel_memory() and termination of the 
 * process with still mmaped regions.
 *
 * The software references are in a way replicate functionality of the mmap references. Currently, they are 
 * used to keep track of non-persistent memory allocated and re-used within the same application (which is
 * actually discouraged). In this case the number of pcilib_alloc_kernel_memory() should be matched by 
 * number of pcilib_free_kernel_memory() calls and only on a last call the memory will be really released.
 * Normally, all but last calls have to be accompanied by PCILIB_KMEM_FLAG_REUSE flag or the error 
 * is reported. But to keep code simpler, the pcilib will not complain until there are software references 
 * on hold. When the last software reference is released, we try actually clean up the memory and the
 * error is reported if other types of references are still present.
 * The software references may stuck if application crashes before calling pcilib_free_kernel_memory(). 
 * pcilib will prevent us from releasing the kernel memory with stuck references. Such memory can 
 * be cleaned only with PCILIB_KMEM_FLAG_FORCE flag or using  pcilib_clean_kernel_memory() call.
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in,out] k		- kernel memory handle returned from pcilib_alloc_kernel_memory() call
 * @param[in] flags		- various flags defining which referenes to hold and return and if the memory should be preserved or freed
 */
void pcilib_free_kernel_memory(pcilib_t *ctx, pcilib_kmem_handle_t *k, pcilib_kmem_flags_t flags);


/**
 * Free / dereference all kernel memory buffers associated with the specified \a use
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] use		- use-number to clean
 * @param[in] flags		- various flags defining which referenes to hold and return and if the memory should be dereferenced or freed
 * @return 			- error or 0 on success
 */
int pcilib_clean_kernel_memory(pcilib_t *ctx, pcilib_kmem_use_t use, pcilib_kmem_flags_t flags);


/**
 * Synchronizes usage of the specified buffer between hardware and the system.
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] k			- kernel memory handle returned from pcilib_alloc_kernel_memory() call
 * @param[in] dir 		- synchronization direction (allows either device or system access)
 * @param[in] block		- specifies the buffer within the kernel memory (buffers are numbered from 0)
 * @return 			- error or 0 on success
 */
int pcilib_kmem_sync_block(pcilib_t *ctx, pcilib_kmem_handle_t *k, pcilib_kmem_sync_direction_t dir, size_t block);


/**
 * Get a valid pointer on the user-space mapping of a single-buffer kernel memory
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] k			- kernel memory handle returned from pcilib_alloc_kernel_memory() call
 * @return 			- user-space pointer
 */
void* volatile pcilib_kmem_get_ua(pcilib_t *ctx, pcilib_kmem_handle_t *k);

/**
 * Get a physical address of a single-buffer kernel memory
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] k			- kernel memory handle returned from pcilib_alloc_kernel_memory() call
 * @return 			- physical address
 */
uintptr_t pcilib_kmem_get_pa(pcilib_t *ctx, pcilib_kmem_handle_t *k);

/**
 * Get a bus address of a single-buffer kernel memory
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] k			- kernel memory handle returned from pcilib_alloc_kernel_memory() call
 * @return 			- bus address or 0 if no bus mapping
 */
uintptr_t pcilib_kmem_get_ba(pcilib_t *ctx, pcilib_kmem_handle_t *k);

/**
 * Get a valid pointer on the user-space mapping of the specified kernel memory buffer
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] k			- kernel memory handle returned from pcilib_alloc_kernel_memory() call
 * @param[in] block		- specifies the buffer within the kernel memory (buffers are numbered from 0)
 * @return 			- user-space pointer
 */
void* volatile pcilib_kmem_get_block_ua(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);

/**
 * Get a physical address of the specified kernel memory buffer
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] k			- kernel memory handle returned from pcilib_alloc_kernel_memory() call
 * @param[in] block		- specifies the buffer within the kernel memory (buffers are numbered from 0)
 * @return 			- physical address
 */
uintptr_t pcilib_kmem_get_block_pa(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);

/**
 * Get a bus address of the specified kernel memory buffer
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] k			- kernel memory handle returned from pcilib_alloc_kernel_memory() call
 * @param[in] block		- specifies the buffer within the kernel memory (buffers are numbered from 0)
 * @return 			- bus address or 0 if no bus mapping
 */
uintptr_t pcilib_kmem_get_block_ba(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);


/**
 * Get size of the specified kernel memory buffer
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] k			- kernel memory handle returned from pcilib_alloc_kernel_memory() call
 * @param[in] block		- specifies the buffer within the kernel memory (buffers are numbered from 0)
 * @return 			- size in bytes
 */
size_t pcilib_kmem_get_block_size(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);


/**
 * Reports if the kernel memory was reused fully, partially, or allocated completely anew
 *
 * @param[in,out] ctx		- pcilib context
 * @param[in] k			- kernel memory handle returned from pcilib_alloc_kernel_memory() call
 * @return 			- re-use information
 */
pcilib_kmem_reuse_state_t pcilib_kmem_is_reused(pcilib_t *ctx, pcilib_kmem_handle_t *k);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_KMEM_H */
