#define _PCILIB_DMA_NWL_C
#define _BSD_SOURCE

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
    
    return 0;
}

int dma_nwl_stop_loopback(nwl_dma_t *ctx) {
    uint32_t val;

    val = 0;
    nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
    nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);
    
    return 0;
}


int dma_nwl_start(nwl_dma_t *ctx) {
    if (ctx->started) return 0;
    
#ifdef NWL_GENERATE_DMA_IRQ
    dma_nwl_enable_irq(ctx, PCILIB_DMA_IRQ, 0);
#endif /* NWL_GENERATE_DMA_IRQ */

    ctx->started = 1;

    return 0;
}

int dma_nwl_stop(nwl_dma_t *ctx) {
    int err;

    pcilib_dma_engine_t i;
    
    ctx->started = 0;
    
    err = dma_nwl_free_irq(ctx);
    if (err) return err;
    
    err = dma_nwl_stop_loopback(ctx);
    if (err) return err;
    
    for (i = 0; i < ctx->n_engines; i++) {
	err = dma_nwl_stop_engine(ctx, i);
	if (err) return err;
    }
    
    return 0;
}


pcilib_dma_context_t *dma_nwl_init(pcilib_t *pcilib) {
    int i;
    int err;
    uint32_t val;
    pcilib_dma_engine_t n_engines;

    pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);
    
    nwl_dma_t *ctx = malloc(sizeof(nwl_dma_t));
    if (ctx) {
	memset(ctx, 0, sizeof(nwl_dma_t));
	ctx->pcilib = pcilib;

	pcilib_register_bank_t dma_bank = pcilib_find_bank_by_addr(pcilib, PCILIB_REGISTER_BANK_DMA);
	if (dma_bank == PCILIB_REGISTER_BANK_INVALID) {
	    free(ctx);
	    pcilib_error("DMA Register Bank could not be found");
	    return NULL;
	}
	
	ctx->dma_bank = model_info->banks + dma_bank;
	ctx->base_addr = pcilib_resolve_register_address(pcilib, ctx->dma_bank->bar, ctx->dma_bank->read_addr);

	for (i = 0, n_engines = 0; i < 2 * PCILIB_MAX_DMA_ENGINES; i++) {
	    char *addr = ctx->base_addr + DMA_OFFSET + i * DMA_ENGINE_PER_SIZE;

	    memset(ctx->engines + n_engines, 0, sizeof(pcilib_nwl_engine_description_t));

	    err = dma_nwl_read_engine_config(ctx, ctx->engines + n_engines, addr);
	    if (err) continue;
	    
	    pcilib_set_dma_engine_description(pcilib, n_engines, (pcilib_dma_engine_description_t*)(ctx->engines + n_engines));
	    ++n_engines;
	}
	pcilib_set_dma_engine_description(pcilib, n_engines, NULL);
	
	ctx->n_engines = n_engines;
	
	err = nwl_add_registers(ctx);
	if (err) {
	    free(ctx);
	    pcilib_error("Failed to add DMA registers");
	    return NULL;
	}
    }
    return (pcilib_dma_context_t*)ctx;
}

void  dma_nwl_free(pcilib_dma_context_t *vctx) {
    pcilib_dma_engine_t i;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;
    if (ctx) {
	for (i = 0; i < ctx->n_engines; i++) dma_nwl_stop_engine(ctx, i);
	dma_nwl_stop(ctx);
	free(ctx);
    }
}


