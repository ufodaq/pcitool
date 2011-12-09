#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "tools.h"

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

void *pcilib_datacpy32(void * dst, void const * src, uint8_t size, size_t n, pcilib_endianess_t endianess) {
    uint32_t * plDst = (uint32_t *) dst;
    uint32_t const * plSrc = (uint32_t const *) src;

    int swap = 0;

    if (endianess) 
        swap = (endianess == PCILIB_BIG_ENDIAN)?(ntohs(1)!=1):(ntohs(1)==1);

    assert(size == 4);	// only 32 bit at the moment

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
} 

int pcilib_get_page_mask() {
    int pagesize,pagemask,temp;

    pagesize = sysconf(_SC_PAGESIZE);

    for( pagemask=0, temp = pagesize; temp != 1; ) {
	temp = (temp >> 1);
	pagemask = (pagemask << 1)+1;
    }
    return pagemask;
}

int pcilib_add_timeout(struct timeval *tv, pcilib_timeout_t timeout) {
    tv->tv_usec += timeout%1000000;
    if (tv->tv_usec > 999999) {
	tv->tv_usec -= 1000000;
	tv->tv_sec += 1 + timeout/1000000;
    } else {
	tv->tv_sec += timeout/1000000;
    }
}

int pcilib_calc_deadline(struct timeval *tv, pcilib_timeout_t timeout) {
    gettimeofday(tv, NULL);
    pcilib_add_timeout(tv, timeout);
        
    return 0;
}

int pcilib_check_deadline(struct timeval *tve, pcilib_timeout_t timeout) {
    int64_t res;
    struct timeval tvs;
    
    if (!tve->tv_sec) return 0;
    
    gettimeofday(&tvs, NULL);
    res = ((tve->tv_sec - tvs.tv_sec)*1000000 + (tve->tv_usec - tvs.tv_usec));
	// Hm... Some problems comparing signed and unsigned. So, sign check first
    if ((res < 0)||(res < timeout)) {
	return 1;
    }

    return 0;
}

pcilib_timeout_t pcilib_calc_time_to_deadline(struct timeval *tve) {
    int64_t res;
    struct timeval tvs;
    
    gettimeofday(&tvs, NULL);
    res = ((tve->tv_sec - tvs.tv_sec)*1000000 + (tve->tv_usec - tvs.tv_usec));
    
    if (res < 0) return 0;
    return res;
}

int pcilib_sleep_until_deadline(struct timeval *tv) {
    struct timespec wait;
    pcilib_timeout_t duration;

    duration = pcilib_calc_time_to_deadline(tv);
    wait.tv_sec = duration / 1000000;
    wait.tv_nsec = 1000 * (duration % 1000000);
    nanosleep(&wait, NULL);

    return 0;
}

int pcilib_timecmp(struct timeval *tv1, struct timeval *tv2) {
    if (tv1->tv_sec > tv2->tv_sec) return 1;
    else if (tv1->tv_sec > tv2->tv_sec) return -1;
    else if (tv1->tv_usec > tv2->tv_usec) return 1;
    else if (tv1->tv_usec < tv2->tv_usec) return -1;
    return 0;
}
