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

#include "tools.h"
#include "error.h"

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
