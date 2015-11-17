#ifndef _PCILIB_PAGECPY_H
#define _PCILIB_PAGECPY_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function should be used to move large blocks of non-cached memory between
 * aligned memory locations. The function will determine the CPU model and alginment
 * and call appropriate implementation. If nothing suitable found, standard memcpy
 * will be used. It is OK to call on small or unligned data, the standard memcpy
 * will be executed in this case. The memory regions should not intersect.
 * Only AVX implementation so far.
 * @param[out] dst - destination memory region
 * @param[in] src - source memory region
 * @param[in] size - size of memory region in bytes.
 * @return - `dst` or NULL on error
 */
void pcilib_pagecpy(void *dst, void *src, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_PAGECPY_H */
