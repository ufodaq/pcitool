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

int pcilib_init_register_banks(pcilib_t *ctx) {
    int err; 
    size_t start = ctx->num_banks_init;

    err = pcilib_map_register_space(ctx);
    if (err) return err;

    for (; ctx->num_banks_init < ctx->num_banks; ctx->num_banks_init++) {
	pcilib_register_bank_context_t *bank_ctx;
	pcilib_register_protocol_t protocol;
	const pcilib_register_protocol_api_description_t *bapi;

	protocol = pcilib_find_register_protocol_by_addr(ctx, ctx->banks[ctx->num_banks_init].protocol);
	if (protocol == PCILIB_REGISTER_PROTOCOL_INVALID) {
	    const char *name = ctx->banks[ctx->num_banks_init].name;
	    if (!name) name = "unnamed";
	    pcilib_error("Invalid register protocol address (%u) is specified for bank %i (%s)", ctx->banks[ctx->num_banks_init].protocol, ctx->banks[ctx->num_banks_init].addr, name);
	    pcilib_free_register_banks(ctx, start);
	    return PCILIB_ERROR_INVALID_BANK;
	}
	
	bapi = ctx->protocols[protocol].api;

	if (bapi->init) {
	    const char *model = ctx->protocols[protocol].model;
	    if (!model) model = ctx->model;

	    bank_ctx = bapi->init(ctx, ctx->num_banks_init, model, ctx->protocols[protocol].args);
	} else
	    bank_ctx = (pcilib_register_bank_context_t*)malloc(sizeof(pcilib_register_bank_context_t));
	
	if (!bank_ctx) {
	    pcilib_free_register_banks(ctx, start);
	    return PCILIB_ERROR_FAILED;
	}
	
	bank_ctx->bank = ctx->banks + ctx->num_banks_init;
	bank_ctx->api = bapi;
	bank_ctx->xml = ctx->xml.bank_nodes[ctx->num_banks_init];
	ctx->bank_ctx[ctx->num_banks_init] = bank_ctx;
    }

    return 0;
}

void pcilib_free_register_banks(pcilib_t *ctx, pcilib_register_bank_t start) {
    size_t i;

    for (i = start; i < ctx->num_banks_init; i++) {
	const pcilib_register_protocol_api_description_t *bapi = ctx->bank_ctx[i]->api;
	
	if (ctx->bank_ctx[i]) {
	    if (bapi->free)
		bapi->free(ctx, ctx->bank_ctx[i]);
	    else
		free(ctx->bank_ctx[i]);
		
	    ctx->bank_ctx[i] = NULL;
	}
    }

    ctx->num_banks_init = start;
}

int pcilib_add_register_banks(pcilib_t *ctx, pcilib_model_modification_flags_t flags, size_t n, const pcilib_register_bank_description_t *banks, pcilib_register_bank_t *ids) {
    int err;
    size_t i;
    pcilib_register_bank_t bank;
    size_t dyn_banks = ctx->dyn_banks;
    size_t num_banks = ctx->num_banks;
    size_t cur_banks = num_banks;
	
    if (!n) {
	for (n = 0; banks[n].access; n++);
    }

    if ((ctx->num_banks + n + 1) > PCILIB_MAX_REGISTER_BANKS)
	return PCILIB_ERROR_TOOBIG;

    for (i = 0; i < n; i++) {
	    // Try to find if the bank is already existing...
	bank = pcilib_find_register_bank_by_name(ctx, banks[i].name);
	if ((bank == PCILIB_REGISTER_BANK_INVALID)&&(banks[i].addr != PCILIB_REGISTER_BANK_DYNAMIC))
	    bank = pcilib_find_register_bank_by_addr(ctx, banks[i].addr);

	if (bank == PCILIB_REGISTER_BANK_INVALID) {
	    bank = num_banks++;
	} else if (flags&PCILIB_MODEL_MODIFICATION_FLAG_SKIP_EXISTING) {
	    if (ids) ids[i] = bank;
	    continue;
	} else if ((flags&PCILIB_MODEL_MODIFICATION_FLAG_OVERRIDE) == 0) {
	    if (pcilib_find_register_bank_by_name(ctx, banks[i].name) == PCILIB_REGISTER_BANK_INVALID)
		pcilib_error("The bank %s is already existing and override flag is not set", banks[i].name);
	    else
		pcilib_error("The bank with address 0x%lx is already existing and override flag is not set", banks[i].addr);
	    memset(ctx->banks + ctx->num_banks, 0, sizeof(pcilib_register_bank_description_t));
	    return PCILIB_ERROR_EXIST;
	}
	
	memcpy(ctx->banks + bank,  banks + i, sizeof(pcilib_register_bank_description_t));
	if (ids) ids[i] = bank;
	
	if (banks[i].addr == PCILIB_REGISTER_BANK_DYNAMIC) {
	    ctx->banks[bank].addr = PCILIB_REGISTER_BANK_DYNAMIC + dyn_banks + 1;
	    dyn_banks++;
	}
    }

    ctx->num_banks = num_banks;

	// If banks are already initialized, we need to re-run the initialization code
    if (ctx->reg_bar_mapped) {
	ctx->reg_bar_mapped = 0;
        err = pcilib_init_register_banks(ctx);
        if (err) {
            ctx->num_banks = cur_banks;
	    memset(ctx->banks + ctx->num_banks, 0, sizeof(pcilib_register_bank_description_t));
            return err;
        }
    }

    ctx->dyn_banks = dyn_banks;

    return 0;
}

