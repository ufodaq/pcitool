//#define PCILIB_FILE_IO
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

static int pcilib_detect_model(pcilib_t *ctx, const char *model) {
    int i, j;

    const pcilib_model_description_t *model_info = NULL;
    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);

    model_info = pcilib_find_plugin_model(ctx, board_info->vendor_id, board_info->device_id, model);
    if (model_info) {
	memcpy(&ctx->model_info, model_info, sizeof(pcilib_model_description_t));
	memcpy(&ctx->dma, model_info->dma, sizeof(pcilib_dma_description_t));
	ctx->model = strdup(model_info->name);
    } else if (model) {
	    // If not found, check for DMA models
	for (i = 0; pcilib_dma[i].name; i++) {
	    if (!strcasecmp(model, pcilib_dma[i].name))
		break;
	}

	if (pcilib_dma[i].api) {
	    model_info = &ctx->model_info;
	    memcpy(&ctx->dma, &pcilib_dma[i], sizeof(pcilib_dma_description_t));
	    ctx->model_info.dma = &ctx->dma;
	}
    }

	    // Precedens of register configuration: DMA/Event Initialization (top), XML, Event Description, DMA Description (least)
    if (model_info) {
	const pcilib_dma_description_t *dma = model_info->dma;

	if (dma) {
	    if (dma->banks)
		pcilib_add_register_banks(ctx, 0, dma->banks);

	    if (dma->registers)
		pcilib_add_registers(ctx, 0, dma->registers);

	    if (dma->engines) {
		for (j = 0; dma->engines[j].addr_bits; j++);
		memcpy(ctx->engines, dma->engines, j * sizeof(pcilib_dma_engine_description_t));
		ctx->num_engines = j;
	    } else
		ctx->dma.engines = ctx->engines;
	}

	if (model_info->protocols)
	    pcilib_add_register_protocols(ctx, 0, model_info->protocols);
		
	if (model_info->banks)
	    pcilib_add_register_banks(ctx, 0, model_info->banks);

	if (model_info->registers)
	    pcilib_add_registers(ctx, 0, model_info->registers);
		
	if (model_info->ranges)
	    pcilib_add_register_ranges(ctx, 0, model_info->ranges);
    }

    // Load XML registers

    // Check for all installed models
    // memcpy(&ctx->model_info, model, sizeof(pcilib_model_description_t));
    // how we reconcile the banks from event model and dma description? The banks specified in the DMA description should override corresponding banks of events...


    if ((model)&&(!model_info)/*&&(no xml)*/)
	return PCILIB_ERROR_NOTFOUND;

    return 0;
}



pcilib_t *pcilib_open(const char *device, const char *model) {
    int err;
    size_t i;
    pcilib_t *ctx = malloc(sizeof(pcilib_t));

    if (ctx) {
	memset(ctx, 0, sizeof(pcilib_t));
	
	ctx->handle = open(device, O_RDWR);
	if (ctx->handle < 0) {
	    pcilib_error("Error opening device (%s)", device);
	    free(ctx);
	    return NULL;
	}
	
	ctx->page_mask = (uintptr_t)-1;

	ctx->alloc_reg = PCILIB_DEFAULT_REGISTER_SPACE;
	ctx->registers = (pcilib_register_description_t *)malloc(PCILIB_DEFAULT_REGISTER_SPACE * sizeof(pcilib_register_description_t));
	ctx->register_ctx = (pcilib_register_context_t *)malloc(PCILIB_DEFAULT_REGISTER_SPACE * sizeof(pcilib_register_context_t));
	
	if ((!ctx->registers)||(!ctx->register_ctx)) {
	    pcilib_error("Error allocating memory for register model");
	    pcilib_close(ctx);
	    return NULL;
	}
	
	memset(ctx->registers, 0, sizeof(pcilib_register_description_t));
	memset(ctx->banks, 0, sizeof(pcilib_register_bank_description_t));
	memset(ctx->ranges, 0, sizeof(pcilib_register_range_t));

	memset(ctx->register_ctx, 0, PCILIB_DEFAULT_REGISTER_SPACE * sizeof(pcilib_register_context_t));

	
	for (i = 0; pcilib_protocols[i].api; i++);
	memcpy(ctx->protocols, pcilib_protocols, i * sizeof(pcilib_register_protocol_description_t));
	ctx->num_protocols = i;

	err = pcilib_detect_model(ctx, model);
	if (err) {
	    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
	    if (board_info)
	        pcilib_error("Error (%i) configuring model %s (%x:%x)", err, (model?model:""), board_info->vendor_id, board_info->device_id);
	    else
	        pcilib_error("Error (%i) configuring model %s", err, (model?model:""));
	    pcilib_close(ctx);
	    return NULL;
	}

	if (!ctx->model)
	    ctx->model = strdup(model?model:"pci");

	ctx->model_info.registers = ctx->registers;
	ctx->model_info.banks = ctx->banks;
	ctx->model_info.protocols = ctx->protocols;
	ctx->model_info.ranges = ctx->ranges;


	err = pcilib_init_register_banks(ctx);
	if (err) {
	    pcilib_error("Error (%i) initializing regiser banks\n", err);
	    pcilib_close(ctx);
	    return NULL;
	}
	
	err = pcilib_init_event_engine(ctx);
	if (err) {
	    pcilib_error("Error (%i) initializing event engine\n", err);
	    pcilib_close(ctx);
	    return NULL;
	}
    }

    return ctx;
}


