#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

void *memcpy8(void * dst, void const * src, size_t len) {
    int i;
    for (i = 0; i < len; i++) ((char*)dst)[i] = ((char*)src)[i];
    return dst;
}


void *memcpy32(void * dst, void const * src, size_t len) {
    uint32_t * plDst = (uint32_t *) dst;
    uint32_t const * plSrc = (uint32_t const *) src;

    while (len >= 4) {
        *plDst++ = *plSrc++;
        len -= 4;
    }

    char * pcDst = (char *) plDst;
    char const * pcSrc = (char const *) plSrc;

    while (len--) {
        *pcDst++ = *pcSrc++;
    }

    return (dst);
} 

void *memcpy64(void * dst, void const * src, size_t len) {
    uint64_t * plDst = (uint64_t *) dst;
    uint64_t const * plSrc = (uint64_t const *) src;

    while (len >= 8) {
        *plDst++ = *plSrc++;
        len -= 4;
    }

    char * pcDst = (char *) plDst;
    char const * pcSrc = (char const *) plSrc;

    while (len--) {
        *pcDst++ = *pcSrc++;
    }

    return (dst);
} 


int get_page_mask() {
    int pagesize,pagemask,temp;

    pagesize = getpagesize();

    for( pagemask=0, temp = pagesize; temp != 1; ) {
	temp = (temp >> 1);
	pagemask = (pagemask << 1)+1;
    }
    return pagemask;
}
