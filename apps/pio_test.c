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
//#define REALTIME

#define BAR PCILIB_BAR0
#define BITS 32
#define MASK ((1ll << BITS) - 1)


#define WR(addr, value) { *(uint32_t*)(bar + addr) = value; }
#define RD(addr, value) { value = *(uint32_t*)(bar + addr); }

unsigned long long bits[BITS];

int main(int argc, char *argv[]) {
    uint32_t i, j;
    pcilib_t *pci;
    uint32_t reg, value, diff, errors;
    void* volatile bar;
    
    unsigned long long attempts = 0, failures = 0;

    if (argc < 2) {
	printf("Usage: %s <register>\n", argv[0]);
	exit(0);
    }
    
    if (sscanf(argv[1], "%x", &reg) != 1) {
	printf("Can't parse register %s\n", argv[1]);
	exit(1);
    }
    
#ifdef REALTIME
    pid_t pid;
    struct sched_param sched = {0};

    pid = getpid();
    sched.sched_priority = sched_get_priority_min(SCHED_FIFO);
    if (sched_setscheduler(pid, SCHED_FIFO, &sched))
	printf("Warning: not able to get real-time priority\n");
#endif /* REALTIME */

    pci = pcilib_open(DEVICE, PCILIB_MODEL_DETECT);
    if (!pci) {
	printf("pcilib_open\n");
	exit(1);
    }

    bar = pcilib_map_bar(pci, BAR);
    if (!bar) {
	pcilib_close(pci);
	printf("map bar\n");
	exit(1);
    }
    
    for (i = 0; 1; i++) {
	WR(reg, (i%MASK));
	RD(reg, value);
	
	attempts++;
	if (value != (i%MASK)) {
	    failures++;

	    diff = value ^ (i%MASK);
	    for (errors = 0, j = 0; j < BITS; j++)
		if (diff&(1<<j)) errors++;
	    bits[errors]++;
		
	    //printf("written: %x, read: %x\n", i, value);
	}

	if ((i % 1048576 ) == 0) {
	    printf("Attempts %llu, failures %llu (", attempts, failures);
	    for (j = 1; j < BITS; j++)
		printf("%llu ", bits[j]);
	    printf(")\n");
	}

    }

    pcilib_unmap_bar(pci, BAR, bar);
    pcilib_close(pci);
}
