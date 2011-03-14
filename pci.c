#define _PCILIB_PCI_C

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
#include <errno.h>
#include <assert.h>

#include "kernel.h"
#include "tools.h"

#include "pci.h"
#include "ipecamera.h"
#include "error.h"

#define BIT_MASK(bits) ((1l << (bits)) - 1)


//#define PCILIB_FILE_IO

struct pcilib_s {
    int handle;
    
    uintptr_t page_mask;
    pci_board_info board_info;
    pcilib_model_t model;

    pcilib_bar_t reg_bar;
    char *reg_space;
    
#ifdef PCILIB_FILE_IO
    int file_io_handle;
#endif /* PCILIB_FILE_IO */
};

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
    pcilib_error = err;
    pcilib_warning = warn;
}

pcilib_t *pcilib_open(const char *device, pcilib_model_t model) {
    pcilib_t *ctx = malloc(sizeof(pcilib_t));

    if (ctx) {
	ctx->handle = open(device, O_RDWR);
	ctx->page_mask = (uintptr_t)-1;
	ctx->model = model;
	ctx->reg_space = NULL;
    }

    return ctx;
}

const pci_board_info *pcilib_get_board_info(pcilib_t *ctx) {
    int ret;
    
    if (ctx->page_mask ==  (uintptr_t)-1) {
	ret = ioctl( ctx->handle, PCIDRIVER_IOC_PCI_INFO, &ctx->board_info );
	if (ret) pcilib_error("PCIDRIVER_IOC_PCI_INFO ioctl have failed");
	
	ctx->page_mask = pcilib_get_page_mask();
    }
    
    return &ctx->board_info;
}


pcilib_model_t pcilib_get_model(pcilib_t *ctx) {
    if (ctx->model == PCILIB_MODEL_DETECT) {
	unsigned short vendor_id;
	unsigned short device_id;

	const pci_board_info *board_info = pcilib_get_board_info(ctx);

	if ((board_info->vendor_id == PCIE_XILINX_VENDOR_ID)&&(board_info->device_id == PCIE_IPECAMERA_DEVICE_ID))
	    ctx->model = PCILIB_MODEL_IPECAMERA;
	else
	    ctx->model = PCILIB_MODEL_PCI;
    }
    
    return ctx->model;
}

static int pcilib_detect_bar(pcilib_t *ctx, uintptr_t addr, size_t size) {
    int ret,i;
	
    const pci_board_info *board_info = pcilib_get_board_info(ctx);
		
    for (i = 0; i < PCILIB_MAX_BANKS; i++) {
	if ((addr >= board_info->bar_start[i])&&((board_info->bar_start[i] + board_info->bar_length[i]) >= (addr + size))) return i;
    }
	
    return -1;
}

static void pcilib_detect_address(pcilib_t *ctx, pcilib_bar_t *bar, uintptr_t *addr, size_t size) {
    const pci_board_info *board_info = pcilib_get_board_info(ctx);
    
    if (*bar == PCILIB_BAR_DETECT) {
	*bar = pcilib_detect_bar(ctx, *addr, size);
	if (*bar < 0) pcilib_error("The requested data block at address 0x%x with size 0x%x does not belongs to any available memory bank", *addr, size);
    } else {
	if ((*addr < board_info->bar_start[*bar])||((board_info->bar_start[*bar] + board_info->bar_length[*bar]) < (((uintptr_t)*addr) + size))) {
	    if ((board_info->bar_length[*bar]) >= (((uintptr_t)*addr) + size)) 
		*addr += board_info->bar_start[*bar];
	    else
		pcilib_error("The requested data block at address 0x%x with size 0x%x does not belong the specified memory bank (Bar %i: starting at 0x%x with size 0x%x)", *addr, size, *bar, board_info->bar_start[*bar], board_info->bar_length[*bar]);
	}
    }
    
    *addr -= board_info->bar_start[*bar];
    *addr += board_info->bar_start[*bar] & ctx->page_mask;
}

