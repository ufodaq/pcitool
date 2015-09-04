#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include "pcilib.h"
#include "pci.h"
#include "tools.h"
#include "error.h"
#include "model.h"
#include "plugin.h"
#include "bar.h"

int pcilib_read_fifo(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t fifo_size, size_t n, void *buf) {
    int i;
    void *data;

    pcilib_detect_address(ctx, &bar, &addr, fifo_size);
    data = pcilib_map_bar(ctx, bar);

    for (i = 0; i < n; i++) {
	pcilib_memcpy(buf + i * fifo_size, data + addr, fifo_size);
    }

    pcilib_unmap_bar(ctx, bar, data);

    return 0;
}

int pcilib_write_fifo(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t fifo_size, size_t n, void *buf) {
    int i;
    void *data;

    pcilib_detect_address(ctx, &bar, &addr, fifo_size);
    data = pcilib_map_bar(ctx, bar);

    for (i = 0; i < n; i++) {
	pcilib_memcpy(data + addr, buf + i * fifo_size, fifo_size);
    }

    pcilib_unmap_bar(ctx, bar, data);

    return 0;
}
