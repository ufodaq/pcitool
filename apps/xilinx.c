#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>

#include "pcilib.h"
#include "irq.h"
#include "kmem.h"

#define DEVICE "/dev/fpga0"
#define BAR PCILIB_BAR0
#define USE PCILIB_KMEM_USE(PCILIB_KMEM_USE_USER, 1)
#define BUFFERS 16
#define ITERATIONS 16384
#define PAGE_SIZE 4096
#define TIMEOUT 100000

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

int main() {
    int err;
    int i, j;
    pcilib_t *pci;
    pcilib_kmem_handle_t *kbuf;
    uint32_t status;
    struct timeval start, end;
    size_t size, run_time;
    void* volatile bar;

    pcilib_kmem_flags_t clean_flags = PCILIB_KMEM_FLAG_HARDWARE|PCILIB_KMEM_FLAG_PERSISTENT|PCILIB_KMEM_FLAG_EXCLUSIVE;


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
    WR(0x10, 0x20)
    WR(0x14, 0x13131313)


    gettimeofday(&start, NULL);
    for (i = 0; i < ITERATIONS; i++) {
	for (j = 0; j < BUFFERS; j++ ) {
	    uintptr_t ba = pcilib_kmem_get_block_ba(pci, kbuf, j);
	    WR(0x08, ba)
	    
	    WR(0x04, 0x01)

	    err = pcilib_wait_irq(pci, PCILIB_IRQ_SOURCE_DEFAULT, TIMEOUT, NULL);
	    if (err) printf("Timeout waiting for IRQ, err: %i\n", err);
	    
	    RD(0x04, status);
	    if ((status&0xFFFF) != 0x101) printf("Invalid status %x\n", status);
//	    WR(0x04, 0x00);

	    WR(0x00, 1)
	    do {
		RD(0x04, status);
	    } while (status != 0);
//	    usleep(1);
	    WR(0x00, 0)
	}
    }
    gettimeofday(&end, NULL);

    pcilib_free_kernel_memory(pci, kbuf,  0);
    pcilib_disable_irq(pci, 0);
    pcilib_unmap_bar(pci, BAR, bar);
    pcilib_close(pci);

    run_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    size = ITERATIONS * BUFFERS * PAGE_SIZE;

    printf("%.3lf GB/s: transfered %zu bytes in %zu us using %u buffers\n", 1000000. * size / run_time / 1024 / 1024 / 1024, size, run_time, BUFFERS);
}
