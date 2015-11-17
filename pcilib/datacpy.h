#ifndef _PCILIB_DATACPY_H
#define _PCILIB_DATACPY_H

#include <stdio.h>
#include <stdint.h>

#include <pcilib.h>

#ifdef __cplusplus
extern "C" {
#endif

void *pcilib_datacpy32(void * dst, void const * src, size_t n, pcilib_endianess_t endianess);
void *pcilib_datacpy64(void * dst, void const * src, size_t n, pcilib_endianess_t endianess);
void *pcilib_datacpy(void * dst, void const * src, uint8_t size, size_t n, pcilib_endianess_t endianess);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_DATACPY_H */
