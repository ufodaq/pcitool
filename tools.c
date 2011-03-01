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
        uint32_t a = (*plSrc & 0xFF) << 24;
        a |= (*plSrc & 0xFF00) << 8;
        a |= (*plSrc & 0xFF0000) >> 8;
        a |= (*plSrc & 0xFF000000) >> 24;
        *plDst = a;
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

void *memcpy64(void * dst, void const * src, size_t len) {
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

/*
void *memcpy128(void * dst, void const * src, size_t len) {

    long pos = - (len>>2);
    char * plDst = (char *) dst - 4 * pos;
    char const * plSrc = (char const *) src - 4 * pos;

    if (pos) {
        __asm__ __volatile__ (
            "1:						\n\t"
            "mov	(%0,%2,4), %%edi		\n\t"
            "mov	%%edi, (%1,%2,4)		\n\t"
            "inc	%2				\n\t"
            "jnz 	1b				\n\t"
	: 
	: "r" (plSrc), "r" (plDst), "r" (pos)
	: "%edi"
        );
    }



    long pos = - ((len>>4)<<4);
    char * plDst = (char *) dst - pos;
    char const * plSrc = (char const *) src - pos;

    if (pos) {
        __asm__ __volatile__ (
            "1:						\n\t"
//            "movdqa	(%0,%2), %%xmm0			\n\t"
            "mov	(%0,%2), %%esi			\n\t"
            "movd	%%esi, %%xmm0			\n\t"
            "mov	4(%0,%2), %%esi			\n\t"
            "movd	%%esi, %%xmm1			\n\t"
            "mov	8(%0,%2), %%esi			\n\t"
            "movd	%%esi, %%xmm2			\n\t"
            "mov	12(%0,%2), %%esi		\n\t"
            "movd	%%esi, %%xmm3			\n\t"
	    "pslldq	$4, %%xmm1			\n\t"
	    "por	%%xmm1, %%xmm0			\n\t"
	    "pslldq	$8, %%xmm2			\n\t"
	    "por	%%xmm2, %%xmm0			\n\t"
	    "pslldq	$12, %%xmm3			\n\t"
	    "por	%%xmm3, %%xmm0			\n\t"
	    
            "movntdq	%%xmm0, (%1,%2)			\n\t"
            "add	$16, %2				\n\t"
            "jnz 	1b				\n\t"
	: 
	: "r" (plSrc), "r" (plDst), "r" (pos)
	: "%rsi"
        );
    }



    len &= 0x3;

    char * pcDst = (char *) plDst;
    char const * pcSrc = (char const *) plSrc;

    while (len--) {
        *pcDst++ = *pcSrc++;
    }

    return (dst);
} 
*/


int get_page_mask() {
    int pagesize,pagemask,temp;

    pagesize = getpagesize();

    for( pagemask=0, temp = pagesize; temp != 1; ) {
	temp = (temp >> 1);
	pagemask = (pagemask << 1)+1;
    }
    return pagemask;
}
