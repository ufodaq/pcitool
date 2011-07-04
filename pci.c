#define _PCILIB_PCI_C
//#define PCILIB_FILE_IO
#define _POSIX_C_SOURCE 199309L

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

#include "kernel.h"
#include "tools.h"

#include "dma.h"
#include "pci.h"
#include "ipecamera/model.h"
#include "error.h"

#define BIT_MASK(bits) ((1l << (bits)) - 1)




static void pcilib_print_error(const char *msg, ...) {
    va_list va;
    
    va_start(va, msg);
    vprintf(msg, va);
    va_end(va);
    printf("\n");
}

void (*pcilib_error)(const char *msg, ...) = pcilib_print_error;
void (*pcilib_warning)(const char *msg, ...) = pcilib_print_error;

int pcilib_set_error_handler(void (*err)(const char *msg, ...), void (*warn)(const char *msg, ...)) {
    if (err) pcilib_error = err;
    else pcilib_error = pcilib_print_error;
    if (warn) pcilib_warning = warn;
    else pcilib_warning = pcilib_print_error;
}

pcilib_t *pcilib_open(const char *device, pcilib_model_t model) {
    pcilib_event_api_description_t *api;
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
	ctx->model = model;

	if (!model) model = pcilib_get_model(ctx);
	
	ctx->model_info = pcilib_model + model;
	
	api = pcilib_model[model].event_api;
        if ((api)&&(api->init)) ctx->event_ctx = api->init(ctx);
    }

    return ctx;
}

