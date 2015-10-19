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
#include <alloca.h>

#include "pci.h"
#include "bank.h"

#include "tools.h"
#include "error.h"
#include "property.h"
#include "views/enum.h"

int pcilib_add_registers(pcilib_t *ctx, pcilib_model_modification_flags_t flags, size_t n, const pcilib_register_description_t *registers, pcilib_register_t *ids) {
	// DS: Overrride existing registers 
	// Registers identified by addr + offset + size + type or name
    int err;
    size_t size;
    pcilib_register_t i;

    pcilib_register_description_t *regs;
    pcilib_register_context_t *reg_ctx;

    pcilib_register_bank_t bank = PCILIB_REGISTER_BANK_INVALID;
    pcilib_register_bank_addr_t bank_addr = (pcilib_register_bank_addr_t)-1;
    pcilib_register_bank_t *banks;

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

	if (ctx->register_ctx != reg_ctx) {
	        // We need to recreate cache if context is moved...
	    HASH_CLEAR(hh, ctx->reg_hash);
	    for (i = 0; i < ctx->num_reg; i++) {
                pcilib_register_context_t *cur = &ctx->register_ctx[ctx->num_reg + i];
                HASH_ADD_KEYPTR(hh, ctx->reg_hash, cur->name, strlen(cur->name), cur);
	    }
	}

	ctx->register_ctx = reg_ctx;
	ctx->alloc_reg = size;
    }

    banks = (pcilib_register_bank_t*)alloca(n * sizeof(pcilib_register_bank_t));
    if (!banks) return PCILIB_ERROR_MEMORY;

    for (i = 0; i < n; i++) {
        if (registers[i].bank != bank_addr) {
            bank_addr = registers[i].bank;
            bank = pcilib_find_register_bank_by_addr(ctx, bank_addr);
            if (bank == PCILIB_REGISTER_BANK_INVALID) {
                pcilib_error("Invalid bank address (0x%lx) is specified for register %s", bank_addr, registers[i].name);
                return PCILIB_ERROR_INVALID_BANK;
            }
        }

/*
            // No hash so far, will iterate.
        pcilib_register_t reg = pcilib_find_register(ctx, ctx->banks[bank].name, registers[i].name);
        if (reg != PCILIB_REGISTER_INVALID) {
            pcilib_error("Register %s is already defined in the model", registers[i].name);
            return PCILIB_ERROR_EXIST;
        }
*/

        banks[i] = bank;
    }

    err = pcilib_add_register_properties(ctx, n, banks, registers);
    if (err) return err;

    for (i = 0; i < n; i++) {
        pcilib_register_context_t *cur = &ctx->register_ctx[ctx->num_reg + i];

        cur->reg = ctx->num_reg + i;
        cur->name = registers[i].name;
        cur->bank = banks[i];
        HASH_ADD_KEYPTR(hh, ctx->reg_hash, cur->name, strlen(cur->name), cur);
    }

    memcpy(ctx->registers + ctx->num_reg, registers, n * sizeof(pcilib_register_description_t));
    memset(ctx->registers + ctx->num_reg + n, 0, sizeof(pcilib_register_description_t));

    if (ids) {
	size_t i;
	
	for (i = 0; i < n; i++)
	    ids[i] = ctx->num_reg + i;
    }

    ctx->num_reg += n;

    return 0;
}

void pcilib_clean_registers(pcilib_t *ctx, pcilib_register_t start) {
    pcilib_register_t reg;
    pcilib_register_context_t *reg_ctx, *tmp;

    if (start) {
        HASH_ITER(hh, ctx->reg_hash, reg_ctx, tmp) {
            if (reg_ctx->reg >= start) {
                HASH_DEL(ctx->reg_hash, reg_ctx);
            }
        }
    } else {
        HASH_CLEAR(hh, ctx->reg_hash);
    }

    for (reg = start; reg < ctx->num_reg; reg++) {
	if (ctx->register_ctx[reg].views)
	    free(ctx->register_ctx[reg].views);
    }

    if (ctx->registers)
        memset(&ctx->registers[start], 0, sizeof(pcilib_register_description_t));

    if (ctx->register_ctx)
        memset(&ctx->register_ctx[start], 0, (ctx->alloc_reg - start) * sizeof(pcilib_register_context_t));

    ctx->num_reg = start;
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
	if (err) break;
    }
    
    if ((bits > 0)&&(!err)) {
	pcilib_register_value_t val = 0;
	err = bapi->read(ctx, bctx, addr + n * access, &val);

	val = (val >> offset)&BIT_MASK(bits);
	memcpy(buf + n, &val, sizeof(pcilib_register_value_t));
    }
    
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