int pcilib_add_register_protocols(pcilib_t *ctx, pcilib_model_modification_flags_t flags, size_t n, const pcilib_register_protocol_description_t *protocols, pcilib_register_protocol_t *ids) {
	// DS: Override existing banks 
	
    if (!n) {
	for (n = 0; protocols[n].api; n++);
    }

    if ((ctx->num_protocols + n + 1) > PCILIB_MAX_REGISTER_PROTOCOLS)
	return PCILIB_ERROR_TOOBIG;
	
    memcpy(ctx->protocols + ctx->num_protocols, protocols, n * sizeof(pcilib_register_protocol_description_t));
    ctx->num_protocols += n;

    return 0;
}

int pcilib_add_register_ranges(pcilib_t *ctx, pcilib_model_modification_flags_t flags, size_t n, const pcilib_register_range_t *ranges) {
    if (!n) {
	for (n = 0; ranges[n].end; n++);
    }

    if ((ctx->num_ranges + n + 1) > PCILIB_MAX_REGISTER_RANGES)
	return PCILIB_ERROR_TOOBIG;
	
    memcpy(ctx->ranges + ctx->num_ranges, ranges, n * sizeof(pcilib_register_range_t));
    ctx->num_ranges += n;

    return 0;
}

pcilib_register_bank_t pcilib_find_register_bank_by_addr(pcilib_t *ctx, pcilib_register_bank_addr_t bank) {
    pcilib_register_bank_t i;

    for (i = 0; ctx->banks[i].access; i++)
	if (ctx->banks[i].addr == bank) return i;

    return PCILIB_REGISTER_BANK_INVALID;
}

pcilib_register_bank_t pcilib_find_register_bank_by_name(pcilib_t *ctx, const char *bankname) {
    pcilib_register_bank_t i;

    for (i = 0; ctx->banks[i].access; i++)
	if (!strcasecmp(ctx->banks[i].name, bankname)) return i;

    return PCILIB_REGISTER_BANK_INVALID;
}

pcilib_register_bank_t pcilib_find_register_bank(pcilib_t *ctx, const char *bank) {
    pcilib_register_bank_t res;
    unsigned long addr;
    
    if (!bank) {
        const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
	const pcilib_register_bank_description_t *banks = model_info->banks;
	if ((banks)&&(banks[0].access)) return (pcilib_register_bank_t)0;
	return PCILIB_REGISTER_BANK_INVALID;
    }
    
    if (pcilib_isxnumber(bank)&&(sscanf(bank,"%lx", &addr) == 1)) {
	res = pcilib_find_register_bank_by_addr(ctx, addr);
	if (res != PCILIB_REGISTER_BANK_INVALID) return res;
    }
    
    return pcilib_find_register_bank_by_name(ctx, bank);
}

    // DS: FIXME create hash during map_register space
pcilib_register_t pcilib_find_register(pcilib_t *ctx, const char *bank, const char *reg) {
    pcilib_register_t i;
    pcilib_register_bank_t bank_id;
    pcilib_register_bank_addr_t bank_addr = 0;
    pcilib_register_context_t *reg_ctx;

    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    const pcilib_register_description_t *registers =  model_info->registers;
    
    if (bank) {
	bank_id = pcilib_find_register_bank(ctx, bank);
	if (bank_id == PCILIB_REGISTER_BANK_INVALID) {
	    pcilib_error("Invalid bank (%s) is specified", bank);
	    return PCILIB_REGISTER_INVALID;
	}
	
	bank_addr = model_info->banks[bank_id].addr;

            // ToDo: we can use additionaly per-bank hashes
        for (i = 0; registers[i].bits; i++) {
	    if ((!strcasecmp(registers[i].name, reg))&&((!bank)||(registers[i].bank == bank_addr))) return i;
        }
    } else {
        HASH_FIND_STR(ctx->reg_hash, reg, reg_ctx);
        if (reg_ctx) return reg_ctx->reg;
    }


    return PCILIB_REGISTER_INVALID;
};