pcilib_model_description_t *pcilib_get_model_description(pcilib_t *ctx) {
    return ctx->model_info;
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


int pcilib_wait_irq(pcilib_t *ctx, pcilib_irq_source_t source, unsigned long timeout) {
    int err;

    err = ioctl(ctx->handle, PCIDRIVER_IOC_WAITI, source);
    if (err) {
	pcilib_error("PCIDRIVER_IOC_WAITI ioctl have failed");
	return PCILIB_ERROR_FAILED;
    }
    
    return 0;
}

pcilib_context_t *pcilib_get_implementation_context(pcilib_t *ctx) {
    return ctx->event_ctx;
}

pcilib_model_t pcilib_get_model(pcilib_t *ctx) {
    if (ctx->model == PCILIB_MODEL_DETECT) {
	unsigned short vendor_id;
	unsigned short device_id;

	//return PCILIB_MODEL_PCI;
	
	const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
	if (!board_info) return PCILIB_MODEL_PCI;

	if ((board_info->vendor_id == PCIE_XILINX_VENDOR_ID)&&(board_info->device_id == PCIE_IPECAMERA_DEVICE_ID))
	    ctx->model = PCILIB_MODEL_IPECAMERA;
	else
	    ctx->model = PCILIB_MODEL_PCI;
    }
    
    return ctx->model;
}

static pcilib_bar_t pcilib_detect_bar(pcilib_t *ctx, uintptr_t addr, size_t size) {
    pcilib_bar_t i;
	
    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
    if (!board_info) return PCILIB_BAR_INVALID;
		
    for (i = 0; i < PCILIB_MAX_BANKS; i++) {
	if ((addr >= board_info->bar_start[i])&&((board_info->bar_start[i] + board_info->bar_length[i]) >= (addr + size))) return i;
    }
	
    return PCILIB_BAR_INVALID;
}

static int pcilib_detect_address(pcilib_t *ctx, pcilib_bar_t *bar, uintptr_t *addr, size_t size) {
    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
    if (!board_info) return PCILIB_ERROR_NOTFOUND;
    
    if (*bar == PCILIB_BAR_DETECT) {
	*bar = pcilib_detect_bar(ctx, *addr, size);
	if (*bar < 0) {
	    pcilib_error("The requested data block at address 0x%x with size 0x%x does not belongs to any available memory bank", *addr, size);
	    return PCILIB_ERROR_NOTFOUND;
	}
    } else {
	if ((*addr < board_info->bar_start[*bar])||((board_info->bar_start[*bar] + board_info->bar_length[*bar]) < (((uintptr_t)*addr) + size))) {
	    if ((board_info->bar_length[*bar]) >= (((uintptr_t)*addr) + size)) {
		*addr += board_info->bar_start[*bar];
	    } else {
		pcilib_error("The requested data block at address 0x%x with size 0x%x does not belong the specified memory bank (Bar %i: starting at 0x%x with size 0x%x)", *addr, size, *bar, board_info->bar_start[*bar], board_info->bar_length[*bar]);
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

int pcilib_read(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf) {
    int i;
    void *data;
    unsigned int offset;
    char local_buf[size];
    

    pcilib_detect_address(ctx, &bar, &addr, size);
    data = pcilib_map_bar(ctx, bar);

/*
    for (i = 0; i < size/4; i++)  {
	((uint32_t*)((char*)data+addr))[i] = 0x100 * i + 1;
    }
*/
    pcilib_memcpy(buf, data + addr, size);
    
    pcilib_unmap_bar(ctx, bar, data);    
}

int pcilib_write(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf) {
    int i;
    void *data;
    unsigned int offset;
    char local_buf[size];
    

    pcilib_detect_address(ctx, &bar, &addr, size);
    data = pcilib_map_bar(ctx, bar);

    pcilib_memcpy(data + addr, buf, size);
    
    pcilib_unmap_bar(ctx, bar, data);    
}


pcilib_register_bank_t pcilib_find_bank_by_addr(pcilib_t *ctx, pcilib_register_bank_addr_t bank) {
    pcilib_register_bank_t i;
    pcilib_model_t model = pcilib_get_model(ctx);
    pcilib_register_bank_description_t *banks = pcilib_model[model].banks;

    for (i = 0; banks[i].access; i++)
	if (banks[i].addr == bank) return i;
	
    return -1;
}

pcilib_register_bank_t pcilib_find_bank_by_name(pcilib_t *ctx, const char *bankname) {
    pcilib_register_bank_t i;
    pcilib_register_bank_description_t *banks = pcilib_model[ctx->model].banks;

    for (i = 0; banks[i].access; i++)
	if (!strcasecmp(banks[i].name, bankname)) return i;
	
    return -1;
}

pcilib_register_bank_t pcilib_find_bank(pcilib_t *ctx, const char *bank) {
    pcilib_register_bank_t res;
    unsigned long addr;
    
    if (!bank) {
	pcilib_model_t model = pcilib_get_model(ctx);
	pcilib_register_bank_description_t *banks = pcilib_model[model].banks;
	if ((banks)&&(banks[0].access)) return (pcilib_register_bank_t)0;
	return -1;
    }
    
    if (pcilib_isxnumber(bank)&&(sscanf(bank,"%lx", &addr) == 1)) {
	res = pcilib_find_bank_by_addr(ctx, addr);
	if (res != PCILIB_REGISTER_BANK_INVALID) return res;
    }
    
    return pcilib_find_bank_by_name(ctx, bank);
}

    // FIXME create hash during map_register space
pcilib_register_t pcilib_find_register(pcilib_t *ctx, const char *bank, const char *reg) {
    pcilib_register_t i;
    pcilib_register_bank_t bank_id;
    pcilib_register_bank_addr_t bank_addr;
    
    pcilib_model_t model = pcilib_get_model(ctx);
    
    pcilib_register_description_t *registers =  pcilib_model[model].registers;
    
    if (bank) {
	bank_id = pcilib_find_bank(ctx, bank);
	if (bank_id == PCILIB_REGISTER_BANK_INVALID) {
	    pcilib_error("Invalid bank (%s) is specified", bank);
	    return -1;
	}
	
	bank_addr = pcilib_model[model].banks[bank_id].addr;
    }
    
    for (i = 0; registers[i].bits; i++) {
	if ((!strcasecmp(registers[i].name, reg))&&((!bank)||(registers[i].bank == bank_addr))) return i;
    }
    
    return (pcilib_register_t)-1;
};


pcilib_event_t pcilib_find_event(pcilib_t *ctx, const char *event) {
    int i;
    pcilib_register_bank_t res;
    unsigned long addr;
    
    pcilib_model_t model = pcilib_get_model(ctx);
    pcilib_event_description_t *events = pcilib_model[model].events;
    
    for (i = 0; events[i].name; i++) {
	if (!strcasecmp(events[i].name, event)) return (1<<i);
    }

    return (pcilib_event_t)-1;
}


int pcilib_map_register_space(pcilib_t *ctx) {
    int err;
    pcilib_register_bank_t i;
    
    if (!ctx->reg_bar_mapped)  {
	pcilib_model_t model = pcilib_get_model(ctx);
	pcilib_register_bank_description_t *banks = pcilib_model[model].banks;
    
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
	
	for (i = 0; i < PCILIB_MAX_BANKS; i++) {
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
	
/*
static void pcilib_unmap_register_space(pcilib_t *ctx) {
    if (ctx->reg_space) {
	pcilib_unmap_bar(ctx, ctx->reg_bar, ctx->reg_space);
	ctx->reg_space = NULL;
    }
}

static void pcilib_unmap_data_space(pcilib_t *ctx) {
    if (ctx->data_space) {
	pcilib_unmap_bar(ctx, ctx->data_bar, ctx->data_space);
	ctx->data_space = NULL;
    }
}
*/

char  *pcilib_resolve_register_address(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr) {
    if (bar == PCILIB_BAR_DETECT) {
	    // First checking the default register bar
	size_t offset = addr - ctx->board_info.bar_start[ctx->reg_bar];
	if ((addr > ctx->board_info.bar_start[ctx->reg_bar])&&(offset < ctx->board_info.bar_length[ctx->reg_bar])) {
	    return ctx->bar_space[ctx->reg_bar] + offset + (ctx->board_info.bar_start[ctx->reg_bar] & ctx->page_mask);
	}
	    
	    // Otherwise trying to detect
	bar = pcilib_detect_bar(ctx, addr, 1);
	if (bar != PCILIB_BAR_INVALID) {
	    size_t offset = addr - ctx->board_info.bar_start[bar];
	    if ((offset < ctx->board_info.bar_length[bar])&&(ctx->bar_space[bar])) {
		return ctx->bar_space[bar] + offset + (ctx->board_info.bar_start[bar] & ctx->page_mask);
	    }
	}
    } else {
	if (addr < ctx->board_info.bar_length[bar]) {
	    return ctx->bar_space[bar] + addr + (ctx->board_info.bar_start[bar] & ctx->page_mask);
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
	pcilib_model_t model = pcilib_get_model(ctx);
	pcilib_event_api_description_t *eapi = pcilib_model[model].event_api;
	pcilib_dma_api_description_t *dapi = pcilib_model[model].dma_api;
    
        if ((eapi)&&(eapi->free)) eapi->free(ctx->event_ctx);
        if ((dapi)&&(dapi->free)) dapi->free(ctx->dma_ctx);
	
	while (ctx->kmem_list) {
	    pcilib_free_kernel_memory(ctx, ctx->kmem_list);
	}

	for (i = 0; i < PCILIB_MAX_BANKS; i++) {
	    if (ctx->bar_space[i]) {
		char *ptr = ctx->bar_space[i];
		ctx->bar_space[i] = NULL;
		pcilib_unmap_bar(ctx, i, ptr);
	    }
	}
	close(ctx->handle);
	
	free(ctx);
    }
}

static int pcilib_read_register_space_internal(pcilib_t *ctx, pcilib_register_bank_t bank, pcilib_register_addr_t addr, size_t n, uint8_t bits, pcilib_register_value_t *buf) {
    int err;
    int rest;
    size_t i;
    
    pcilib_model_t model = pcilib_get_model(ctx);
    pcilib_register_bank_description_t *b = pcilib_model[model].banks + bank;

    assert(bits < 8 * sizeof(pcilib_register_value_t));
    
    if (((addr + n) > b->size)||(((addr + n) == b->size)&&(bits))) {
	pcilib_error("Accessing sregister (%u regs at addr %u) out of register space (%u registers total)", bits?(n+1):n, addr, b->size);
	return PCILIB_ERROR_OUTOFRANGE;
    }

    err = pcilib_map_register_space(ctx);
    if (err) {
	pcilib_error("Failed to map the register space");
	return err;
    }
    
    //n += bits / b->access;
    //bits %= b->access; 
    
    for (i = 0; i < n; i++) {
	err = pcilib_protocol[b->protocol].read(ctx, b, addr + i, b->access, buf + i);
	if (err) break;
    }
    
    if ((bits > 0)&&(!err)) err = pcilib_protocol[b->protocol].read(ctx, b, addr + n, bits, buf + n);
    
    return err;
}

int pcilib_read_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf) {
    pcilib_register_bank_t bank_id = pcilib_find_bank(ctx, bank);
    if (bank_id == PCILIB_REGISTER_BANK_INVALID) {
	if (bank) pcilib_error("Invalid register bank is specified (%s)", bank);
	else pcilib_error("Register bank should be specified");
	return PCILIB_ERROR_INVALID_BANK;
    }
    
    return pcilib_read_register_space_internal(ctx, bank_id, addr, n, 0, buf);
}

int pcilib_read_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t *value) {
    int err;
    size_t i, n, bits;
    pcilib_register_value_t res;
    pcilib_register_description_t *r;
    pcilib_register_bank_description_t *b;
    pcilib_model_t model = pcilib_get_model(ctx);

    r = pcilib_model[model].registers + reg;
    b = pcilib_model[model].banks + r->bank;
    
    n = r->bits / b->access;
    bits = r->bits % b->access; 

    pcilib_register_value_t buf[n + 1];
    err = pcilib_read_register_space_internal(ctx, r->bank, r->addr, n, bits, buf);

    if ((b->endianess == PCILIB_BIG_ENDIAN)||((b->endianess == PCILIB_HOST_ENDIAN)&&(ntohs(1) == 1))) {
	pcilib_error("Big-endian byte order support is not implemented");
	return PCILIB_ERROR_NOTSUPPORTED;
    } else {
	res = 0;
	if (bits) ++n;
	for (i = 0; i < n; i++) {
	    res |= buf[i] << (i * b->access);
	}
    }
    
    *value = res;
    
    return err;
}


int pcilib_read_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t *value) {
    int err;
    int reg;
    
    reg = pcilib_find_register(ctx, bank, regname);
    if (reg < 0) {
	pcilib_error("Register (%s) is not found", regname);
	return PCILIB_ERROR_NOTFOUND;
    }
    
    return pcilib_read_register_by_id(ctx, reg, value);

//    registers[reg].bank
//    printf("%li %li", sizeof(pcilib_model[model].banks), sizeof(pcilib_register_bank_description_t));
}


static int pcilib_write_register_space_internal(pcilib_t *ctx, pcilib_register_bank_t bank, pcilib_register_addr_t addr, size_t n, uint8_t bits, pcilib_register_value_t *buf) {
    int err;
    int rest;
    size_t i;
    
    pcilib_model_t model = pcilib_get_model(ctx);
    pcilib_register_bank_description_t *b = pcilib_model[model].banks + bank;

    assert(bits < 8 * sizeof(pcilib_register_value_t));

    if (((addr + n) > b->size)||(((addr + n) == b->size)&&(bits))) {
	pcilib_error("Accessing sregister (%u regs at addr %u) out of register space (%u registers total)", bits?(n+1):n, addr, b->size);
	return PCILIB_ERROR_OUTOFRANGE;
    }

    err = pcilib_map_register_space(ctx);
    if (err) {
	pcilib_error("Failed to map the register space");
	return err;
    }
    
    //n += bits / b->access;
    //bits %= b->access; 
    
    for (i = 0; i < n; i++) {
	err = pcilib_protocol[b->protocol].write(ctx, b, addr + i, b->access, buf[i]);
	if (err) break;
    }
    
    if ((bits > 0)&&(!err)) err = pcilib_protocol[b->protocol].write(ctx, b, addr + n, bits, buf[n]);
    
    return err;
}

int pcilib_write_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf) {
    pcilib_register_bank_t bank_id = pcilib_find_bank(ctx, bank);
    if (bank_id == PCILIB_REGISTER_BANK_INVALID) {
	if (bank) pcilib_error("Invalid register bank is specified (%s)", bank);
	else pcilib_error("Register bank should be specified");
	return PCILIB_ERROR_INVALID_BANK;
    }
    
    return pcilib_write_register_space_internal(ctx, bank_id, addr, n, 0, buf);
}


int pcilib_write_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t value) {
    int err;
    size_t i, n, bits;
    pcilib_register_value_t res;
    pcilib_register_description_t *r;
    pcilib_register_bank_description_t *b;
    pcilib_model_t model = pcilib_get_model(ctx);

    r = pcilib_model[model].registers + reg;
    b = pcilib_model[model].banks + r->bank;
    
    n = r->bits / b->access;
    bits = r->bits % b->access; 

    pcilib_register_value_t buf[n + 1];
    memset(buf, 0, (n + 1) * sizeof(pcilib_register_value_t));
    
    if ((b->endianess == PCILIB_BIG_ENDIAN)||((b->endianess == PCILIB_HOST_ENDIAN)&&(ntohs(1) == 1))) {
	pcilib_error("Big-endian byte order support is not implemented");
	return PCILIB_ERROR_NOTSUPPORTED;
    } else {
	if (b->access == sizeof(pcilib_register_value_t) * 8) {
	    buf[0] = value;
	} else {
	    for (i = 0, res = value; (res > 0)&&(i <= n); ++i) {
		buf[i] = res & BIT_MASK(b->access);
	        res >>= b->access;
	    }
	
	    if (res) {
		pcilib_error("Value %i is too big to fit in the register %s", value, r->name);
		return PCILIB_ERROR_OUTOFRANGE;
	    }
	}
    }

    err = pcilib_write_register_space_internal(ctx, r->bank, r->addr, n, bits, buf);
    return err;
}

int pcilib_write_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t value) {
    int err;
    int reg;
    
    reg = pcilib_find_register(ctx, bank, regname);
    if (reg < 0) {
	pcilib_error("Register (%s) is not found", regname);
	return PCILIB_ERROR_NOTFOUND;
    }

    return pcilib_write_register_by_id(ctx, reg, value);
}


int pcilib_reset(pcilib_t *ctx) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_t model = pcilib_get_model(ctx);

    api = pcilib_model[model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }
    
    if (api->reset) 
	return api->reset(ctx->event_ctx);
	
    return 0;
}

int pcilib_start(pcilib_t *ctx, pcilib_event_t event_mask, void *callback, void *user) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_t model = pcilib_get_model(ctx);

    api = pcilib_model[model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->start) 
	return api->start(ctx->event_ctx, event_mask, callback, user);

    return 0;
}

int pcilib_stop(pcilib_t *ctx) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_t model = pcilib_get_model(ctx);

    api = pcilib_model[model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->stop) 
	return api->stop(ctx->event_ctx);

    return 0;
}

