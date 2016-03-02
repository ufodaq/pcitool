#ifndef _PCILIB_DATACPY_H
#define _PCILIB_DATACPY_H

#include <stdio.h>
#include <stdint.h>

#include <pcilib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The collection of slow memcpy functions to move the data between BAR and system memory.
 * If necessary the endianess conversion is performed to ensure that the data is encoded 
 * using the specified endianess in the BAR memory and using the native host order in the 
 * system memory. Since endianess conversion is symmetric, it is irrelevant if we are 
 * copying from system memory to BAR memory or vice-versa.
 *
 * The hardware may restrict access width or expose different behavior depending on the 
 * access width. These functions access memory using the specified word width only. 
 * 8-, 16-, 32-, and 64-bit wide access is supported.
 *
 * @param[out] dst 	- the destination memory region
 * @param[in] src 	- the source memory region
 * @param[in] access	- the size of word (a single memory access) in bytes
 * @param[in] n 	- the number of words to copy (\p n * \p access bytes are copied).
 * @param[in] endianess	- the endianess of the data words in the BAR memory
 * @return 		- `dst` or NULL on error
 */
void *pcilib_datacpy(void * dst, void const * src, uint8_t access, size_t n, pcilib_endianess_t endianess);

/**
 * The collection of slow memcpy functions to move the data between BAR and system memory. 
 * If necessary the endianess conversion is performed to ensure that the data is encoded 
 * using the specified endianess in the BAR memory and using the native host order in the 
 * system memory. Since endianess conversion is symmetric, it is irrelevant if we are 
 * copying from system memory to BAR memory or vice-versa.
 *
 * The hardware may restrict access width or expose different behavior depending on the 
 * access width. This function only perform 32-bit memory accesses.
 *
 * @param[out] dst 	- the destination memory region
 * @param[in] src 	- the source memory region
 * @param[in] n		- the number of 32-bit words to copy (4 * \p n bytes are copied)
 * @param[in] endianess	- the endianess of the data words in the BAR memory
 * @return 		- `dst` or NULL on error
 */
void *pcilib_datacpy32(void * dst, void const * src, size_t n, pcilib_endianess_t endianess);

/**
 * The collection of slow memcpy functions to move the data between BAR and system memory. 
 * If necessary the endianess conversion is performed to ensure that the data is encoded 
 * using the specified endianess in the BAR memory and using the native host order in the 
 * system memory. Since endianess conversion is symmetric, it is irrelevant if we are 
 * copying from system memory to BAR memory or vice-versa.
 *
 * The hardware may restrict access width or expose different behavior depending on the 
 * access width. This function only perform 64-bit memory accesses.
 *
 * @param[out] dst 	- the destination memory region
 * @param[in] src 	- the source memory region
 * @param[in] n		- the number of 64-bit words to copy (8 * \p n bytes are copied)
 * @param[in] endianess	- the endianess of the data words in the BAR memory
 * @return 		- `dst` or NULL on error
 */
void *pcilib_datacpy64(void * dst, void const * src, size_t n, pcilib_endianess_t endianess);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_DATACPY_H */
