#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>


#include "tools.h"
#include "model.h"
#include "error.h"
#include "kmem.h"
#include "pcilib.h"
#include "pci.h"
#include "datacpy.h"

typedef struct pcilib_software_register_bank_context_s pcilib_software_register_bank_context_t;

/**
 * structure defining the context of software registers
 */
struct pcilib_software_register_bank_context_s {
    pcilib_register_bank_context_t bank_ctx;	/**< the bank context associated with the software registers */
    pcilib_kmem_handle_t *kmem;			/**< the kernel memory for software registers */
    volatile void *addr;			/**< the virtual adress of the allocated kernel memory*/
};

void pcilib_software_registers_close(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx) {
	if (((pcilib_software_register_bank_context_t*)bank_ctx)->kmem)
	    pcilib_free_kernel_memory(ctx, ((pcilib_software_register_bank_context_t*)bank_ctx)->kmem, PCILIB_KMEM_FLAG_REUSE);
	free(bank_ctx);
}

pcilib_register_bank_context_t* pcilib_software_registers_open(pcilib_t *ctx, pcilib_register_bank_t bank, const char* model, const void *args) {
	int err;
	pcilib_software_register_bank_context_t *bank_ctx;
	pcilib_kmem_handle_t *handle;
	pcilib_lock_t *lock;
	pcilib_kmem_reuse_state_t reused;

	const pcilib_register_bank_description_t *bank_desc = ctx->banks + bank;
	
	if (bank_desc->size > PCILIB_KMEM_PAGE_SIZE) {
	    pcilib_error("Currently software register banks are limited to %lu bytes, but %lu requested", PCILIB_KMEM_PAGE_SIZE, bank_desc->size);
	    return NULL;
	}

	bank_ctx = calloc(1, sizeof(pcilib_software_register_bank_context_t));
	if (!bank_ctx) {
	    pcilib_error("Memory allocation for bank context has failed");
	    return NULL;
	}

	/*we get a lock to protect the creation of the kernel space for software registers against multiple creations*/
	lock = pcilib_get_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, "softreg/%s", bank_desc->name);
	if (!lock) {
	    pcilib_software_registers_close(ctx, (pcilib_register_bank_context_t*)bank_ctx);
	    pcilib_error("Failed to initialize a lock to protect bank %s with software registers", bank_desc->name);
	    return NULL;
	}
	
	err = pcilib_lock(lock);
	if (err) {
	    pcilib_return_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, lock);
	    pcilib_software_registers_close(ctx, (pcilib_register_bank_context_t*)bank_ctx);
	    pcilib_error("Error (%i) obtaining lock on bank %s with software registers", err, bank_desc->name);
	    return NULL;
	}

	/*creation of the kernel space*/
	handle = pcilib_alloc_kernel_memory(ctx, PCILIB_KMEM_TYPE_PAGE, 1, PCILIB_KMEM_PAGE_SIZE, 0, PCILIB_KMEM_USE(PCILIB_KMEM_USE_SOFTWARE_REGISTERS, bank_desc->addr), PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_PERSISTENT);
	if (!handle) {
	    pcilib_unlock(lock);
	    pcilib_return_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, lock);
	    pcilib_software_registers_close(ctx, (pcilib_register_bank_context_t*)bank_ctx);
	    pcilib_error("Allocation of kernel memory for software registers has failed");
	    return NULL;
	}
	
	bank_ctx->kmem = handle;
	bank_ctx->addr = pcilib_kmem_get_block_ua(ctx, handle, 0);
	reused = pcilib_kmem_is_reused(ctx, handle);

	if ((reused & PCILIB_KMEM_REUSE_REUSED) == 0) {
	    pcilib_register_t i;

	    if (reused & PCILIB_KMEM_REUSE_PARTIAL) {
		pcilib_unlock(lock);
		pcilib_return_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, lock);
		pcilib_software_registers_close(ctx, (pcilib_register_bank_context_t*)bank_ctx);
		pcilib_error("Inconsistent software registers are found (only part of required buffers is available)");
		return NULL;
	    }

	/* here we fill the software registers with their default value*/		
	    for (i = 0; ctx->model_info.registers[i].name != NULL; i++) {
		if ((ctx->model_info.registers[i].bank == ctx->banks[bank].addr)&&(ctx->model_info.registers[i].type == PCILIB_REGISTER_STANDARD)) {
		    *(pcilib_register_value_t*)(bank_ctx->addr + ctx->model_info.registers[i].addr) = ctx->model_info.registers[i].defvalue;
		}
	    }
	}

	pcilib_unlock(lock);
	pcilib_return_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, lock);

	return (pcilib_register_bank_context_t*)bank_ctx;
}

uintptr_t pcilib_software_registers_resolve(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_address_resolution_flags_t flags, pcilib_register_addr_t addr) {
    if (addr == PCILIB_REGISTER_ADDRESS_INVALID) addr = 0;

    switch (flags&PCILIB_ADDRESS_RESOLUTION_MASK_ADDRESS_TYPE) {
     case 0:
        return (uintptr_t)((pcilib_software_register_bank_context_t*)bank_ctx)->addr + addr;

     case PCILIB_ADDRESS_RESOLUTION_FLAG_PHYS_ADDRESS:
	return pcilib_kmem_get_block_pa(ctx, ((pcilib_software_register_bank_context_t*)bank_ctx)->kmem, 0) + addr;
    }

    return PCILIB_ADDRESS_INVALID;
}


int pcilib_software_registers_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t *value){
    const pcilib_register_bank_description_t *b = bank_ctx->bank;
    int access = b->access / 8;

    pcilib_register_value_t val = 0;

    if ((addr + sizeof(pcilib_register_value_t)) > bank_ctx->bank->size) {
	pcilib_error("Trying to access space outside of the define register bank (bank: %s, addr: 0x%lx)", bank_ctx->bank->name, addr);
	return PCILIB_ERROR_INVALID_ADDRESS;
    }

    pcilib_datacpy(&val, (void*)((pcilib_software_register_bank_context_t*)bank_ctx)->addr + addr, access, 1, b->raw_endianess);
    *value = val;

    return 0;
}

int pcilib_software_registers_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t value) {
    const pcilib_register_bank_description_t *b = bank_ctx->bank;
    int access = b->access / 8;

    if ((addr + sizeof(pcilib_register_value_t)) > bank_ctx->bank->size) {
	pcilib_error("Trying to access space outside of the define register bank (bank: %s, addr: 0x%lx)", bank_ctx->bank->name, addr);
	return PCILIB_ERROR_INVALID_ADDRESS;
    }

    // we consider this atomic operation and, therefore, do no locking
    pcilib_datacpy((void*)((pcilib_software_register_bank_context_t*)bank_ctx)->addr + addr, &value, access, 1, b->raw_endianess);

    return 0;
}
