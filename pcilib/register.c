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

#include "pci.h"
#include "bank.h"

#include "tools.h"
#include "error.h"


int pcilib_add_registers(pcilib_t *ctx, size_t n, const pcilib_register_description_t *registers) {
	// DS: Overrride existing registers 
	// Registers identified by addr + offset + size + type or name
	
    pcilib_register_description_t *regs;
    pcilib_register_context_t *reg_ctx;
    size_t size;

    if (!n) {
	for (n = 0; registers[n].bits; n++);
    }

    if ((ctx->num_reg + n + 1) > ctx->alloc_reg) {
	for (size = ctx->alloc_reg; size < 2 * (n + ctx->num_reg + 1); size<<=1);

	regs = (pcilib_register_description_t*)realloc(ctx->registers, size * sizeof(pcilib_register_description_t));
	if (!regs) return PCILIB_ERROR_MEMORY;

	ctx->registers = regs;
	ctx->model_info.registers = regs;

	reg_ctx = (pcilib_register_context_t*)realloc(ctx->registers, size * sizeof(pcilib_register_context_t));
	if (!reg_ctx) return PCILIB_ERROR_MEMORY;
	
	memset(reg_ctx + ctx->alloc_reg, 0, (size - ctx->alloc_reg) * sizeof(pcilib_register_context_t));

	ctx->register_ctx = reg_ctx;

	ctx->alloc_reg = size;
    }

    memcpy(ctx->registers + ctx->num_reg, registers, n * sizeof(pcilib_register_description_t));
    memset(ctx->registers + ctx->num_reg + n, 0, sizeof(pcilib_register_description_t));
    ctx->num_reg += n;

    return 0;
}


static int pcilib_read_register_space_internal(pcilib_t *ctx, pcilib_register_bank_t bank, pcilib_register_addr_t addr, size_t n, pcilib_register_size_t offset, pcilib_register_size_t bits, pcilib_register_value_t *buf) {
    int err;
    size_t i;

    pcilib_register_bank_context_t *bctx = ctx->bank_ctx[bank];
    const pcilib_register_protocol_api_description_t *bapi = bctx->api;
    const pcilib_register_bank_description_t *b = bctx->bank;

    int access = b->access / 8;

    assert(bits < 8 * sizeof(pcilib_register_value_t));

    if (!bapi->read) {
	pcilib_error("Used register protocol does not define a way to read register value");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (((addr + n) > b->size)||(((addr + n) == b->size)&&(bits))) {
	if ((b->format)&&(strchr(b->format, 'x')))
	    pcilib_error("Accessing register (%u regs at addr 0x%x) out of register space (%u registers total)", bits?(n+1):n, addr, b->size);
	else 
	    pcilib_error("Accessing register (%u regs at addr %u) out of register space (%u registers total)", bits?(n+1):n, addr, b->size);
	return PCILIB_ERROR_OUTOFRANGE;
    }

    //err = pcilib_init_register_banks(ctx);
    //if (err) return err;
    
    //n += bits / b->access;
    //bits %= b->access; 
    
    for (i = 0; i < n; i++) {
	err = bapi->read(ctx, bctx, addr + i * access, buf + i);
	printf("buf +i: %i \n",buf[i]);
	if(err) printf("err internal 1: %i\n",err);
	if (err) break;
    }
    
    if ((bits > 0)&&(!err)) {
	pcilib_register_value_t val = 0;
	err = bapi->read(ctx, bctx, addr + n * access, &val);
	
	val = (val >> offset)&BIT_MASK(bits);
	printf("val : %i\n",val);
	memcpy(buf + n, &val, sizeof(pcilib_register_value_t));
		if(err) printf("err internal 2: %i\n",err);
    }
    	printf("err internal 3: %i\n",err);
    printf("buf internal: %i\n",buf[0]);
    
    return err;
}

int pcilib_read_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf) {
    pcilib_register_bank_t bank_id = pcilib_find_register_bank(ctx, bank);
    if (bank_id == PCILIB_REGISTER_BANK_INVALID) {
	if (bank) pcilib_error("Invalid register bank is specified (%s)", bank);
	else pcilib_error("Register bank should be specified");
	return PCILIB_ERROR_INVALID_BANK;
    }
    
    return pcilib_read_register_space_internal(ctx, bank_id, addr, n, 0, 0, buf);
}

