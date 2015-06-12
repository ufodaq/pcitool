#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include "model.h"
#include "error.h"
#include "kmem.h"
#include "pcilib.h"
#include "pci.h"


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
	pcilib_register_value_t *init=NULL;
	pcilib_kmem_handle_t *test;
	int i;
	int j;
	
	bank_ctx=calloc(1,sizeof(pcilib_register_bank_context_t));
	bank_ctx->bank=ctx->banks + bank;
	
	test=pcilib_alloc_kernel_memory(ctx, 0, 0, 0, 0,PCILIB_KMEM_USE_STANDARD,PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_PERSISTENT);
	if (!test)pcilib_error("allocation of kernel memory for registers has failed");

	if(pcilib_kmem_is_reused(ctx,test)== PCILIB_KMEM_REUSE_ALLOCATED){
		bank_ctx->bank_software_register_adress=test;
	}else{
		bank_ctx->bank_software_register_adress=test;
		init=test;		
		j=0;
		while(ctx->model_info.registers[j].name!=NULL){
		  if(ctx->model_info.registers[j].bank==(ctx->banks + bank).addr){
		    pcilib_write_register_by_id(ctx,ctx->model_info.registers[j],ctx->model_info.registers[j].defvalue);
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
	int err=1;

	err=pcilib_clean_kernel_memory(bank_ctx->ctx, 0, 0);
	
	if(err) pcilib_error("Error closing register kernel space");
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
  *value= *(pcilib_register_value_t*) bank_ctx->bank_software_register_adress+addr;
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
	*(pcilib_register_value_t*)(bank_ctx->bank_software_register_adress+addr)=value;
	return 0;
}
