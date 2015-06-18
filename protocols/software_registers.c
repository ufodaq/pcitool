#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include "model.h"
#include "error.h"
#include "kmem.h"
#include "pcilib.h"
#include "pci.h"

#include <stdio.h>

/**
 * pcilib_software_registers_open
 * this function initializes the kernel space memory and stores in it the default values of the registers of the given bank index, if it was not initialized by a concurrent process, and return a bank context containing the adress of this kernel space. It the kernel space memory was already initialized by a concurrent process, then this function just return the bank context with the adress of this kernel space already used
 *@param[in] ctx the pcilib_t structure running
 * @param[in] bank the bank index that will permits to get the bank we want registers from
 * @param[in] model not used
 * @param[in] args not used
 *@return a bank context with the adress of the kernel space memory related to it
 */
pcilib_register_bank_context_t* pcilib_software_registers_open(pcilib_t *ctx, pcilib_register_bank_t bank,const char* model, const void *args){
	pcilib_register_bank_context_t* bank_ctx;
	pcilib_kmem_handle_t *handle;
	int j;
	/* the protocol thing is here to make sure to avoid segfault in write_registers_internal, but is not useful as it is now*/
	pcilib_register_protocol_t protocol;
	protocol = pcilib_find_register_protocol_by_addr(ctx, ctx->banks[bank].protocol);
	
	bank_ctx=calloc(1,sizeof(pcilib_register_bank_context_t));
	bank_ctx->bank=ctx->banks + bank;
	bank_ctx->ctx=ctx;
	bank_ctx->api=ctx->protocols[protocol].api;
	ctx->bank_ctx[bank]=bank_ctx;

	handle=pcilib_alloc_kernel_memory(ctx, PCILIB_KMEM_TYPE_PAGE, 1, 0, 1,PCILIB_KMEM_USE_STANDARD,PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_PERSISTENT);

	if (!handle)pcilib_error("allocation of kernel memory for registers has failed");
	
	bank_ctx->kmem_base_address=handle;

	if(pcilib_kmem_is_reused(ctx,handle)!= PCILIB_KMEM_REUSE_ALLOCATED){
		j=0;
		while(ctx->model_info.registers[j].name!=NULL){
		  if(ctx->model_info.registers[j].bank==(ctx->banks+bank)->addr){
		/* !!!!!warning!!!!! 
		hey suren,
		you may check here too  :the programm seems to always go this path, so pcilib_write_register_by_id always write the original value, kmem_is_reused working?
		*/
		pcilib_write_register_by_id(ctx,j,ctx->model_info.registers[j].defvalue);
			
			}	
			j++;
		}		
	}

	return bank_ctx;
}


/**
 * pcilib_software_registers_close
 * this function clear the kernel memory space that could have been allocated for software registers
 * @param[in] bank_ctx the bank context running that we get from the initialisation function
 */	
void pcilib_software_registers_close(pcilib_register_bank_context_t *bank_ctx){
	/*!!!!! to check!!!!
	
	ps: i am using uint32_t to calculate registers adress, may it change in the future? should we define some #define?	
	*/
	pcilib_free_kernel_memory(bank_ctx->ctx,bank_ctx->kmem_base_address, 0);
}

/**
 * pcilib_software_registers_read
 * this function read the value of a said register in the kernel space
 * @param[in] ctx the pcilib_t structure runnning
 * @param[in] bank_ctx the bank context that was returned by the initialisation function
 * @param[in] addr the adress of the register we want to read
 *@param[out] value the value of the register
 * @return 0 in case of success
 */
int pcilib_software_registers_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx,pcilib_register_addr_t addr, pcilib_register_value_t *value){
	int i;
	void* base_addr;
	
	base_addr=pcilib_kmem_get_block_ua(ctx,bank_ctx->kmem_base_address,0);
	for(i=0;ctx->registers[i].name!=NULL;i++){
		if(ctx->registers[i].addr==addr) break;
	}
	*value=*(pcilib_register_value_t*)(base_addr+i*sizeof(uint32_t));
	return 0;
}

/**
 * pcilib_software_registers_write
 * this function write the said value to a said register in the kernel space
 * @param[in] ctx the pcilib_t structure runnning
 * @param[in] bank_ctx the bank context that was returned by the initialisation function
 * @param[in] addr the adress of the register we want to write in
 *@param[out] value the value we want to write in the register
 * @return 0 in case of success
 */
int pcilib_software_registers_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t value){
	int i;
	void* base_addr;
	
	base_addr=pcilib_kmem_get_block_ua(ctx,bank_ctx->kmem_base_address,0);
	for(i=0;ctx->registers[i].name!=NULL;i++){
		if(ctx->registers[i].addr==addr) break;
	}
	*(pcilib_register_value_t*)(base_addr+i*sizeof(uint32_t))=value;
	return 0;
}
