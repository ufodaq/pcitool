#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>

#include "pcilib.h"
#include "locking.h"
#include "mem.h"
#include "error.h"
#include "pci.h"



void *pcilib_map_area(pcilib_t *ctx, uintptr_t addr, size_t size) {
    void *res;
    int err, ret; 

    err = pcilib_lock_global(ctx);
    if (err) {
	pcilib_error("Error (%i) acquiring mmap lock", err);
	return NULL;
    }

    ret = ioctl( ctx->handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_AREA );
    if (ret) {
	pcilib_unlock_global(ctx);
	pcilib_error("PCIDRIVER_IOC_MMAP_MODE ioctl have failed");
	return NULL;
    }

    res = mmap( 0, size, PROT_WRITE | PROT_READ, MAP_SHARED, ctx->handle, addr );

    pcilib_unlock_global(ctx);

    if ((!res)||(res == MAP_FAILED)) {
	pcilib_error("Failed to mmap area 0x%lx of size %zu bytes", addr, size);
	return NULL;
    }

    return res;
}

void pcilib_unmap_area(pcilib_t *ctx, void *addr, size_t size) {
    munmap(addr, size);
}
