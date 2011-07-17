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

int dma_nwl_init_irq(nwl_dma_t *ctx, uint32_t val) {
    if (val&(DMA_INT_ENABLE|DMA_USER_INT_ENABLE)) {
	if (val&DMA_INT_ENABLE) ctx->irq_preserve |= PCILIB_DMA_IRQ;
	if (val&DMA_USER_INT_ENABLE) ctx->irq_preserve |= PCILIB_EVENT_IRQ;
    }
    
    ctx->irq_started = 1;
    return 0;
}

int dma_nwl_free_irq(nwl_dma_t *ctx) {
    if (ctx->irq_started) {
	dma_nwl_disable_irq((pcilib_dma_context_t*)ctx, 0);
	if (ctx->irq_preserve) dma_nwl_enable_irq((pcilib_dma_context_t*)ctx, ctx->irq_preserve, 0);
	ctx->irq_enabled = 0;
	ctx->irq_started = 0;
    }
    return 0;
}

int dma_nwl_enable_irq(pcilib_dma_context_t *vctx, pcilib_irq_type_t type, pcilib_dma_flags_t flags) {
    uint32_t val;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;
    
    if (flags&PCILIB_DMA_FLAG_PERSISTENT) ctx->irq_preserve |= type;

    if ((ctx->irq_enabled&type) == type) return 0;
    
    type |= ctx->irq_enabled;
    
    nwl_read_register(val, ctx, ctx->base_addr, REG_DMA_CTRL_STATUS);
    if (!ctx->irq_started) dma_nwl_init_irq(ctx, val);

    val &= ~(DMA_INT_ENABLE|DMA_USER_INT_ENABLE);
    nwl_write_register(val, ctx, ctx->base_addr, REG_DMA_CTRL_STATUS);
    
    pcilib_clear_irq(ctx->pcilib, NWL_DMA_IRQ_SOURCE);

    if (type & PCILIB_DMA_IRQ) val |= DMA_INT_ENABLE;
    if (type & PCILIB_EVENT_IRQ) val |= DMA_USER_INT_ENABLE;
    nwl_write_register(val, ctx, ctx->base_addr, REG_DMA_CTRL_STATUS);
    
    ctx->irq_enabled = type;

    return 0;
}


int dma_nwl_disable_irq(pcilib_dma_context_t *vctx, pcilib_dma_flags_t flags) {
    uint32_t val;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;
    
    ctx->irq_enabled = 0;
    
    nwl_read_register(val, ctx, ctx->base_addr, REG_DMA_CTRL_STATUS);
    if (!ctx->irq_started) dma_nwl_init_irq(ctx, val);
    val &= ~(DMA_INT_ENABLE|DMA_USER_INT_ENABLE);
    nwl_write_register(val, ctx, ctx->base_addr, REG_DMA_CTRL_STATUS);
    
    if (flags&PCILIB_DMA_FLAG_PERSISTENT) ctx->irq_preserve = 0;

    return 0;
}


int dma_nwl_enable_engine_irq(nwl_dma_t *ctx, pcilib_dma_engine_t dma) {
    uint32_t val;
    
    dma_nwl_enable_irq(ctx, PCILIB_DMA_IRQ, 0);

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
