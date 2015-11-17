#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdint.h>
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

static void pcilib_run_cpuid(uint32_t eax, uint32_t ecx, uint32_t* abcd) {
    uint32_t ebx = 0, edx;
# if defined( __i386__ ) && defined ( __PIC__ )
     /* in case of PIC under 32-bit EBX cannot be clobbered */
    __asm__ ( "movl %%ebx, %%edi \n\t cpuid \n\t xchgl %%ebx, %%edi" : "=D" (ebx),
# else
    __asm__ ( "cpuid" : "+b" (ebx),
# endif
              "+a" (eax), "+c" (ecx), "=d" (edx) );
    abcd[0] = eax; abcd[1] = ebx; abcd[2] = ecx; abcd[3] = edx;
}

static int pcilib_check_xcr0_ymm() {
    uint32_t xcr0;
    __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
    return ((xcr0 & 6) == 6); /* checking if xmm and ymm state are enabled in XCR0 */
}

static int pcilib_check_4th_gen_intel_core_features() {
    uint32_t abcd[4];
    uint32_t fma_movbe_osxsave_mask = ((1 << 12) | (1 << 22) | (1 << 27));
    uint32_t avx2_bmi12_mask = (1 << 5) | (1 << 3) | (1 << 8);

    /* CPUID.(EAX=01H, ECX=0H):ECX.FMA[bit 12]==1   && 
       CPUID.(EAX=01H, ECX=0H):ECX.MOVBE[bit 22]==1 && 
       CPUID.(EAX=01H, ECX=0H):ECX.OSXSAVE[bit 27]==1 */
    pcilib_run_cpuid( 1, 0, abcd );
    if ( (abcd[2] & fma_movbe_osxsave_mask) != fma_movbe_osxsave_mask ) 
        return 0;

    if ( ! pcilib_check_xcr0_ymm() )
        return 0;

    /*  CPUID.(EAX=07H, ECX=0H):EBX.AVX2[bit 5]==1  &&
        CPUID.(EAX=07H, ECX=0H):EBX.BMI1[bit 3]==1  &&
        CPUID.(EAX=07H, ECX=0H):EBX.BMI2[bit 8]==1  */
    pcilib_run_cpuid( 7, 0, abcd );
    if ( (abcd[1] & avx2_bmi12_mask) != avx2_bmi12_mask ) 
        return 0;

    /* CPUID.(EAX=80000001H):ECX.LZCNT[bit 5]==1 */
    pcilib_run_cpuid( 0x80000001, 0, abcd );
    if ( (abcd[2] & (1 << 5)) == 0)
        return 0;

    return 1;
}

static int pcilib_detect_cpu_gen() {
    if (pcilib_check_4th_gen_intel_core_features())
	return 4;
    return 0;
}

int pcilib_get_cpu_gen() {
    int gen = -1;

    if (gen < 0 )
        gen = pcilib_detect_cpu_gen();

    return gen;
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

int pcilib_get_cpu_count() {
    int err;

    int cpu_count;
    cpu_set_t mask;

    err = sched_getaffinity(getpid(), sizeof(mask), &mask);
    if (err) return 1;

#ifdef CPU_COUNT
    cpu_count = CPU_COUNT(&mask);
#else
    for (cpu_count = 0; cpu_count < CPU_SETSIZE; cpu_count++) {
	if (!CPU_ISSET(cpu_count, &mask)) break;
    }
#endif

    if (!cpu_count) cpu_count = PCILIB_DEFAULT_CPU_COUNT;
    return cpu_count;    
}