int pcilib_get_register_attr_by_id(pcilib_t *ctx, pcilib_register_t reg, const char *attr, pcilib_value_t *val) {
    int err;

    assert(reg < ctx->num_reg);

    err = pcilib_get_xml_attr(ctx, ctx->register_ctx[reg].xml, attr, val);
/*
        // Shall we return from parrent register if not found?
    if ((err == PCILIB_ERROR_NOTFOUND)&&(ctx->registers[reg].type == PCILIB_REGISTER_TYPE_BITS)) {
        pcilib_register_t parent = pcilib_find_standard_register_by_addr(ctx, ctx->registers[reg].addr);
        err = pcilib_get_xml_attr(ctx, ctx->register_ctx[parent].xml, attr, val);
    }
*/
    return err;
}

int pcilib_get_register_attr(pcilib_t *ctx, const char *bank, const char *regname, const char *attr, pcilib_value_t *val) {
    pcilib_register_t reg;
    
    reg = pcilib_find_register(ctx, bank, regname);
    if (reg == PCILIB_REGISTER_INVALID) {
        pcilib_error("Register (%s) is not found", regname);
        return PCILIB_ERROR_NOTFOUND;
    }

    return pcilib_get_register_attr_by_id(ctx, reg, attr, val);
}

pcilib_register_info_t *pcilib_get_register_info(pcilib_t *ctx, const char *req_bank_name, const char *req_reg_name, pcilib_list_flags_t flags) {
    pcilib_register_t i, from, to, pos = 0;
    pcilib_register_info_t *info;
    pcilib_register_bank_t bank;
    pcilib_register_bank_addr_t bank_addr;
    const char *bank_name;

    info = (pcilib_register_info_t*)malloc((ctx->num_reg + 1) * sizeof(pcilib_register_info_t));
    if (!info) return NULL;

    if (req_bank_name) {
        bank = pcilib_find_register_bank_by_name(ctx, req_bank_name);
        if (bank == PCILIB_REGISTER_BANK_INVALID) {
            pcilib_error("The specified bank (%s) is not found", req_bank_name);
            return NULL;
        }
        bank_addr = ctx->banks[bank].addr;
        bank_name = req_bank_name;
    } else {
        bank_addr = PCILIB_REGISTER_BANK_INVALID;
        bank_name = NULL;
    }

    if (req_reg_name) {
        pcilib_register_t reg = pcilib_find_register(ctx, req_bank_name, req_reg_name);
        if (reg == PCILIB_REGISTER_INVALID) {
            pcilib_error("The specified register (%s) is not found", req_reg_name);
            return NULL;
        }
        from = reg;
        to = reg + 1;
    } else {
        from = 0;
        to = ctx->num_reg;
    }

    for (i = from; i < to; i++) {
        const pcilib_register_value_range_t *range = &ctx->register_ctx[i].range;
        const pcilib_register_value_name_t *names = NULL;

        if (req_bank_name) {
            if (ctx->registers[i].bank != bank_addr) continue;
        } else {
            if (ctx->registers[i].bank != bank_addr) {
                bank_addr = ctx->registers[i].bank;
                bank = pcilib_find_register_bank_by_addr(ctx, bank_addr);
                if (bank == PCILIB_REGISTER_BANK_INVALID) bank_name = NULL;
                else bank_name = ctx->banks[bank].name;
            }
        }

        if (ctx->registers[i].views) {
            int j;
            for (j = 0; ctx->registers[i].views[j].view; j++) {
                pcilib_view_t view = pcilib_find_view_by_name(ctx, ctx->registers[i].views[j].view);
                if ((view != PCILIB_VIEW_INVALID)&&((ctx->views[view]->api == &pcilib_enum_view_xml_api)||(ctx->views[view]->api == &pcilib_enum_view_static_api)))
                    names = ((pcilib_enum_view_description_t*)(ctx->views[view]))->names;
            }
        }

        if (range->min == range->max) 
            range = NULL;

        info[pos++] = (pcilib_register_info_t){
            .id = i,
            .name = ctx->registers[i].name,
            .description = ctx->registers[i].description,
            .bank = bank_name,
            .mode = ctx->registers[i].mode,
            .defvalue = ctx->registers[i].defvalue,
            .range = range,
            .values = names
        };
    }
    memset(&info[pos], 0, sizeof(pcilib_register_info_t));
    return info;
}

pcilib_register_info_t *pcilib_get_register_list(pcilib_t *ctx, const char *req_bank_name, pcilib_list_flags_t flags) {
    return pcilib_get_register_info(ctx, req_bank_name, NULL, flags);
}

void pcilib_free_register_info(pcilib_t *ctx, pcilib_register_info_t *info) {
    if (info)
        free(info);
}