int pcilib_read_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t *value) {
    int err;
    size_t i, n;
    pcilib_register_size_t bits;
    pcilib_register_value_t res;
    pcilib_register_bank_t bank;
    const pcilib_register_description_t *r;
    const pcilib_register_bank_description_t *b;
    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    r = model_info->registers + reg;
    
    bank = pcilib_find_register_bank_by_addr(ctx, r->bank);
    if (bank == PCILIB_REGISTER_BANK_INVALID) return PCILIB_ERROR_INVALID_BANK;
    
    b = model_info->banks + bank;
    
    n = r->bits / b->access;
    bits = r->bits % b->access; 

    pcilib_register_value_t buf[n + 1];
    err = pcilib_read_register_space_internal(ctx, bank, r->addr, n, r->offset, bits, buf);

    if ((b->endianess == PCILIB_BIG_ENDIAN)||((b->endianess == PCILIB_HOST_ENDIAN)&&(ntohs(1) == 1))) {
	pcilib_error("Big-endian byte order support is not implemented");
	return PCILIB_ERROR_NOTSUPPORTED;
    } else {
      printf("bits: %i, n %lu\n",bits, n);
	res = 0;
	if (bits) ++n;
	for (i = 0; i < n; i++) {
	  printf("res: %i buf[i]: %i\n",res,buf[i]);
	    res |= buf[i] << (i * b->access);
	    printf("res: %i \n",res);
	}
    }
    
    *value = res;
    printf("value : %i\n",*value);
    
    return err;
}


int pcilib_read_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t *value) {
    int reg;

    reg = pcilib_find_register(ctx, bank, regname);
    if (reg == PCILIB_REGISTER_INVALID) {
	pcilib_error("Register (%s) is not found", regname);
	return PCILIB_ERROR_NOTFOUND;
    }

    return pcilib_read_register_by_id(ctx, reg, value);
}


static int pcilib_write_register_space_internal(pcilib_t *ctx, pcilib_register_bank_t bank, pcilib_register_addr_t addr, size_t n, pcilib_register_size_t offset, pcilib_register_size_t bits, pcilib_register_value_t rwmask, pcilib_register_value_t *buf) {
    int err;
    size_t i;

    pcilib_register_bank_context_t *bctx = ctx->bank_ctx[bank];
    const pcilib_register_protocol_api_description_t *bapi = bctx->api;
    const pcilib_register_bank_description_t *b = bctx->bank;

    int access = b->access / 8;

    assert(bits < 8 * sizeof(pcilib_register_value_t));

    if (!bapi->write) {
	pcilib_error("Used register protocol does not define a way to write value into the register");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (((addr + n) > b->size)||(((addr + n) == b->size)&&(bits))) {
	if ((b->format)&&(strchr(b->format, 'x')))
	    pcilib_error("Accessing register (%u regs at addr 0x%x) out of register space (%u registers total)", bits?(n+1):n, addr, b->size);
	else 
	    pcilib_error("Accessing register (%u regs at addr %u) out of register space (%u registers total)", bits?(n+1):n, addr, b->size);
	return PCILIB_ERROR_OUTOFRANGE;
    }

    //err = pcilib_init_register_banks(ctx);
    //if (err) return err;
    
    //n += bits / b->access;
    //bits %= b->access; 
    
    for (i = 0; i < n; i++) {
	err = bapi->write(ctx, bctx, addr + i * access, buf[i]);
	if (err) break;
    }
    
    if ((bits > 0)&&(!err)) {
	pcilib_register_value_t val = (buf[n]&BIT_MASK(bits))<<offset;
	pcilib_register_value_t mask = BIT_MASK(bits)<<offset;

	if (~mask&rwmask) {
	    pcilib_register_value_t rval;

	    if (!bapi->read) {
		pcilib_error("Used register protocol does not define a way to read register. Therefore, it is only possible to write a full bank word, not partial as required by the accessed register");
		return PCILIB_ERROR_NOTSUPPORTED;
	    }

	    err = bapi->read(ctx, bctx, addr + n * access, &rval); 
	    if (err) return err;

	    val |= (rval & rwmask & ~mask);
	}
	
	err = bapi->write(ctx, bctx, addr + n * access, val);
    }
    
    return err;
}

int pcilib_write_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf) {
    pcilib_register_bank_t bank_id = pcilib_find_register_bank(ctx, bank);
    if (bank_id == PCILIB_REGISTER_BANK_INVALID) {
	if (bank) pcilib_error("Invalid register bank is specified (%s)", bank);
	else pcilib_error("Register bank should be specified");
	return PCILIB_ERROR_INVALID_BANK;
    }
    
    return pcilib_write_register_space_internal(ctx, bank_id, addr, n, 0, 0, 0, buf);
}


int pcilib_write_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t value) {
    int err;
    size_t i, n;
    pcilib_register_size_t bits;
    pcilib_register_bank_t bank;
    pcilib_register_value_t res;
    const pcilib_register_description_t *r;
    const pcilib_register_bank_description_t *b;
    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    r = model_info->registers + reg;

    bank = pcilib_find_register_bank_by_addr(ctx, r->bank);
    if (bank == PCILIB_REGISTER_BANK_INVALID) return PCILIB_ERROR_INVALID_BANK;

    b = model_info->banks + bank;
    
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

    err = pcilib_write_register_space_internal(ctx, bank, r->addr, n, r->offset, bits, r->rwmask, buf);
    return err;
}

int pcilib_write_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t value) {
    int reg;
    
    reg = pcilib_find_register(ctx, bank, regname);
    if (reg == PCILIB_REGISTER_INVALID) {
	pcilib_error("Register (%s) is not found", regname);
	return PCILIB_ERROR_NOTFOUND;
    }

    return pcilib_write_register_by_id(ctx, reg, value);
}
