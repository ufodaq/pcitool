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

#include "cpu.h"
#include "pci.h"
#include "tools.h"
#include "error.h"


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

void pcilib_memcpy4k_avx(void *dst, const void *src, size_t size) {
    size_t sse_size = (size / 512);

    __asm__ __volatile__ (
            "push 	%2			\n\t"
            "mov        $0, %%rax		\n\t"

            "1:					\n\t"

            "vmovdqa 	   (%1,%%rax), %%ymm0	\n\t"
            "vmovdqa 	 32(%1,%%rax), %%ymm1	\n\t"
            "vmovdqa 	 64(%1,%%rax), %%ymm2	\n\t"
            "vmovdqa 	 96(%1,%%rax), %%ymm3	\n\t"
            "vmovdqa 	128(%1,%%rax), %%ymm4	\n\t"
            "vmovdqa 	160(%1,%%rax), %%ymm5	\n\t"
            "vmovdqa 	192(%1,%%rax), %%ymm6	\n\t"
            "vmovdqa 	224(%1,%%rax), %%ymm7	\n\t"

            "vmovdqa 	256(%1,%%rax), %%ymm8	\n\t"
            "vmovdqa 	288(%1,%%rax), %%ymm9	\n\t"
            "vmovdqa 	320(%1,%%rax), %%ymm10	\n\t"
            "vmovdqa 	352(%1,%%rax), %%ymm11	\n\t"
            "vmovdqa 	384(%1,%%rax), %%ymm12	\n\t"
            "vmovdqa 	416(%1,%%rax), %%ymm13	\n\t"
            "vmovdqa 	448(%1,%%rax), %%ymm14	\n\t"
            "vmovdqa 	480(%1,%%rax), %%ymm15	\n\t"

            "vmovdqa	%%ymm0,    (%0,%%rax)	\n\t"
            "vmovdqa	%%ymm1,  32(%0,%%rax)	\n\t"
            "vmovntps	%%ymm2,  64(%0,%%rax)	\n\t"
            "vmovntps	%%ymm3,  96(%0,%%rax)	\n\t"
            "vmovntps	%%ymm4, 128(%0,%%rax)	\n\t"
            "vmovntps	%%ymm5, 160(%0,%%rax)	\n\t"
            "vmovntps	%%ymm6, 192(%0,%%rax)	\n\t"
            "vmovntps	%%ymm7, 224(%0,%%rax)	\n\t"

            "vmovntps	%%ymm8,  256(%0,%%rax)	\n\t"
            "vmovntps	%%ymm9,  288(%0,%%rax)	\n\t"
            "vmovntps	%%ymm10, 320(%0,%%rax)	\n\t"
            "vmovntps	%%ymm11, 352(%0,%%rax)	\n\t"
            "vmovntps	%%ymm12, 384(%0,%%rax)	\n\t"
            "vmovntps	%%ymm13, 416(%0,%%rax)	\n\t"
            "vmovntps	%%ymm14, 448(%0,%%rax)	\n\t"
            "vmovntps	%%ymm15, 480(%0,%%rax)	\n\t"

            "add	$512, %%rax		\n\t"
            "dec	%2			\n\t"
            "jnz 	1b			\n\t"
            "pop 	%2			\n\t"

            "mfence"
    :
    : "p" (dst), "p" (src), "r" (sse_size)
    : "%rax"
        );
}

void pcilib_pagecpy(void *dst, const void *src, size_t size) {
    int gen = pcilib_get_cpu_gen();
    if ((gen > 3)&&((size%4096)==0)&&(((uintptr_t)dst%32)==0)&&(((uintptr_t)src%32)==0)) {
	pcilib_memcpy4k_avx(dst, src, size);
    } else
	memcpy(dst, src, size);
}