pcilib_event_id_t pcilib_get_next_event(pcilib_t *ctx, pcilib_event_t event_mask, const struct timespec *timeout) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_t model = pcilib_get_model(ctx);

    api = pcilib_model[model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->next_event) 
	return api->next_event(ctx->event_ctx, event_mask, timeout);

    pcilib_error("Event enumeration is not suppored by API");
    return PCILIB_EVENT_ID_INVALID;
}

int pcilib_trigger(pcilib_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_t model = pcilib_get_model(ctx);

    api = pcilib_model[model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->trigger) 
	return api->trigger(ctx->event_ctx, event, trigger_size, trigger_data);

    pcilib_error("Self triggering is not supported by the selected model");
    return PCILIB_ERROR_NOTSUPPORTED;
}


void *pcilib_get_data_with_argument(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size) {
    pcilib_event_api_description_t *api = pcilib_model[ctx->model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return NULL;
    }

    if (api->get_data) 
	return api->get_data(ctx->event_ctx, event_id, data_type, arg_size, arg, size);

    return NULL;
}

void *pcilib_get_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t *size) {
    pcilib_event_api_description_t *api = pcilib_model[ctx->model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return NULL;
    }

    if (api->get_data) 
	return api->get_data(ctx->event_ctx, event_id, data_type, 0, NULL, size);

    return NULL;
}