double dma_nwl_benchmark(pcilib_dma_context_t *vctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction) {
    int i;
    int res;
    int err;
    size_t bytes;
    uint32_t val;
    uint32_t *buf, *cmp;
    const char *error = NULL;
//    pcilib_register_value_t regval;

    size_t us = 0;
    struct timeval start, cur;

    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    pcilib_dma_engine_t readid = pcilib_find_dma_by_addr(ctx->pcilib, PCILIB_DMA_FROM_DEVICE, dma);
    pcilib_dma_engine_t writeid = pcilib_find_dma_by_addr(ctx->pcilib, PCILIB_DMA_TO_DEVICE, dma);

    char *read_base = ctx->engines[readid].base_addr;
    char *write_base = ctx->engines[writeid].base_addr;

    if (size%sizeof(uint32_t)) size = 1 + size / sizeof(uint32_t);
    else size /= sizeof(uint32_t);

	// Not supported
    if (direction == PCILIB_DMA_TO_DEVICE) return -1.;

	// Stop Generators and drain old data
    dma_nwl_stop_loopback(ctx);
//    dma_nwl_stop_engine(ctx, readid); // DS: replace with something better

    __sync_synchronize();

    pcilib_skip_dma(ctx->pcilib, readid);

#ifdef NWL_GENERATE_DMA_IRQ
    dma_nwl_enable_engine_irq(ctx, readid);
    dma_nwl_enable_engine_irq(ctx, writeid);
#endif /* NWL_GENERATE_DMA_IRQ */


    dma_nwl_start_loopback(ctx, direction, size * sizeof(uint32_t));

/*
    printf("Packet size: %li\n", size * sizeof(uint32_t));
    pcilib_read_register(ctx->pcilib, NULL, "dma1w_counter", &regval);
    printf("Count write: %lx\n", regval);

    nwl_read_register(val, ctx, read_base, REG_DMA_ENG_CTRL_STATUS);
    printf("Read DMA control: %lx\n", val);    
    nwl_read_register(val, ctx, write_base, REG_DMA_ENG_CTRL_STATUS);
    printf("Write DMA control: %lx\n", val);    

    nwl_read_register(val, ctx, write_base, REG_DMA_ENG_NEXT_BD);
    printf("Pointer1: %lx\n", val);    
    nwl_read_register(val, ctx, write_base, REG_SW_NEXT_BD);
    printf("Pointer2: %lx\n", val);    
*/

	// Allocate memory and prepare data
    buf = malloc(size * sizeof(uint32_t));
    cmp = malloc(size * sizeof(uint32_t));
    if ((!buf)||(!cmp)) {
	if (buf) free(buf);
	if (cmp) free(cmp);
	return -1;
    }

    memset(cmp, 0x13, size * sizeof(uint32_t));

	// Benchmark
    for (i = 0; i < iterations; i++) {
        gettimeofday(&start, NULL);
	if (direction&PCILIB_DMA_TO_DEVICE) {
	    memcpy(buf, cmp, size * sizeof(uint32_t));

	    err = pcilib_write_dma(ctx->pcilib, writeid, addr, size * sizeof(uint32_t), buf, &bytes);
	    if ((err)||(bytes != size * sizeof(uint32_t))) {
		error = "Write failed";
	        break;
	    }
	}
/*
    printf("RegRead: %i\n",pcilib_read_register(ctx->pcilib, NULL, "dma1w_counter", &regval));
    printf("Count write (%i of %i): %lx\n", i, iterations, regval);

    printf("RegRead: %i\n",pcilib_read_register(ctx->pcilib, NULL, "dma1r_counter", &regval));
    printf("Count read (%i of %i): %lx\n", i, iterations, regval);


    nwl_read_register(val, ctx, read_base, REG_DMA_ENG_COMP_BYTES);
    printf("Compl Bytes (read): %lx\n", val);    

    nwl_read_register(val, ctx, write_base, REG_DMA_ENG_COMP_BYTES);
    printf("Compl Bytes (write): %lx\n", val);    

    nwl_read_register(val, ctx, read_base, REG_DMA_ENG_CTRL_STATUS);
    printf("Read DMA control (after write): %lx\n", val);    
*/
/*
    nwl_read_register(val, ctx, read_base, REG_DMA_ENG_CTRL_STATUS);
    printf("Read DMA control (after write): %lx\n", val);    
    nwl_read_register(val, ctx, write_base, REG_DMA_ENG_CTRL_STATUS);
    printf("Write DMA control (after write): %lx\n", val);    
*/
	memset(buf, 0, size * sizeof(uint32_t));
        
	err = pcilib_read_dma(ctx->pcilib, readid, addr, size * sizeof(uint32_t), buf, &bytes);
        gettimeofday(&cur, NULL);
	us += ((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec));

	if ((err)||(bytes != size * sizeof(uint32_t))) {
/*
    nwl_read_register(val, ctx, read_base, REG_DMA_ENG_CTRL_STATUS);
    printf("Read DMA control: %lx\n", val);    
    nwl_read_register(val, ctx, write_base, REG_DMA_ENG_CTRL_STATUS);
    printf("Write DMA control: %lx\n", val);    
    nwl_read_register(val, ctx, write_base, REG_DMA_ENG_NEXT_BD);
    printf("After Pointer wr1: %lx\n", val);    
    nwl_read_register(val, ctx, write_base, REG_SW_NEXT_BD);
    printf("After Pointer wr2: %lx\n", val);    
    pcilib_read_register(ctx->pcilib, NULL, "end_address", &regval);
    printf("End address: %lx\n", regval);

	nwl_read_register(val, ctx, read_base, REG_DMA_ENG_NEXT_BD);
	printf("After Pointer read1: %lx\n", val);    
	nwl_read_register(val, ctx, read_base, REG_SW_NEXT_BD);
	printf("After Pointer read2: %lx\n", val);    
*/
	     error = "Read failed";
	     break;
	}
	
	if (direction == PCILIB_DMA_BIDIRECTIONAL) {
	    res = memcmp(buf, cmp, size * sizeof(uint32_t));
	    if (res) {
		error = "Written and read values does not match";
		break;
	    }
	}
	     
    }
    
    if (error) {
	pcilib_warning("%s at iteration %i, error: %i, bytes: %zu", error, i, err, bytes);
    }
    
/*
    puts("Finished...");
    nwl_read_register(val, ctx, read_base, REG_DMA_ENG_NEXT_BD);
    printf("After Pointer read1: %lx\n", val);    
    nwl_read_register(val, ctx, read_base, REG_SW_NEXT_BD);
    printf("After Pointer read2: %lx\n", val);    

    nwl_read_register(val, ctx, write_base, REG_DMA_ENG_NEXT_BD);
    printf("After Pointer wr1: %lx\n", val);    
    nwl_read_register(val, ctx, write_base, REG_SW_NEXT_BD);
    printf("After Pointer wr2: %lx\n", val);    
*/

#ifdef NWL_GENERATE_DMA_IRQ
    dma_nwl_disable_engine_irq(ctx, writeid);
    dma_nwl_disable_engine_irq(ctx, readid);
#endif /* NWL_GENERATE_DMA_IRQ */

    dma_nwl_stop_loopback(ctx);

    __sync_synchronize();
    
    if (direction == PCILIB_DMA_FROM_DEVICE) {
	pcilib_skip_dma(ctx->pcilib, readid);
    }
    
    free(cmp);
    free(buf);

    return error?-1:(1. * size * sizeof(uint32_t) * iterations * 1000000) / (1024. * 1024. * us);
}