const pcilib_board_info_t *pcilib_get_board_info(pcilib_t *ctx) {
    int ret;
    
    if (ctx->page_mask ==  (uintptr_t)-1) {
	ret = ioctl( ctx->handle, PCIDRIVER_IOC_PCI_INFO, &ctx->board_info );
	if (ret) {
	    pcilib_error("PCIDRIVER_IOC_PCI_INFO ioctl have failed");
	    return NULL;
	}
	
	ctx->page_mask = pcilib_get_page_mask();
    }
    
    return &ctx->board_info;
}


pcilib_context_t *pcilib_get_implementation_context(pcilib_t *ctx) {
    return ctx->event_ctx;
}


static pcilib_bar_t pcilib_detect_bar(pcilib_t *ctx, uintptr_t addr, size_t size) {
    int n = 0;
    pcilib_bar_t i;
	
    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
    if (!board_info) return PCILIB_BAR_INVALID;
		
    for (i = 0; i < PCILIB_MAX_BARS; i++) {
	if (board_info->bar_length[i] > 0) {
	    if ((addr >= board_info->bar_start[i])&&((board_info->bar_start[i] + board_info->bar_length[i]) >= (addr + size))) return i;

	    if (n) n = -1;
	    else n = i + 1;
	}
    }

    if (n > 0) return n - 1;

    return PCILIB_BAR_INVALID;
}

int pcilib_detect_address(pcilib_t *ctx, pcilib_bar_t *bar, uintptr_t *addr, size_t size) {
    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
    if (!board_info) return PCILIB_ERROR_NOTFOUND;
    
    if (*bar == PCILIB_BAR_DETECT) {
	*bar = pcilib_detect_bar(ctx, *addr, size);
	if (*bar == PCILIB_BAR_INVALID) {
	    pcilib_error("The requested data block at address 0x%x with size %zu does not belongs to any available memory bank", *addr, size);
	    return PCILIB_ERROR_NOTFOUND;
	}
	if (*addr < board_info->bar_start[*bar]) 
	    *addr += board_info->bar_start[*bar];
    } else {
	if ((*addr < board_info->bar_start[*bar])||((board_info->bar_start[*bar] + board_info->bar_length[*bar]) < (((uintptr_t)*addr) + size))) {
	    if ((board_info->bar_length[*bar]) >= (((uintptr_t)*addr) + size)) {
		*addr += board_info->bar_start[*bar];
	    } else {
		pcilib_error("The requested data block at address 0x%x with size %zu does not belong the specified memory bank (Bar %i: starting at 0x%x with size 0x%x)", *addr, size, *bar, board_info->bar_start[*bar], board_info->bar_length[*bar]);
		return PCILIB_ERROR_NOTFOUND;
	    }
	}
    }

    *addr -= board_info->bar_start[*bar];
    *addr += board_info->bar_start[*bar] & ctx->page_mask;

    return 0;
}

