#ifndef _PCILIB_MEMCPY_H
#define _PCILIB_MEMCPY_H

#include <stdio.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * The collection of slow memcpy functions to move the data between BAR and system memory. 
 *
 * The hardware may restrict access width or expose different behavior depending on the 
 * access width. These functions access memory using the specified word width only. 
 * 8-, 16-, 32-, and 64-bit wide access is supported.
 *
 * @param[out] dst 	- the destination memory region
 * @param[in] src 	- the source memory region
 * @param[in] access	- the size of word (a single memory access) in bytes
 * @param[in] n 	- the number of words to copy (\p n * \p access bytes are copied).
 * @return 		- `dst` or NULL on error
 */
void *pcilib_memcpy(void * dst, void const * src, uint8_t access, size_t n);

/**
 * The collection of slow memcpy functions to move the data between BAR and system memory. 
 *
 * The hardware may restrict access width or expose different behavior depending on the 
 * access width. This function only perform 8-bit memory accesses.
 *
 * @param[out] dst 	- the destination memory region
 * @param[in] src 	- the source memory region
 * @param[in] len	- the number of bytes to copy
 * @return 		- `dst` or NULL on error
 */
void *pcilib_memcpy8(void * dst, void const * src, size_t len);

/**
 * The collection of slow memcpy functions to move the data between BAR and system memory. 
 *
 * The hardware may restrict access width or expose different behavior depending on the 
 * access width. This function only perform 32-bit memory accesses.
 *
 * @param[out] dst 	- the destination memory region
 * @param[in] src 	- the source memory region
 * @param[in] len	- the number of bytes to copy
 * @return 		- `dst` or NULL on error
 */
void *pcilib_memcpy32(void * dst, void const * src, size_t len);


/**
 * The collection of slow memcpy functions to move the data between BAR and system memory. 
 *
 * The hardware may restrict access width or expose different behavior depending on the 
 * access width. This function only perform 64-bit memory accesses.
 *
 * @param[out] dst 	- the destination memory region
 * @param[in] src 	- the source memory region
 * @param[in] len	- the number of bytes to copy
 * @return 		- `dst` or NULL on error
 */
void *pcilib_memcpy64(void * dst, void const * src, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_MEMCPY_H */
