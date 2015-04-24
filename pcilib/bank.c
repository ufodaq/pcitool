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
	    return PCILIB_ERROR_INVALID_BANK;
	}
	
	bapi = ctx->protocols[protocol].api;

	if (bapi->init) {
	    const char *model = ctx->protocols[protocol].model;
	    if (!model) model = ctx->model;

	    bank_ctx = bapi->init(ctx, ctx->num_banks_init, model, ctx->protocols[protocol].args);
	} else
	    bank_ctx = (pcilib_register_bank_context_t*)malloc(sizeof(pcilib_register_bank_context_t));
	
	if (!bank_ctx)
	    return PCILIB_ERROR_FAILED;
	
	bank_ctx->bank = ctx->banks + ctx->num_banks_init;
	bank_ctx->api = bapi;
	ctx->bank_ctx[ctx->num_banks_init] = bank_ctx;
    }

    return 0;
}

void pcilib_free_register_banks(pcilib_t *ctx) {
    size_t i;

    for (i = 0; i < ctx->num_banks_init; i++) {
	const pcilib_register_protocol_api_description_t *bapi = ctx->bank_ctx[i]->api;
	
	if (ctx->bank_ctx[i]) {
	    if (bapi->free)
		bapi->free(ctx->bank_ctx[i]);
	    else
		free(ctx->bank_ctx[i]);
		
	    ctx->bank_ctx[i] = NULL;
	}
    }

    ctx->num_banks_init = 0;
}


int pcilib_add_register_banks(pcilib_t *ctx, size_t n, const pcilib_register_bank_description_t *banks) {
	// DS: What we are doing if bank exists? 
	
    if (!n) {
	for (n = 0; banks[n].access; n++);
    }

    if ((ctx->num_banks + n + 1) > PCILIB_MAX_REGISTER_BANKS)
	return PCILIB_ERROR_TOOBIG;
	
    memset(ctx->banks + ctx->num_banks + n, 0, sizeof(pcilib_register_bank_description_t));
    memcpy(ctx->banks + ctx->num_banks, banks, n * sizeof(pcilib_register_bank_description_t));
    ctx->num_banks += n;

	// If banks are already initialized, we need to re-run the initialization code
	// DS: Locking is currently missing
    if (ctx->reg_bar_mapped) {
	ctx->reg_bar_mapped = 0;
        return pcilib_init_register_banks(ctx);
    }
    
    return 0;
}

pcilib_register_bank_t pcilib_find_register_bank_by_addr(pcilib_t *ctx, pcilib_register_bank_addr_t bank) {
    pcilib_register_bank_t i;
    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    const pcilib_register_bank_description_t *banks = model_info->banks;

    for (i = 0; banks[i].access; i++)
	if (banks[i].addr == bank) return i;

    return PCILIB_REGISTER_BANK_INVALID;
}

pcilib_register_bank_t pcilib_find_register_bank_by_name(pcilib_t *ctx, const char *bankname) {
    pcilib_register_bank_t i;
    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    const pcilib_register_bank_description_t *banks = model_info->banks;

    for (i = 0; banks[i].access; i++)
	if (!strcasecmp(banks[i].name, bankname)) return i;

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

    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    const pcilib_register_description_t *registers =  model_info->registers;
    
    if (bank) {
	bank_id = pcilib_find_register_bank(ctx, bank);
	if (bank_id == PCILIB_REGISTER_BANK_INVALID) {
	    pcilib_error("Invalid bank (%s) is specified", bank);
	    return PCILIB_REGISTER_INVALID;
	}
	
	bank_addr = model_info->banks[bank_id].addr;
    }
    
    for (i = 0; registers[i].bits; i++) {
	if ((!strcasecmp(registers[i].name, reg))&&((!bank)||(registers[i].bank == bank_addr))) return i;
    }

    return PCILIB_REGISTER_INVALID;
};


pcilib_register_protocol_t pcilib_find_register_protocol_by_addr(pcilib_t *ctx, pcilib_register_protocol_addr_t protocol) {
    pcilib_register_protocol_t i;
    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    const pcilib_register_protocol_description_t *protocols = model_info->protocols;

    for (i = 0; protocols[i].api; i++)
	if (protocols[i].addr == protocol) return i;

    return PCILIB_REGISTER_PROTOCOL_INVALID;
}

pcilib_register_protocol_t pcilib_find_register_protocol_by_name(pcilib_t *ctx, const char *name) {
    pcilib_register_protocol_t i;
    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    const pcilib_register_protocol_description_t *protocols = model_info->protocols;

    for (i = 0; protocols[i].api; i++)
	if (!strcasecmp(protocols[i].name, name)) return i;

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
