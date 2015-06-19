#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>

#include "model.h"
#include "error.h"
#include "kmem.h"
#include "pcilib.h"
#include "pci.h"

typedef struct pcilib_software_register_bank_context_s pcilib_software_register_bank_context_t;

struct pcilib_software_register_bank_context_s {
    pcilib_register_bank_context_t bank_ctx;

    pcilib_kmem_handle_t *kmem;
    void *addr;
};

/**
 * pcilib_software_registers_close
 * this function clear the kernel memory space that could have been allocated for software registers
 * @param[in] bank_ctx the bank context running that we get from the initialisation function
 */	
void pcilib_software_registers_close(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx) {
	if (((pcilib_software_register_bank_context_t*)bank_ctx)->kmem)
	    pcilib_free_kernel_memory(ctx, ((pcilib_software_register_bank_context_t*)bank_ctx)->kmem, PCILIB_KMEM_FLAG_REUSE);
	free(bank_ctx);
}

/**
 * pcilib_software_registers_open
 * this function initializes the kernel space memory and stores in it the default values of the registers of the given bank index, if it was not initialized by a concurrent process, and return a bank context containing the adress of this kernel space. It the kernel space memory was already initialized by a concurrent process, then this function just return the bank context with the adress of this kernel space already used
 * @param[in] ctx the pcilib_t structure running
 * @param[in] bank the bank index that will permits to get the bank we want registers from
 * @param[in] model not used
 * @param[in] args not used
 * @return a bank context with the adress of the kernel space memory related to it
 */
pcilib_register_bank_context_t* pcilib_software_registers_open(pcilib_t *ctx, pcilib_register_bank_t bank, const char* model, const void *args) {
	pcilib_software_register_bank_context_t *bank_ctx;
	pcilib_kmem_handle_t *handle;
	pcilib_kmem_reuse_state_t reused;

	const pcilib_register_bank_description_t *bank_desc = ctx->banks + bank;
	
	if (bank_desc->size > PCILIB_KMEM_PAGE_SIZE) {
	    pcilib_error("Currently software register banks are limited to %lu bytes, but %lu requested", PCILIB_KMEM_PAGE_SIZE, bank_desc->size);
	    return NULL;
	}

	bank_ctx = calloc(1, sizeof(pcilib_software_register_bank_context_t));

	handle = pcilib_alloc_kernel_memory(ctx, PCILIB_KMEM_TYPE_PAGE, 1, PCILIB_KMEM_PAGE_SIZE, 0, PCILIB_KMEM_USE(PCILIB_KMEM_USE_SOFTWARE_REGISTERS, bank), PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_PERSISTENT);
	if (!handle) {
	    pcilib_error("Allocation of kernel memory for software registers has failed");
	    pcilib_software_registers_close(ctx, (pcilib_register_bank_context_t*)bank_ctx);
	    return NULL;
	}
	
	bank_ctx->kmem = handle;
	bank_ctx->addr = pcilib_kmem_get_block_ua(ctx, handle, 0);
	reused = pcilib_kmem_is_reused(ctx, handle);

	if ((reused & PCILIB_KMEM_REUSE_REUSED) == 0) {
	    pcilib_register_t i;

	    if (reused & PCILIB_KMEM_REUSE_PARTIAL) {
		pcilib_error("Inconsistent software registers are found (only part of required buffers is available)");
		pcilib_software_registers_close(ctx, (pcilib_register_bank_context_t*)bank_ctx);
		return NULL;
	    }
		
	    for (i = 0; ctx->model_info.registers[i].name != NULL; i++) {
		if ((ctx->model_info.registers[i].bank == ctx->banks[bank].addr)&&(ctx->model_info.registers[i].type == PCILIB_REGISTER_STANDARD)) {
		    *(pcilib_register_value_t*)(bank_ctx->addr + ctx->model_info.registers[i].addr) = ctx->model_info.registers[i].defvalue;
		}
	    }
	}

	return (pcilib_register_bank_context_t*)bank_ctx;
}

/**
 * pcilib_software_registers_read
 * this function read the value of a said register in the kernel space
 * @param[in] ctx the pcilib_t structure runnning
 * @param[in] bank_ctx the bank context that was returned by the initialisation function
 * @param[in] addr the adress of the register we want to read
 * @param[out] value the value of the register
 * @return 0 in case of success
 */
int pcilib_software_registers_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t *value){
	if ((addr + sizeof(pcilib_register_value_t)) > bank_ctx->bank->size) {
	    pcilib_error("Trying to access space outside of the define register bank (bank: %s, addr: 0x%lx)", bank_ctx->bank->name, addr);
	    return PCILIB_ERROR_INVALID_ADDRESS;
	}
	
	*value = *(pcilib_register_value_t*)(((pcilib_software_register_bank_context_t*)bank_ctx)->addr + addr);
	return 0;
}

/**
 * pcilib_software_registers_write
 * this function write the said value to a said register in the kernel space
 * @param[in] ctx the pcilib_t structure runnning
 * @param[in] bank_ctx the bank context that was returned by the initialisation function
 * @param[in] addr the adress of the register we want to write in
 * @param[out] value the value we want to write in the register
 * @return 0 in case of success
 */
int pcilib_software_registers_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t value){
	if ((addr + sizeof(pcilib_register_value_t)) > bank_ctx->bank->size) {
	    pcilib_error("Trying to access space outside of the define register bank (bank: %s, addr: 0x%lx)", bank_ctx->bank->name, addr);
	    return PCILIB_ERROR_INVALID_ADDRESS;
	}
	
	*(pcilib_register_value_t*)(((pcilib_software_register_bank_context_t*)bank_ctx)->addr + addr) = value;
	return 0;
}
