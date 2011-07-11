#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "pcilib.h"

#include "pci.h"
#include "error.h"
#include "tools.h"

#include "nwl.h"
#include "nwl_defines.h"

int dma_nwl_enable_irq(nwl_dma_t *ctx, pcilib_irq_type_t type) {
    uint32_t val;
    
    if (ctx->irq_enabled == type) return 0;

    nwl_read_register(val, ctx, ctx->base_addr, REG_DMA_CTRL_STATUS);
    val &= ~(DMA_INT_ENABLE|DMA_USER_INT_ENABLE);
    nwl_write_register(val, ctx, ctx->base_addr, REG_DMA_CTRL_STATUS);
    
    pcilib_clear_irq(ctx->pcilib, NWL_DMA_IRQ_SOURCE);

    if (type & PCILIB_DMA_IRQ) val |= DMA_INT_ENABLE;
    if (type & PCILIB_EVENT_IRQ) val |= DMA_USER_INT_ENABLE;
    nwl_write_register(val, ctx, ctx->base_addr, REG_DMA_CTRL_STATUS);
    
    ctx->irq_enabled = type;

    return 0;
}

int dma_nwl_disable_irq(nwl_dma_t *ctx) {
    uint32_t val;
    
    ctx->irq_enabled = 0;
    
    nwl_read_register(val, ctx, ctx->base_addr, REG_DMA_CTRL_STATUS);
    val &= ~(DMA_INT_ENABLE|DMA_USER_INT_ENABLE);
    nwl_write_register(val, ctx, ctx->base_addr, REG_DMA_CTRL_STATUS);
    
    return 0;
}

int dma_nwl_enable_engine_irq(nwl_dma_t *ctx, pcilib_dma_engine_t dma) {
    uint32_t val;
    
    dma_nwl_enable_irq(ctx, ctx->irq_enabled|PCILIB_DMA_IRQ);

    nwl_read_register(val, ctx, ctx->engines[dma].base_addr, REG_DMA_ENG_CTRL_STATUS);
    val |= (DMA_ENG_INT_ENABLE);
    nwl_write_register(val, ctx, ctx->engines[dma].base_addr, REG_DMA_ENG_CTRL_STATUS);
    
    return 0;
}

int dma_nwl_disable_engine_irq(nwl_dma_t *ctx, pcilib_dma_engine_t dma) {
    uint32_t val;

    nwl_read_register(val, ctx, ctx->engines[dma].base_addr, REG_DMA_ENG_CTRL_STATUS);
    val &= ~(DMA_ENG_INT_ENABLE);
    nwl_write_register(val, ctx, ctx->engines[dma].base_addr, REG_DMA_ENG_CTRL_STATUS);

    return 0;
}


// ACK
