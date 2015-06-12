/**
 * @file software_registers.h
 * @skip author nicolas zilio, nicolas.zilio@hotmail.fr
 * @brief header file for implementation of the protocol with registers registered in the kernel space.
 */

#ifndef _PCILIB_SOFTREG_PROT
#define _PCILIB_SOFTREG_PROT

#include "pcilib.h"
#include "version.h"
#include "model.h"

/**
 * this function initialize the kernel space memory for the use of software register. it initializes the kernel space memory and stores in it the default values of the registers of the given bank index, if it was not initialized by a concurrent process, and return a bank context containing the adress of this kernel space. If the kernel space memory was already initialized by a concurrent process, then this function just return the bank context with the adress of this kernel space already used
 *@param[in] ctx the pcilib_t structure running
 * @param[in] bank the bank index that will permits to get the bank we want registers from
 * @param[in] model not used
 * @param[in] args not used
 *@return a bank context with the adress of the kernel space memory related to it
 */
pcilib_register_bank_context_t* pcilib_software_registers_open(pcilib_t *ctx, pcilib_register_bank_t bank, const char* model, const void *args);

/**
 * this function clear the kernel memory space that could have been allocated for software registers.
 * @param[in] bank_ctx the bank context running that we get from the initialisation function
 */
void pcilib_software_registers_close(pcilib_register_bank_context_t *bank_ctx);

/**
 * this function read the value of a said register in the kernel space.
 * @param[in] ctx the pcilib_t structure runnning
 * @param[in] bank_ctx the bank context that was returned by the initialisation function
 * @param[in] addr the adress of the register we want to read
 *@param[out] value the value of the register
 * @return 0 in case of success
 */
int  pcilib_software_registers_read(pcilib_t *ctx, pcilib_register_bank_context_t* bank_ctx,pcilib_register_addr_t addr, pcilib_register_value_t *value);

/**
 * this function write the said value to a said register in the kernel space
 * @param[in] ctx the pcilib_t structure runnning
 * @param[in] bank_ctx the bank context that was returned by the initialisation function
 * @param[in] addr the adress of the register we want to write in
 *@param[out] value the value we want to write in the register
 * @return 0 in case of success
 */
int pcilib_software_registers_write(pcilib_t *ctx,pcilib_register_bank_context_t* bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t value);

#ifdef _PCILIB_EXPORT_C
/**
 * new protocol addition to the protocol api.
 */
const pcilib_register_protocol_api_description_t pcilib_register_software_register_protocol_api =
  { PCILIB_VERSION, pcilib_software_registers_open, pcilib_software_registers_close,pcilib_software_registers_read, pcilib_software_registers_write }; /**< we add there the protocol to the list of possible protocols*/
#endif /* _PCILIB_EXPORT_C */

#endif /* _PCILIB_SOFTREG_PROT*/