int pcilib_return_data(pcilib_t *ctx, pcilib_event_id_t event_id) {
    pcilib_event_api_description_t *api = pcilib_model[ctx->model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->return_data) 
	return api->return_data(ctx->event_ctx, event_id);

    return 0;
}


typedef struct {
    pcilib_t *ctx;
    
    size_t *size;
    void **data;
} pcilib_grab_callback_user_data_t;

static int pcilib_grab_callback(pcilib_event_t event, pcilib_event_id_t event_id, void *vuser) {
    int err;
    void *data;
    size_t size;
    int allocated = 0;

    pcilib_grab_callback_user_data_t *user = (pcilib_grab_callback_user_data_t*)vuser;

    data = pcilib_get_data(user->ctx, event_id, PCILIB_EVENT_DATA, &size);
    if (!data) {
	pcilib_error("Error getting event data");
	return PCILIB_ERROR_FAILED;
    }
    
    if (*(user->data)) {
	if ((user->size)&&(*(user->size) < size)) {
	    pcilib_error("The supplied buffer does not have enough space to hold the event data. Buffer size is %z, but %z is required", user->size, size);
	    return PCILIB_ERROR_MEMORY;
	}

	*(user->size) = size;
    } else {
	*(user->data) = malloc(size);
	if (!*(user->data)) {
	    pcilib_error("Memory allocation (%i bytes) for event data is failed");
	    return PCILIB_ERROR_MEMORY;
	}
	if (*(user->size)) *(user->size) = size;
	allocated = 1;
    }
    
    memcpy(*(user->data), data, size);
    
    err = pcilib_return_data(user->ctx, event_id);
    if (err) {
	if (allocated) {
	    free(*(user->data));
	    *(user->data) = NULL;
	}
	pcilib_error("The event data had been overwritten before it was returned, data corruption may occur");
	return err;
    }
    
    return 0;
}

int pcilib_grab(pcilib_t *ctx, pcilib_event_t event_mask, size_t *size, void **data, const struct timespec *timeout) {
    int err;
    
    pcilib_grab_callback_user_data_t user = {ctx, size, data};
    
    err = pcilib_start(ctx, event_mask, pcilib_grab_callback, &user);
    if (!err) {
	if (timeout) nanosleep(timeout, NULL);
        else err = pcilib_trigger(ctx, event_mask, 0, NULL);
    }
    pcilib_stop(ctx);
    return 0;
}
