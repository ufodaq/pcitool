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

void *pcilib_datacpy32(void * dst, void const * src, size_t n, pcilib_endianess_t endianess) {
    uint32_t * plDst = (uint32_t *) dst;
    uint32_t const * plSrc = (uint32_t const *) src;

    int swap = 0;

    if (endianess) 
        swap = (endianess == PCILIB_BIG_ENDIAN)?(ntohs(1)!=1):(ntohs(1)==1);

    if (swap) {
        while (n > 0) {
            *plDst = ntohl(*plSrc);
            ++plSrc;
            ++plDst;
            --n;
        }
    } else {
        while (n > 0) {
            *plDst = *plSrc;
            ++plSrc;
            ++plDst;
            --n;
        }
    }

    return dst;
} 

void *pcilib_datacpy64(void * dst, void const * src, size_t n, pcilib_endianess_t endianess) {
    uint64_t * plDst = (uint64_t *) dst;
    uint64_t const * plSrc = (uint64_t const *) src;

    int swap = 0;

    if (endianess) 
        swap = (endianess == PCILIB_BIG_ENDIAN)?(be64toh(1)!=1):(be64toh(1)==1);

    if (swap) {
        while (n > 0) {
            *plDst = ntohl(*plSrc);
            ++plSrc;
            ++plDst;
            --n;
        }
    } else {
        while (n > 0) {
            *plDst = *plSrc;
            ++plSrc;
            ++plDst;
            --n;
        }
    }

    return dst;
}

typedef void* (*pcilib_datacpy_routine_t)(void * dst, void const * src, size_t n, pcilib_endianess_t endianess);
static pcilib_datacpy_routine_t pcilib_datacpy_routines[4] = {
    NULL, NULL, pcilib_datacpy32, pcilib_datacpy64
};

void *pcilib_datacpy(void * dst, void const * src, uint8_t size, size_t n, pcilib_endianess_t endianess) {
    size_t pos = 0;
    pcilib_datacpy_routine_t routine;

    assert((size)&&(size <= 8));

    while (size >>= 1) ++pos;
    routine = pcilib_datacpy_routines[pos];

    return routine(dst, src, n, endianess);
}
