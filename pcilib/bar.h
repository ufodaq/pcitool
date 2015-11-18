#ifndef _PCILIB_BAR_H
#define _PCILIB_BAR_H


typedef struct {
    pcilib_bar_t bar;
    size_t size;
    uintptr_t phys_addr;
    void *virt_addr;
} pcilib_bar_info_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Detects in which PCI BAR the specified buffer is residing. The complete specified buffer
 * of \a size bytes should fit into the BAR. Otherwise, an error will be returned.
 * @param[in] ctx	- pcilib context
 * @param[in] addr	- physical address of the begining of the buffer
 * @param[in] size	- the size of the buffer
 * @return		- return the BAR (numbered from 0) or PCILIB_BAR_INVALID if buffer does not belong to any BAR.
 */
pcilib_bar_t pcilib_detect_bar(pcilib_t *ctx, uintptr_t addr, size_t size);

/**
 * Maps the specified bar to the address space of the process. This function may be called multiple times for 
 * the same bar. It will only map the BAR once. Normally, the required BARs will be automatically mapped when
 * BAR addresses are resolved with pcilib_resolve_bar_address() and similar.
 * @param[in,out] ctx 	- pcilib context
 * @param[in] bar	- the PCI BAR number (numbered from 0)
 * return 		- the address where the BAR is mapped. You can't use this address directly, 
 *			instead pcilib_resolve_register_address(ctx, bar, 0) have to be used to find 
 *			the start of the BAR in virtual address space.
 */
void *pcilib_map_bar(pcilib_t *ctx, pcilib_bar_t bar);

/**
 * Unmaps the specified bar from the address space of the process. Actually, it will only unmap the BAR if it is 
 * not used by DMA or Event egines. So, it is fine to include the calls to pcilib_map_bar() / pcilib_unmap_bar() 
 * when a specific BAR is needed. On other hand, there is a little point in doing so. The BAR may be left mapped
 * and will be automatically umapped when pcilib_close() is called.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bar	- BAR number (numbered from 0)
 * @param[in,out] data	- The address returned by pcilib_map_bar() call
 */
void pcilib_unmap_bar(pcilib_t *ctx, pcilib_bar_t bar, void *data);

/**
 * Detects the BAR and mapping offset of the specified PCI buffer. The complete specified buffer
 * of \a size bytes should fit into the BAR. Otherwise, an error will be returned.
 * @param[in] ctx	- pcilib context
 * @param[in,out] bar	- the function will check if the buffer belongs to the specified BAR unless bar is PCILIB_BAR_DETECT.
 *			It will be set to the actually detected BAR in the last case.
 * @param[in,out] addr	- specifies the address to detect. The address may be specified as absolute physical address or offset in the BAR.
 *			On success, the addr will contain offset which should be added to the address returned by pcilib_map_bar()
 *			to get position of BAR mapping in virtual address space.
 * @param[in] size	- the size of the buffer
 * @return		- error code or 0 in case of success
 */
int pcilib_detect_address(pcilib_t *ctx, pcilib_bar_t *bar, uintptr_t *addr, size_t size);

/**
 * Resolves the virtual address and the size of PCI BAR space used for data exchange.
 * This is left-over from older version of pcilib and currently unused. We may reconsider
 * how it is organized and implemented. The data_space parameter may go into the model definition.
 * @param[in] ctx	- pcilib context
 * @param[in] addr	- may hint there the data space is located, use 0 to autodetect
 * @param[out] size	- the size of data space is returned
 * @return		- virtual address of data space (ready to use) or NULL if detection has failed
 */
char *pcilib_resolve_data_space(pcilib_t *ctx, uintptr_t addr, size_t *size);

const pcilib_bar_info_t *pcilib_get_bar_info(pcilib_t *ctx, pcilib_bar_t bar);
const pcilib_bar_info_t *pcilib_get_bar_list(pcilib_t *ctx);


#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_BAR_H */
