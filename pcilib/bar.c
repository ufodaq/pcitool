#define _BSD_SOURCE
#define _DEFAULT_SOURCE
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

pcilib_bar_t pcilib_detect_bar(pcilib_t *ctx, uintptr_t addr, size_t size) {
    int n = 0;
    pcilib_bar_t i = ctx->reg_bar;
	
    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
    if (!board_info) return PCILIB_BAR_INVALID;

	// First checking the default register bar
    if ((addr >= board_info->bar_start[i])&&((board_info->bar_start[i] + board_info->bar_length[i]) >= (addr + size))) return i;

	// Otherwise iterate
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
	    pcilib_error("The requested data block at address 0x%x with size %zu does not belongs to any available memory bar", *addr, size);
	    return PCILIB_ERROR_NOTFOUND;
	}
	if (*addr < board_info->bar_start[*bar]) 
	    *addr += board_info->bar_start[*bar];
    } else {
	if ((*addr < board_info->bar_start[*bar])||((board_info->bar_start[*bar] + board_info->bar_length[*bar]) < (((uintptr_t)*addr) + size))) {
	    if ((board_info->bar_length[*bar]) >= (((uintptr_t)*addr) + size)) {
		*addr += board_info->bar_start[*bar];
	    } else {
		pcilib_error("The requested data block at address 0x%x with size %zu does not belong the specified memory bar (Bar %i: starting at 0x%x with size 0x%x)", *addr, size, *bar, board_info->bar_start[*bar], board_info->bar_length[*bar]);
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
    int err, ret; 

    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
    if (!board_info) return NULL;

    if (ctx->bar_space[bar]) return ctx->bar_space[bar];

    err = pcilib_lock_global(ctx);
    if (err) {
	pcilib_error("Error (%i) acquiring mmap lock", err);
	return NULL;
    }

    ret = ioctl( ctx->handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_PCI );
    if (ret) {
	pcilib_unlock_global(ctx);
	pcilib_error("PCIDRIVER_IOC_MMAP_MODE ioctl have failed", bar);
	return NULL;
    }

    ret = ioctl( ctx->handle, PCIDRIVER_IOC_MMAP_AREA, PCIDRIVER_BAR0 + bar );
    if (ret) {
	pcilib_unlock_global(ctx);
	pcilib_error("PCIDRIVER_IOC_MMAP_AREA ioctl have failed for bank %i", bar);
	return NULL;
    }

#ifdef PCILIB_FILE_IO
    file_io_handle = open("/root/data", O_RDWR);
    res = mmap( 0, board_info->bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, ctx->file_io_handle, 0 );
#else
    res = mmap( 0, board_info->bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, ctx->handle, 0 );
#endif

    pcilib_unlock_global(ctx);

    if ((!res)||(res == MAP_FAILED)) {
	pcilib_error("Failed to mmap data bank %i", bar);
	return NULL;
    }

    ctx->bar_space[bar] = res;
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

char *pcilib_resolve_bar_address(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr) {
    if (bar == PCILIB_BAR_DETECT) {
	bar = pcilib_detect_bar(ctx, addr, 1);
	if (bar == PCILIB_BAR_INVALID) return NULL;
	
	if ((!ctx->bar_space[bar])&&(!pcilib_map_bar(ctx, bar))) {
	    pcilib_error("Failed to map the requested bar (%i)", bar);
	    return NULL;
	}

	return ctx->bar_space[bar] + (addr - ctx->board_info.bar_start[bar]) + (ctx->board_info.bar_start[bar] & ctx->page_mask);
    } else {
	if ((!ctx->bar_space[bar])&&(!pcilib_map_bar(ctx, bar))) {
	    pcilib_error("Failed to map the requested bar (%i)", bar);
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
            
            if (bar == PCILIB_BAR_NOBAR)
        	continue;

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

const pcilib_bar_info_t *pcilib_get_bar_list(pcilib_t *ctx) {
    int i, bars = 0;

    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
    if (!board_info) return NULL;

    for (i = 0; i < PCILIB_MAX_BARS; i++) {
	if (board_info->bar_length[i] > 0) {
	    ctx->bar_info[bars++] = (pcilib_bar_info_t) {
		.bar = i,
		.size = board_info->bar_length[i],
		.phys_addr =  board_info->bar_start[i],
		.virt_addr = ctx->bar_space[i]
	    };
	}
    }

    return ctx->bar_info;
}

const pcilib_bar_info_t *pcilib_get_bar_info(pcilib_t *ctx, pcilib_bar_t bar) {
    int i;

    const pcilib_bar_info_t *info = pcilib_get_bar_list(ctx);
    if (!info) return NULL;

    for (i = 0; info[i].size; i++) {
	if (info[i].bar == bar)
	    return &info[i];
    }

    return NULL;
}

int pcilib_read(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t access, size_t n, void *buf) {
    void *data;

    pcilib_detect_address(ctx, &bar, &addr, access * n);
    data = pcilib_map_bar(ctx, bar);

    pcilib_memcpy(buf, data + addr, access, n);

    pcilib_unmap_bar(ctx, bar, data);

    return 0;
}

int pcilib_write(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t access, size_t n, void *buf) {
    void *data;

    pcilib_detect_address(ctx, &bar, &addr, access * n);
    data = pcilib_map_bar(ctx, bar);

    pcilib_memcpy(data + addr, buf, access, n);

    pcilib_unmap_bar(ctx, bar, data);

    return 0;
}

