#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <sched.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "pci.h"
#include "tools.h"
#include "error.h"

int pcilib_isnumber(const char *str) {
    int i = 0;
    for (i = 0; str[i]; i++) 
	if (!isdigit(str[i])) return 0;
    return 1;
}

int pcilib_isxnumber(const char *str) {
    int i = 0;
    
    if ((str[0] == '0')&&((str[1] == 'x')||(str[1] == 'X'))) i += 2;
    
    for (; str[i]; i++) 
	if (!isxdigit(str[i])) return 0;

    return 1;
}

int pcilib_isnumber_n(const char *str, size_t len) {
    int i = 0;
    for (i = 0; (str[i])&&(i < len); i++) 
	if (!isdigit(str[i])) return 0;
    return 1;
}

int pcilib_isxnumber_n(const char *str, size_t len) {
    int i = 0;
    
    if ((len > 1)&&(str[0] == '0')&&((str[1] == 'x')||(str[1] == 'X'))) i += 2;
    
    for (; (str[i])&&(i < len); i++) 
	if (!isxdigit(str[i])) return 0;

    return 1;
}


uint16_t pcilib_swap16(uint16_t x) {
    return (((x<<8)&0xFFFF) | ((x>>8)&0xFFFF));
}

uint32_t pcilib_swap32(uint32_t x) {
    return ((x & 0xFF) << 24) | \
	((x & 0xFF00) << 8) | \
	((x & 0xFF0000) >> 8) | \
        ((x & 0xFF000000) >> 24); 
}
 
uint64_t pcilib_swap64(uint64_t x) {
    return (((uint64_t)(x) << 56) | \
        (((uint64_t)(x) << 40) & 0xff000000000000ULL) | \
        (((uint64_t)(x) << 24) & 0xff0000000000ULL) | \
        (((uint64_t)(x) << 8)  & 0xff00000000ULL) | \
        (((uint64_t)(x) >> 8)  & 0xff000000ULL) | \
        (((uint64_t)(x) >> 24) & 0xff0000ULL) | \
        (((uint64_t)(x) >> 40) & 0xff00ULL) | \
        ((uint64_t)(x)  >> 56));
}

void pcilib_swap(void *dst, void *src, size_t size, size_t n) {
    int i;
    switch (size) {
	case 1:
	    if (src != dst) memcpy(dst, src, n);
	break;
	case 2:
	    for (i = 0; i < n; i++) {
		((uint16_t*)dst)[i] = pcilib_swap16(((uint16_t*)src)[i]);
	    }    
	break;
	case 4:
	    for (i = 0; i < n; i++) {
		((uint32_t*)dst)[i] = pcilib_swap32(((uint32_t*)src)[i]);
	    }    
	break;
	case 8:
	    for (i = 0; i < n; i++) {
		((uint64_t*)dst)[i] = pcilib_swap64(((uint64_t*)src)[i]);
	    }    
	break;
	default:
	    pcilib_error("Invalid word size: %i", size);
    }
}




