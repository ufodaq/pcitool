#ifndef _PCILIB_MEM_H
#define _PCILIB_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maps the specified memory area in the address space of the process. 
 * @param[in,out] ctx 	- pcilib context
 * @param[in] addr	- hardware address (should be page-aligned)
 * @param[in] size	- size (should be multiple of page size)
 * return 		- the address where the memory area is mapped
 */
void *pcilib_map_area(pcilib_t *ctx, uintptr_t addr, size_t size);

/**
 * Unmaps the specified memory area in the address space of the process. 
 * @param[in,out] ctx 	- pcilib context
 * @param[in] addr	- pointer to the virtual address where the area is mapped
 * @param[in] size	- size (should be multiple of page size)
 */
void pcilib_unmap_area(pcilib_t *ctx, void *addr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_MEM_H */