void *pcilib_map_bar(pcilib_t *ctx, pcilib_bar_t bar) {
    void *res;
    int ret; 

    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
    if (!board_info) return NULL;
    
    if (ctx->bar_space[bar]) return ctx->bar_space[bar];
    
    ret = ioctl( ctx->handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_PCI );
    if (ret) {
	pcilib_error("PCIDRIVER_IOC_MMAP_MODE ioctl have failed", bar);
	return NULL;
    }

    ret = ioctl( ctx->handle, PCIDRIVER_IOC_MMAP_AREA, PCIDRIVER_BAR0 + bar );
    if (ret) {
	pcilib_error("PCIDRIVER_IOC_MMAP_AREA ioctl have failed for bank %i", bar);
	return NULL;
    }

#ifdef PCILIB_FILE_IO
    file_io_handle = open("/root/data", O_RDWR);
    res = mmap( 0, board_info->bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, ctx->file_io_handle, 0 );
#else
    res = mmap( 0, board_info->bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, ctx->handle, 0 );
#endif
    if ((!res)||(res == MAP_FAILED)) {
	pcilib_error("Failed to mmap data bank %i", bar);
	return NULL;
    }

    
    return res;
}

void pcilib_unmap_bar(pcilib_t *ctx, pcilib_bar_t bar, void *data) {
    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
    if (!board_info) return;

    if (ctx->bar_space[bar]) return;
    
    munmap(data, board_info->bar_length[bar]);
#ifdef PCILIB_FILE_IO
    close(ctx->file_io_handle);
#endif
}

int pcilib_map_register_space(pcilib_t *ctx) {
    int err;
    pcilib_register_bank_t i;
    
    if (!ctx->reg_bar_mapped)  {
	const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
	const pcilib_register_bank_description_t *banks = model_info->banks;
    
	for (i = 0; ((banks)&&(banks[i].access)); i++) {
//	    uint32_t buf[2];
	    void *reg_space;
            pcilib_bar_t bar = banks[i].bar;

	    if (bar == PCILIB_BAR_DETECT) {
		uintptr_t addr = banks[0].read_addr;
	    
		err = pcilib_detect_address(ctx, &bar, &addr, 1);
		if (err) return err;
		
		if (!ctx->bar_space[bar]) {
		    reg_space = pcilib_map_bar(ctx, bar);
//		    pcilib_memcpy(&buf, reg_space, 8);
	    
		    if (reg_space) {
			ctx->bar_space[bar] = reg_space;
		    } else {
			return PCILIB_ERROR_FAILED;
		    }
		}
	    } else if (!ctx->bar_space[bar]) {
		reg_space = pcilib_map_bar(ctx, bar);
		if (reg_space) {
		    ctx->bar_space[bar] = reg_space;
		} else {
		    return PCILIB_ERROR_FAILED;
		}
//		pcilib_memcpy(&buf, reg_space, 8);
		
	    }
	    
	    if (!i) ctx->reg_bar = bar;
	}
	
	
	ctx->reg_bar_mapped = 1;
    }
    
    return 0;
}

int pcilib_map_data_space(pcilib_t *ctx, uintptr_t addr) {
    int err;
    pcilib_bar_t i;
    
    if (!ctx->data_bar_mapped) {
        const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
	if (!board_info) return PCILIB_ERROR_FAILED;

	err = pcilib_map_register_space(ctx);
	if (err) {
	    pcilib_error("Error mapping register space");
	    return err;
	}
	
	int data_bar = -1;	
	
	for (i = 0; i < PCILIB_MAX_BARS; i++) {
	    if ((ctx->bar_space[i])||(!board_info->bar_length[i])) continue;
	    
	    if (addr) {
	        if (board_info->bar_start[i] == addr) {
		    data_bar = i;
		    break;
		}
	    } else {
		if (data_bar >= 0) {
		    data_bar = -1;
		    break;
		}
		
		data_bar = i;
	    }
	}
	    

	if (data_bar < 0) {
	    if (addr) pcilib_error("Unable to find the specified data space (%lx)", addr);
	    else pcilib_error("Unable to find the data space");
	    return PCILIB_ERROR_NOTFOUND;
	}
	
	ctx->data_bar = data_bar;
	
	if (!ctx->bar_space[data_bar]) {
	    char *data_space = pcilib_map_bar(ctx, data_bar);
	    if (data_space) ctx->bar_space[data_bar] = data_space;
	    else {
	        pcilib_error("Unable to map the data space");
		return PCILIB_ERROR_FAILED;
	    }
	}
	
	ctx->data_bar_mapped = 0;
    }
    
    return 0;
}
	
