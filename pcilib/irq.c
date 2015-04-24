#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include "pci.h"

#include "tools.h"
#include "error.h"

int pcilib_wait_irq(pcilib_t *ctx, pcilib_irq_hw_source_t source, pcilib_timeout_t timeout, size_t *count) {
    int err;
    
    interrupt_wait_t arg = { 0 };
    
    arg.source = source;
    arg.timeout = timeout;

    if (count) arg.count = 1;

    err = ioctl(ctx->handle, PCIDRIVER_IOC_WAITI, &arg);
    if (err) {
	pcilib_error("PCIDRIVER_IOC_WAITI ioctl have failed");
	return PCILIB_ERROR_FAILED;
    }
    
    if (!arg.count) return PCILIB_ERROR_TIMEOUT;

    if (count) *count = arg.count;
    
    return 0;
}

int pcilib_clear_irq(pcilib_t *ctx, pcilib_irq_hw_source_t source) {
    int err;
    
    err = ioctl(ctx->handle, PCIDRIVER_IOC_CLEAR_IOQ, source);
    if (err) {
	pcilib_error("PCIDRIVER_IOC_CLEAR_IOQ ioctl have failed");
	return PCILIB_ERROR_FAILED;
    }
    
    return 0;
}


