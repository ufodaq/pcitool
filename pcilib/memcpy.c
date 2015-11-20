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

void *pcilib_memcpy8(void * dst, void const * src, size_t len) {
    int i;
    for (i = 0; i < len; i++) ((char*)dst)[i] = ((char*)src)[i];
    return dst;
}

void *pcilib_memcpy32(void * dst, void const * src, size_t len) {
    uint32_t * plDst = (uint32_t *) dst;
    uint32_t const * plSrc = (uint32_t const *) src;

    while (len >= 4) {
//        *plDst = ntohl(*plSrc);
	*plDst = *plSrc;
        plSrc++;
        plDst++;
        len -= 4;
    }

    char * pcDst = (char *) plDst;
    char const * pcSrc = (char const *) plSrc;

    while (len--) {
        *pcDst++ = *pcSrc++;
    }

    return (dst);
} 


void *pcilib_memcpy64(void * dst, void const * src, size_t len) {
    uint64_t * plDst = (uint64_t *) dst;
    uint64_t const * plSrc = (uint64_t const *) src;

    while (len >= 8) {
        *plDst++ = *plSrc++;
        len -= 8;
    }

    char * pcDst = (char *) plDst;
    char const * pcSrc = (char const *) plSrc;

    while (len--) {
        *pcDst++ = *pcSrc++;
    }

    return (dst);
} 

typedef void* (*pcilib_memcpy_routine_t)(void * dst, void const *src, size_t bytes);
static pcilib_memcpy_routine_t pcilib_memcpy_routines[4] = {
    pcilib_memcpy8, NULL, pcilib_memcpy32, pcilib_memcpy64
};

void *pcilib_memcpy(void * dst, void const * src, uint8_t access, size_t n) {
    size_t pos = 0, size = n * access;
    pcilib_memcpy_routine_t routine;

    assert((access)&&(access <= 8));

    while (access >>= 1) ++pos;
    routine = pcilib_memcpy_routines[pos];

    return routine(dst, src, size);
}
