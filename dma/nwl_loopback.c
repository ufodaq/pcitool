#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "pci.h"
#include "pcilib.h"
#include "error.h"
#include "tools.h"
#include "nwl.h"

#include "nwl_defines.h"


int dma_nwl_start_loopback(nwl_dma_t *ctx,  pcilib_dma_direction_t direction, size_t packet_size) {
    uint32_t val;

	// Re-initializing always
    
    val = packet_size;
    nwl_write_register(val, ctx, ctx->base_addr, PKT_SIZE_ADDRESS);

    switch (direction) {
      case PCILIB_DMA_BIDIRECTIONAL:
	val = LOOPBACK;
	break;
      case PCILIB_DMA_TO_DEVICE:
	return -1;
      case PCILIB_DMA_FROM_DEVICE:
        val = PKTGENR;
	break;
    }

    nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
    nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);

    ctx->loopback_started = 1;
        
    return 0;
}

int dma_nwl_stop_loopback(nwl_dma_t *ctx) {
    uint32_t val = 0;

	/* Stop in any case, otherwise we can have problems in benchmark due to
	engine initialized in previous run, and benchmark is only actual usage.
	Otherwise, we should detect current loopback status during initialization */
    nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
    nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);
    ctx->loopback_started = 0;
    
    return 0;
}
