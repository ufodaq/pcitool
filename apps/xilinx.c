#define _BSD_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sched.h>
#include <sys/time.h>

#include "pcilib.h"
#include "irq.h"
#include "kmem.h"

#define DEVICE "/dev/fpga0"
#define BAR PCILIB_BAR0
#define USE PCILIB_KMEM_USE(PCILIB_KMEM_USE_USER, 1)
#define BUFFERS 16
#define ITERATIONS 16384
#define PAGE_SIZE 4096	// other values are not supported in the kernel
#define TIMEOUT 100000

/* IRQs are slow for some reason. REALTIME mode is slower. Adding delays does not really help,
otherall we have only 3 checks in average. Check ready seems to be not needed and adds quite 
much extra time */
//#define USE_IRQ
//#define CHECK_READY
//#define REALTIME
//#define ADD_DELAYS

//#define WR(addr, value) { val = value; pcilib_write(pci, BAR, addr, sizeof(val), &val); }
//#define RD(addr, value) { pcilib_read(pci, BAR, addr, sizeof(val), &val); value = val; }
#define WR(addr, value) { *(uint32_t*)(bar + addr) = value; }
#define RD(addr, value) { value = *(uint32_t*)(bar + addr); }

static void fail(const char *msg, ...) {
    va_list va;
    
    va_start(va, msg);
    vprintf(msg, va);
    va_end(va);
    printf("\n");

    exit(-1);
}

void hpsleep(size_t ns) {
    struct timespec wait, tv;

    clock_gettime(CLOCK_REALTIME, &wait);

    wait.tv_nsec += ns;
    if (wait.tv_nsec > 999999999) {
	wait.tv_sec += 1;
	wait.tv_nsec = 1000000000 - wait.tv_nsec;
    }

    do {
	clock_gettime(CLOCK_REALTIME, &tv);
    } while ((wait.tv_sec > tv.tv_sec)||((wait.tv_sec == tv.tv_sec)&&(wait.tv_nsec > tv.tv_nsec)));
}


int main() {
    int err;
    int i, j;
    pcilib_t *pci;
    pcilib_kmem_handle_t *kbuf;
    uint32_t status;
    struct timeval start, end;
    size_t size, run_time;
    void* volatile bar;
    uintptr_t bus_addr[BUFFERS];

    pcilib_kmem_flags_t clean_flags = PCILIB_KMEM_FLAG_HARDWARE|PCILIB_KMEM_FLAG_PERSISTENT|PCILIB_KMEM_FLAG_EXCLUSIVE;

#ifdef ADD_DELAYS
    long rpt = 0, rpt2 = 0;
    size_t best_time;
    best_time = 1000000000L * PAGE_SIZE / (4L * 1024 * 1024 * 1024);
#endif /* ADD_DELAYS */

#ifdef REALTIME
    pid_t pid;
    struct sched_param sched = {0};

    pid = getpid();
    sched.sched_priority = sched_get_priority_min(SCHED_FIFO);
    if (sched_setscheduler(pid, SCHED_FIFO, &sched))
	printf("Warning: not able to get real-time priority\n");
#endif /* REALTIME */

    pci = pcilib_open(DEVICE, PCILIB_MODEL_DETECT);
    if (!pci) fail("pcilib_open");

    bar = pcilib_map_bar(pci, BAR);
    if (!bar) {
	pcilib_close(pci);
	fail("map bar");
    }

    pcilib_enable_irq(pci, PCILIB_IRQ_TYPE_ALL, 0);
    pcilib_clear_irq(pci, PCILIB_IRQ_SOURCE_DEFAULT);

    pcilib_clean_kernel_memory(pci, USE, clean_flags);


    kbuf = pcilib_alloc_kernel_memory(pci, PCILIB_KMEM_TYPE_DMA_C2S_PAGE, BUFFERS, PAGE_SIZE, 4096, USE, 0);

    
    WR(0x00, 1)
    usleep(1000);
    WR(0x00, 0)
    WR(0x04, 0)
    
    WR(0x0C, 0x20)
    WR(0x10, (PAGE_SIZE / 0x80))
    WR(0x14, 0x13131313)

    for (j = 0; j < BUFFERS; j++ ) {
        bus_addr[j] = pcilib_kmem_get_block_ba(pci, kbuf, j);
    }

    gettimeofday(&start, NULL);

    for (i = 0; i < ITERATIONS; i++) {
	for (j = 0; j < BUFFERS; j++ ) {
//	    uintptr_t ba = pcilib_kmem_get_block_ba(pci, kbuf, j);
//	    WR(0x08, ba)
	    WR(0x08, bus_addr[j]);
	    WR(0x04, 0x01)

#ifdef USE_IRQ
	    err = pcilib_wait_irq(pci, PCILIB_IRQ_SOURCE_DEFAULT, TIMEOUT, NULL);
	    if (err) printf("Timeout waiting for IRQ, err: %i\n", err);

	    RD(0x04, status);
	    if ((status&0xFFFF) != 0x101) printf("Invalid status %x\n", status);
//	    WR(0x04, 0x00);
#else /* USE_IRQ */
# ifdef ADD_DELAYS
//	    hpsleep(best_time);
	    do {
		rpt++;
		RD(0x04, status);
	    } while (status != 0x101);
# else /* ADD_DELAYS */
	    do {
		RD(0x04, status);
	    } while (status != 0x101);
# endif /* ADD_DELAYS */
#endif /* USE_IRQ */

	    WR(0x00, 1)
#ifdef CHECK_READY
	    do {
		rpt2++;
		RD(0x04, status);
	    } while (status != 0);
#endif /* CHECK_READY */
	    WR(0x00, 0)
	}
    }
    gettimeofday(&end, NULL);

    pcilib_free_kernel_memory(pci, kbuf,  0);
    pcilib_disable_irq(pci, 0);
    pcilib_unmap_bar(pci, BAR, bar);
    pcilib_close(pci);

    run_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    size = (long long int)ITERATIONS * BUFFERS * PAGE_SIZE;

    printf("%.3lf GB/s: transfered %zu bytes in %zu us using %u buffers\n", 1000000. * size / run_time / 1024 / 1024 / 1024, size, run_time, BUFFERS);

# ifdef ADD_DELAYS
    printf("Repeats: %lf, %lf\n",1. * rpt / (ITERATIONS * BUFFERS), 1. * rpt2 / (ITERATIONS * BUFFERS));
#endif /* USE_IRQ */	    
}