pcilib_register_protocol_t pcilib_find_register_protocol_by_addr(pcilib_t *ctx, pcilib_register_protocol_addr_t protocol) {
    pcilib_register_protocol_t i;

    for (i = 0; ctx->protocols[i].api; i++)
	if (ctx->protocols[i].addr == protocol) return i;

    return PCILIB_REGISTER_PROTOCOL_INVALID;
}

pcilib_register_protocol_t pcilib_find_register_protocol_by_name(pcilib_t *ctx, const char *name) {
    pcilib_register_protocol_t i;

    for (i = 0; ctx->protocols[i].api; i++)
	if (!strcasecmp(ctx->protocols[i].name, name)) return i;

    return PCILIB_REGISTER_PROTOCOL_INVALID;
}

pcilib_register_protocol_t pcilib_find_register_protocol(pcilib_t *ctx, const char *protocol) {
    pcilib_register_bank_t res;
    unsigned long addr;

    if (pcilib_isxnumber(protocol)&&(sscanf(protocol,"%lx", &addr) == 1)) {
	res = pcilib_find_register_protocol_by_addr(ctx, addr);
	if (res != PCILIB_REGISTER_BANK_INVALID) return res;
    }

    return pcilib_find_register_protocol_by_name(ctx, protocol);
}

uintptr_t pcilib_resolve_bank_address_by_id(pcilib_t *ctx, pcilib_address_resolution_flags_t flags, pcilib_register_bank_t bank) {
    pcilib_register_bank_context_t *bctx = ctx->bank_ctx[bank];

    if (!bctx->api->resolve)
	return PCILIB_ADDRESS_INVALID;
	
    return bctx->api->resolve(ctx, bctx, flags, PCILIB_REGISTER_ADDRESS_INVALID);
}

uintptr_t pcilib_resolve_bank_address(pcilib_t *ctx, pcilib_address_resolution_flags_t flags, const char *bank) {
    pcilib_register_bank_t bank_id = pcilib_find_register_bank(ctx, bank);
    if (bank_id == PCILIB_REGISTER_BANK_INVALID) {
	if (bank) pcilib_error("Invalid register bank is specified (%s)", bank);
	else pcilib_error("Register bank should be specified");
	return PCILIB_ADDRESS_INVALID;
    }

    return pcilib_resolve_bank_address_by_id(ctx, flags, bank_id);
}

uintptr_t pcilib_resolve_register_address_by_id(pcilib_t *ctx, pcilib_address_resolution_flags_t flags, pcilib_register_t reg) {
    pcilib_register_bank_context_t *bctx = ctx->bank_ctx[ctx->register_ctx[reg].bank];

    if (!bctx->api->resolve)
	return PCILIB_ADDRESS_INVALID;

    return bctx->api->resolve(ctx, bctx, 0, ctx->registers[reg].addr);
}

uintptr_t pcilib_resolve_register_address(pcilib_t *ctx, pcilib_address_resolution_flags_t flags, const char *bank, const char *regname) {
    pcilib_register_t reg = pcilib_find_register(ctx, bank, regname);
    if (reg == PCILIB_REGISTER_INVALID) {
	pcilib_error("Register (%s) is not found", regname);
	return PCILIB_ADDRESS_INVALID;
    }

    return pcilib_resolve_register_address_by_id(ctx, flags, reg);
}

int pcilib_get_register_bank_attr_by_id(pcilib_t *ctx, pcilib_register_bank_t bank, const char *attr, pcilib_value_t *val) {
    assert(bank < ctx->num_banks);

    return pcilib_get_xml_attr(ctx, ctx->bank_ctx[bank]->xml, attr, val);
}

int pcilib_get_register_bank_attr(pcilib_t *ctx, const char *bankname, const char *attr, pcilib_value_t *val) {
    pcilib_register_bank_t bank;

    bank = pcilib_find_register_bank_by_name(ctx, bankname);
    if (bank == PCILIB_REGISTER_BANK_INVALID) {
        pcilib_error("Bank (%s) is not found", bankname);
        return PCILIB_ERROR_NOTFOUND;
    }

    return pcilib_get_register_bank_attr_by_id(ctx, bank, attr, val);
}
