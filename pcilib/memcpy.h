#ifndef _PCILIB_MEMCPY_H
#define _PCILIB_MEMCPY_H

#include <stdio.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

void *pcilib_memcpy8(void * dst, void const * src, size_t len);
void *pcilib_memcpy32(void * dst, void const * src, size_t len);
void *pcilib_memcpy64(void * dst, void const * src, size_t len);
void *pcilib_memcpy(void * dst, void const * src, uint8_t access, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_MEMCPY_H */
