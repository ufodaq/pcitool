#ifndef _PCITOOL_TOOLS_H
#define _PCITOOL_TOOLS_H

#include <stdio.h>
#include <stdint.h>

#include <pcilib.h>

#define BIT_MASK(bits) ((1ll << (bits)) - 1)

#define min2(a, b) (((a)<(b))?(a):(b))

#ifdef __cplusplus
extern "C" {
#endif

int pcilib_isnumber(const char *str);
int pcilib_isxnumber(const char *str);
int pcilib_isnumber_n(const char *str, size_t len);
int pcilib_isxnumber_n(const char *str, size_t len);

uint16_t pcilib_swap16(uint16_t x);
uint32_t pcilib_swap32(uint32_t x);
uint64_t pcilib_swap64(uint64_t x);
void pcilib_swap(void *dst, void *src, size_t size, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* _PCITOOL_TOOS_H */
