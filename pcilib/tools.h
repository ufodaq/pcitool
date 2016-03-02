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

/**
 * Check if provided string is a decimal integer
 * @param[in] str	- string to check
 * @return		- 1 if string is a number and 0 - otherwise
 */
int pcilib_isnumber(const char *str);

/**
 * Check if provided string is a hexdecimal integer, optionally prefexed with 0x
 * @param[in] str	- string to check
 * @return		- 1 if string is a number and 0 - otherwise
 */
int pcilib_isxnumber(const char *str);

/**
 * Check if first \p len bytes of the provided string is a decimal integer
 * @param[in] str	- string to check
 * @param[in] len	- size of the string
 * @return		- 1 if string is a number and 0 - otherwise
 */
int pcilib_isnumber_n(const char *str, size_t len);

/**
 * Check if first \p len bytes of the provided string is a hexdecimal integer, optionally prefexed with 0x
 * @param[in] str	- string to check
 * @param[in] len	- size of the string
 * @return		- 1 if string is a number and 0 - otherwise
 */
int pcilib_isxnumber_n(const char *str, size_t len);

/**
 * Change the endianess of the provided number (between big- and little-endian format)
 * @param[in] x		- number in little/big endian format
 * @return		- number in big/little endian format
 */
uint16_t pcilib_swap16(uint16_t x);

/**
 * Change the endianess of the provided number (between big- and little-endian format)
 * @param[in] x		- number in little/big endian format
 * @return		- number in big/little endian format
 */
uint32_t pcilib_swap32(uint32_t x);

/**
 * Change the endianess of the provided number (between big- and little-endian format)
 * @param[in] x		- number in little/big endian format
 * @return		- number in big/little endian format
 */
uint64_t pcilib_swap64(uint64_t x);

/**
 * Change the endianess of the provided array
 * @param[out] dst 	- the destination memory region, can be equal to \p src
 * @param[in] src 	- the source memory region
 * @param[in] access	- the size of word in bytes (1, 2, 4, or 8)
 * @param[in] n 	- the number of words to copy (\p n * \p access bytes are copied).
 */
void pcilib_swap(void *dst, void *src, size_t access, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* _PCITOOL_TOOS_H */