char  *pcilib_resolve_register_address(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr) {
    if (bar == PCILIB_BAR_DETECT) {
	    // First checking the default register bar
	size_t offset = addr - ctx->board_info.bar_start[ctx->reg_bar];
	if ((addr > ctx->board_info.bar_start[ctx->reg_bar])&&(offset < ctx->board_info.bar_length[ctx->reg_bar])) {
	    if (!ctx->bar_space[ctx->reg_bar]) {
		pcilib_error("The register bar is not mapped");
		return NULL;
	    }

	    return ctx->bar_space[ctx->reg_bar] + offset + (ctx->board_info.bar_start[ctx->reg_bar] & ctx->page_mask);
	}
	    
	    // Otherwise trying to detect
	bar = pcilib_detect_bar(ctx, addr, 1);
	if (bar != PCILIB_BAR_INVALID) {
	    size_t offset = addr - ctx->board_info.bar_start[bar];
	    if ((offset < ctx->board_info.bar_length[bar])&&(ctx->bar_space[bar])) {
		if (!ctx->bar_space[bar]) {
		    pcilib_error("The requested bar (%i) is not mapped", bar);
		    return NULL;
		}
		return ctx->bar_space[bar] + offset + (ctx->board_info.bar_start[bar] & ctx->page_mask);
	    }
	}
    } else {
	if (!ctx->bar_space[bar]) {
	    pcilib_error("The requested bar (%i) is not mapped", bar);
	    return NULL;
	}
	
	if (addr < ctx->board_info.bar_length[bar]) {
	    return ctx->bar_space[bar] + addr + (ctx->board_info.bar_start[bar] & ctx->page_mask);
	}
	
	if ((addr >= ctx->board_info.bar_start[bar])&&(addr < (ctx->board_info.bar_start[bar] + ctx->board_info.bar_length[ctx->reg_bar]))) {
	    return ctx->bar_space[bar] + (addr - ctx->board_info.bar_start[bar]) + (ctx->board_info.bar_start[bar] & ctx->page_mask);
	}
    }

    return NULL;
}

char *pcilib_resolve_data_space(pcilib_t *ctx, uintptr_t addr, size_t *size) {
    int err;
    
    err = pcilib_map_data_space(ctx, addr);
    if (err) {
	pcilib_error("Failed to map the specified address space (%lx)", addr);
	return NULL;
    }
    
    if (size) *size = ctx->board_info.bar_length[ctx->data_bar];
    
    return ctx->bar_space[ctx->data_bar] + (ctx->board_info.bar_start[ctx->data_bar] & ctx->page_mask);
}


void pcilib_close(pcilib_t *ctx) {
    pcilib_bar_t i;

    if (ctx) {
	const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
	const pcilib_event_api_description_t *eapi = model_info->api;
	const pcilib_dma_api_description_t *dapi = ctx->dma.api;
	
        if ((eapi)&&(eapi->free)) eapi->free(ctx->event_ctx);
        if ((dapi)&&(dapi->free)) dapi->free(ctx->dma_ctx);

	pcilib_free_register_banks(ctx);
	
	if (ctx->register_ctx)
	    free(ctx->register_ctx);

	if (ctx->event_plugin)
	    pcilib_plugin_close(ctx->event_plugin);
	
	if (ctx->kmem_list) {
	    pcilib_warning("Not all kernel buffers are properly cleaned");
	
	    while (ctx->kmem_list) {
		pcilib_free_kernel_memory(ctx, ctx->kmem_list, 0);
	    }
	}
	
	for (i = 0; i < PCILIB_MAX_BARS; i++) {
	    if (ctx->bar_space[i]) {
		char *ptr = ctx->bar_space[i];
		ctx->bar_space[i] = NULL;
		pcilib_unmap_bar(ctx, i, ptr);
	    }
	}

	if (ctx->registers)
	    free(ctx->registers);
	
	if (ctx->model)
	    free(ctx->model);
	
	if (ctx->handle >= 0)
	    close(ctx->handle);
	
	free(ctx);
    }
}

int pcilib_read(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf) {
    void *data;

    pcilib_detect_address(ctx, &bar, &addr, size);
    data = pcilib_map_bar(ctx, bar);

    pcilib_memcpy(buf, data + addr, size);

    pcilib_unmap_bar(ctx, bar, data);

    return 0;
}

int pcilib_write(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf) {
    void *data;

    pcilib_detect_address(ctx, &bar, &addr, size);
    data = pcilib_map_bar(ctx, bar);

    pcilib_memcpy(data + addr, buf, size);

    pcilib_unmap_bar(ctx, bar, data);

    return 0;
}


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