void *pcilib_map_bar(pcilib_t *ctx, pcilib_bar_t bar) {
    void *res;
    int ret; 

    const pci_board_info *board_info = pcilib_get_board_info(ctx);
    
    ret = ioctl( ctx->handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_PCI );
    if (ret) pcilib_error("PCIDRIVER_IOC_MMAP_MODE ioctl have failed", bar);

    ret = ioctl( ctx->handle, PCIDRIVER_IOC_MMAP_AREA, PCIDRIVER_BAR0 + bar );
    if (ret) pcilib_error("PCIDRIVER_IOC_MMAP_AREA ioctl have failed for bank %i", bar);

#ifdef PCILIB_FILE_IO
    file_io_handle = open("/root/data", O_RDWR);
    res = mmap( 0, board_info->bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, ctx->file_io_handle, 0 );
#else
    res = mmap( 0, board_info->bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, ctx->handle, 0 );
#endif
    if ((!res)||(res == MAP_FAILED)) pcilib_error("Failed to mmap data bank %i", bar);

    
    return res;
}

void pcilib_unmap_bar(pcilib_t *ctx, pcilib_bar_t bar, void *data) {
    const pci_board_info *board_info = pcilib_get_board_info(ctx);

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


static pcilib_register_bank_t pcilib_find_bank_by_addr(pcilib_t *ctx, pcilib_register_bank_addr_t bank) {
    pcilib_register_bank_t i;
    pcilib_model_t model = pcilib_get_model(ctx);
    pcilib_register_bank_description_t *banks = pcilib_model[model].banks;

    for (i = 0; banks[i].access; i++)
	if (banks[i].addr == bank) return i;
	
    return -1;
}

static pcilib_register_bank_t pcilib_find_bank_by_name(pcilib_t *ctx, const char *bankname) {
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
    
    return -1;
};



static int pcilib_map_register_space(pcilib_t *ctx) {
    if (!ctx->reg_space)  {
	pcilib_model_t model = pcilib_get_model(ctx);
	pcilib_register_bank_description_t *banks = pcilib_model[model].banks;

	if ((banks)&&(banks[0].access)) {
	    void *reg_space;
	    
	    uintptr_t addr = banks[0].read_addr;
	    pcilib_bar_t bar = PCILIB_BAR_DETECT;
	    
	    pcilib_detect_address(ctx, &bar, &addr, 1);
	    reg_space = pcilib_map_bar(ctx, bar);

	    uint32_t buf[2];
	    pcilib_memcpy(&buf, reg_space, 8);
	    
	    if (reg_space) {
	        ctx->reg_bar = bar;
		ctx->reg_space = reg_space;

		return 0;
	    }
	}
	
	return -1;
    }
    
    return 0;
}
	    
	
static void pcilib_unmap_register_space(pcilib_t *ctx) {
    if (ctx->reg_space) {
	pcilib_unmap_bar(ctx, ctx->reg_bar, ctx->reg_space);
	ctx->reg_space = NULL;
    }
}

char  *pcilib_resolve_register_address(pcilib_t *ctx, uintptr_t addr) {
    size_t offset = addr - ctx->board_info.bar_start[ctx->reg_bar];
    if (offset < ctx->board_info.bar_length[ctx->reg_bar]) {
	return ctx->reg_space + offset + (ctx->board_info.bar_start[ctx->reg_bar] & ctx->page_mask);
    }
    return NULL;
}


void pcilib_close(pcilib_t *ctx) {
    if (ctx) {
	if (ctx->reg_space) pcilib_unmap_register_space(ctx);
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

    if (b->endianess) {
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
    
    if (b->endianess) {
	pcilib_error("Big-endian byte order support is not implemented");
	return PCILIB_ERROR_NOTSUPPORTED;
    } else {
	for (i = 0, res = value; (res > 0)&&(i <= n); ++i) {
	    buf[i] = res & BIT_MASK(b->access);
	    res >>= b->access;
	}
	
	if (res) {
	    pcilib_error("Value %i is to big to fit in the register %s", value, r->name);
	    return PCILIB_ERROR_OUTOFRANGE;
	}
    }
    
    err = pcilib_write_register_space_internal(ctx, r->bank, r->addr, n, bits, buf);
    return err;
}

int pcilib_write_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t value) {
    int err;
    int reg;
    
    reg = pcilib_find_register(ctx, bank, regname);
    if (reg < 0) pcilib_error("Register (%s) is not found", regname);

    return pcilib_write_register_by_id(ctx, reg, value);
}

