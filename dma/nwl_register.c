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
#include "nwl_register.h"

int nwl_add_registers(nwl_dma_t *ctx) {
    int err;
    size_t n, i, j;
    int length;
    const char *names[NWL_MAX_DMA_ENGINE_REGISTERS];
    uintptr_t addr[NWL_MAX_DMA_ENGINE_REGISTERS];
    
	// We don't want DMA registers
    if (pcilib_find_bank_by_addr(ctx->pcilib, PCILIB_REGISTER_BANK_DMA) == PCILIB_REGISTER_BANK_INVALID) return 0;
    
    err = pcilib_add_registers(ctx->pcilib, 0, nwl_dma_registers);
    if (err) return err;

    if (ctx->type == PCILIB_DMA_MODIFICATION_DEFAULT) {
	err = pcilib_add_registers(ctx->pcilib, 0, nwl_xrawdata_registers);
	if (err) return err;
    }

    
    for (n = 0; nwl_dma_engine_registers[n].bits; n++) {
	names[n] = nwl_dma_engine_registers[n].name;
	addr[n] = nwl_dma_engine_registers[n].addr;
    }

    if (ctx->n_engines > 9) length = 2;
    else length = 1;

    
    for (i = 0; i < ctx->n_engines; i++) {
	for (j = 0; nwl_dma_engine_registers[j].bits; j++) {
	    const char *direction;
	    nwl_dma_engine_registers[j].name = nwl_dma_engine_register_names[i * NWL_MAX_DMA_ENGINE_REGISTERS + j];
	    nwl_dma_engine_registers[j].addr = addr[j] + (ctx->engines[i].base_addr - ctx->base_addr);
//	    printf("%lx %lx\n", (ctx->engines[i].base_addr - ctx->base_addr), nwl_dma_engine_registers[j].addr);
	    
	    switch (ctx->engines[i].desc.direction) {
		case PCILIB_DMA_FROM_DEVICE:
		    direction =  "r";
		break;
		case PCILIB_DMA_TO_DEVICE:
		    direction = "w";
		break;
		default:
		    direction = "";
	    }
	    
	    sprintf((char*)nwl_dma_engine_registers[j].name, names[j], length, ctx->engines[i].desc.addr, direction);
	}
	
        err = pcilib_add_registers(ctx->pcilib, n, nwl_dma_engine_registers);
	if (err) return err;
    }
    
    for (n = 0; nwl_dma_engine_registers[n].bits; n++) {
	nwl_dma_engine_registers[n].name = names[n];
	nwl_dma_engine_registers[n].addr = addr[n];
    }
    
    return 0;
}
