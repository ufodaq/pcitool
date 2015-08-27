/** 
 * @file xml.h 
 * @version 1.0
 * @brief header file to support of xml configuration.
 *
 * @details    this file is the header file for the implementation of dynamic registers using xml and several funtionalities for the "pci-tool" line command tool from XML files. the xml part has been implemented using libxml2.
 * 
 *      this code was meant to be evolutive as the XML files evolute. In this meaning, most of the xml parsing is realized with XPath expressions(when it was possible), so that changing the xml xould result mainly in changing the XPAth pathes present here.
 * @todo cf compilation chain
 */

#ifndef _XML_
#define _XML_

//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <assert.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "pcilib.h"
#include "register.h"
#include "model.h"
#include "bank.h"
#include "pci.h"
//#include <Python.h>

#define REGISTERS_PATH ((xmlChar*)"/model/banks/bank/registers/register") /**<all standard registers nodes.*/

#define ADRESS_PATH ((xmlChar*)"/model/banks/bank/registers/register/adress") /**<adress path for standard registers.*/
#define OFFSET_PATH ((xmlChar*)"/model/banks/bank/registers/register/offset") /**<offset path for standard registers.*/
#define SIZE_PATH ((xmlChar*)"/model/banks/bank/registers/register/size") /**<size path for standard registers.*/
#define DEFVALUE_PATH ((xmlChar*)"/model/banks/bank/registers/register/default") /**<defvalue path for standard registers.*/
#define RWMASK_PATH ((xmlChar*)"/model/banks/bank/registers/register/rwmask") /**<rwmask path for standard registers.*/
#define MODE_PATH ((xmlChar*)"/model/banks/bank/registers/register/mode") /**<mode path for standard registers.*/
#define NAME_PATH ((xmlChar*)"/model/banks/bank/registers/register/name") /**<name path for standard registers.*/
#define DESCRIPTION_PATH ((xmlChar*)"/model/banks/bank/registers/register/description") /**<description. not used, is searched recursively as it may not appearing in xml. */

#define SUB_OFFSET_PATH ((xmlChar*)"/model/banks/bank/registers/register/registers_bits/register_bits/offset") /**<offset path for bits registers.*/
#define SUB_SIZE_PATH ((xmlChar*)"/model/banks/bank/registers/register/registers_bits/register_bits/size") /**<size path for bits registers.*/
#define SUB_RWMASK_PATH ((xmlChar*)"/model/banks/bank/registers/register/registers_bits/register_bits/sub_rwmask") /**<rwmask path for bits registers. not used, as the value is meant to be the same as the standard register this register depends on.*/
#define SUB_MODE_PATH ((xmlChar*)"/model/banks/bank/registers/register/registers_bits/register_bits/mode") /**<size path for bits registers.*/
#define SUB_NAME_PATH ((xmlChar*)"/model/banks/bank/registers/register/registers_bits/register_bits/name") /**<name path for bits registers.*/
#define SUB_DESCRIPTION_PATH ((xmlChar*)"/model/banks/bank/registers/register/registers_bits/register_bits/adress") /**<description path for bits registers. not used, is searched recursively as it may not appear in the xml.*/

#define VIEW_FORMULA_PATH ((xmlChar*)"/model/views/view[@type=\"formula\"]") /**< the complete node path for views of type formula.*/
#define VIEW_FORMULA_FORMULA_PATH ((xmlChar*)"/model/views/view[@type=\"formula\"]/read_from_register") /**< path to formula in direction read_from_register for views of type formula.*/
#define VIEW_FORMULA_REVERSE_FORMULA_PATH ((xmlChar*)"/model/views/view[@type=\"formula\"]/write_to_register") /**<  path to formula in direction write_to_register for views of type formula.*/
#define VIEW_FORMULA_UNIT_PATH ((xmlChar*)"/model/views/view[@type=\"formula\"]/unit") /**< path to the base unit of the view of type formula. */
#define VIEW_FORMULA_NAME_PATH ((xmlChar*)"/model/views/view[@type=\"formula\"]/name") /**<path to the name of the view of type formula.*/

#define VIEW_ENUM_PATH ((xmlChar*)"/model/views/view[@type=\"enum\"]") /**< the complete node path for views of type enum.*/

#define VIEW_BITS_REGISTER ((xmlChar*)"/model/banks/bank/registers/register/registers_bits/register_bits/views/view") /**< @warning not used normally.*/ 
#define VIEW_STANDARD_REGISTER ((xmlChar*)"/model/banks/bank/registers/register/views/view") /**< @warning not used too.*/
#define VIEW_STANDARD_REGISTER_PLUS ((char*)"/model/banks/bank/registers/register/views/view[.=\"%s\"]") /**< path to the standard registers that depend on the given view.*/
#define VIEW_BITS_REGISTER_PLUS ((char*)"/model/banks/bank/registers/register/registers_bits/register_bits/views/view[.=\"%s\"]") /**< path to the bits registers that depend on the given view.*/

#define BANKS_PATH ((xmlChar*)"/model/banks/bank/bank_description") /**< path to complete nodes of banks.*/
#define BANK_ADDR_PATH ((xmlChar*)"/model/banks/bank/bank_description/adress") /**<path to adress of banks.*/
#define BANK_BAR_PATH ((xmlChar*)"/model/banks/bank/bank_description/bar") /**<path to bar of banks.*/
#define BANK_SIZE_PATH ((xmlChar*)"/model/banks/bank/bank_description/size") /**<path to size of banks.*/
#define BANK_PROTOCOL_PATH ((xmlChar*)"/model/banks/bank/bank_description/protocol") /**< path to protocol of banks.*/
#define BANK_READ_ADDR_PATH ((xmlChar*)"/model/banks/bank/bank_description/read_adress") /**<path to read_addr of banks.*/
#define BANK_WRITE_ADDR_PATH ((xmlChar*)"/model/banks/bank/bank_description/write_adress") /**<path to write_addr of banks.*/
#define BANK_RAW_ENDIANESS_PATH ((xmlChar*)"/model/banks/bank/bank_description/raw_endianess") /**<path to raw_endianess of banks.*/
#define BANK_ACCESS_PATH ((xmlChar*)"/model/banks/bank/bank_description/word_size") /**<path to word_size of banks.*/
#define BANK_ENDIANESS_PATH ((xmlChar*)"/model/banks/bank/bank_description/endianess") /**<path to endianess of banks.*/
#define BANK_FORMAT_PATH ((xmlChar*)"/model/banks/bank/bank_description/format") /**<path to format of banks.*/
#define BANK_NAME_PATH ((xmlChar*)"/model/banks/bank/bank_description/name") /**< path to name of banks.*/
#define BANK_DESCRIPTION_PATH ((xmlChar*)"/model/banks/bank/bank_description/description") /**<path to description of banks.*/
 

/**
 * this path get the units in the units xml file
 */
#define BASE_UNIT_PATH ((char*)"/units/unit")

/**
 * this path permits to get the possible units one given unit can be converted into in the xml.
 */
#define TRANSFORM_UNIT_PATH ((char*)"/units/unit[@name=\"%s\"]/convert_unit")


/**
* this function takes a string and will create an abtract syntax tree from the xml file represented by the string.
* @param[in] filename the name of the xml file containing registers and banks.
* @return an AST corresponding to the xml file.
*/  
xmlDocPtr pcilib_xml_getdoc(char* filename);

/**
* this function takes a context from an AST and an XPath expression, to produce a XPath object containing the nodes corresponding to the xpath expression.
* @param[in] doc the xpath context of the xml file.
* @param[in] xpath the value of the xpath expression that will be computed against the context.
* @return the Xpath object with the nodes we wanted.
*/
xmlXPathObjectPtr pcilib_xml_getsetproperty(xmlXPathContextPtr doc, xmlChar *xpath);

/**
 * this function create the list of registers structures, that are manipulated during execution, from the xml file.
 * @param[in] doc the xpath context of the xml file.
 * @param[out] registers out: the list of the created registers.
 */
void pcilib_xml_initialize_registers(pcilib_t* pci, xmlDocPtr doc,pcilib_register_description_t *registers);

/**
 * this function get the numbers of registers in the xml file for further malloc and xml checking.
 * @param[in] doc the xpath context of the xml file.
 * @return the numbers of registers in xml file.
 * @todo see the usage of this function.
 */
int pcilib_xml_getnumberregisters(xmlXPathContextPtr doc);

/**
 * this function rearrange the list of registers to put them in pcilib_open furthermore, using a merge sort algorithm. The merge sort algorithm was chosen as it's fast and stable, however it uses some memory, but it's not limitating here. 
 * @param[in,out] registers the list of registers in : not ranged out: ranged.
 * @param[in] size the number of registers.
 */
void pcilib_xml_arrange_registers(pcilib_register_description_t *registers,int size);

/**
 * this functions initialize the structures containing banks, for use in the rest of execution, from the xml file.
 * @see pcilib_xml_create_bank(pcilib_register_bank_description_t *mybank,xmlChar* adress, xmlChar *bar, xmlChar *size, xmlChar *protocol,xmlChar *read_addr, xmlChar *write_addr, xmlChar *access, xmlChar *endianess, xmlChar *format, xmlChar *name,xmlChar *description, xmlNodePtr node).
 * @param[in] doc the AST of the xml file.
 * @param[in,out] mybanks the structure containing the banks.
 */
void pcilib_xml_initialize_banks(pcilib_t* pci,xmlDocPtr doc, pcilib_register_bank_description_t* mybanks);

/**
 * this function create a bank from the informations gathered in the xml.
 * @param[out] mybank the created bank.
 * @param[in] adress the adress of the bank that will be created.
 * @param[in] bar the bar of the bank that will be created.
 * @param[in] size the size of the bank that will be created.
 * @param[in] protocol the protocol of the bank that will be created.
 * @param[in] read_addr the read adress for protocol of the bank that will be created.
 * @param[in] write_addr the write adress for the protocol of the bank that will be created.
 * @param[in] access the word size of the bank that will be created.
 * @param[in] endianess the endianess of the bank that will be created.
 * @param[in] format the format of the bank that will be created.
 * @param[in] name the name of the bank that will be created.
 * @param[in] description the description of the bank that will be created.
 * @param[in] node the xmlNodeptr referring to the bank_description node of the bank that will be created.
 *
void pcilib_xml_create_bank(pcilib_register_bank_description_t *mybank,xmlChar* adress, xmlChar *bar, xmlChar *size, xmlChar *protocol,xmlChar *read_addr, xmlChar *write_addr, xmlChar *access, xmlChar *endianess, xmlChar *format, xmlChar *name,xmlChar *description, xmlNodePtr node);
*/

/**
 * this function get the numbers of banks in the xml file for further malloc and xml checking.
 * @param[in] doc the Xpath context of the xml file.
 * @return the number of banks.
 *@todo see the usage of this function.
 */
int pcilib_xml_getnumberbanks(xmlXPathContextPtr doc);

/**
* this function read the config file of the pcitool tool to give back the pwd of diverse files like the xml file to treat, the xsd file, the pythonscript file, the units xml file, the units xsd file.
* @param[in,out] xmlfile the string representating a pwd to a file we want to access in:uninitilized out: the pwd.
* @param[in] i the line at which the function should read the file to get the pwd.
*/
void pcilib_xml_read_config(char** xmlfile, int i);

/**
* this function validates the xml file against the xsd schema defined as the reference.
* @todo change the name of the function if accepted.
* @todo validation of unit file too?
*/
void validation();


/**
* this function create an XPath context (ie some sort of special AST only for XPath) from the AST of the xml file.
* @param[in] doc the AST of the xml file.
*/
xmlXPathContextPtr pcilib_xml_getcontext(xmlDocPtr doc);

/**
* this function is used to register the xpath object containing all registers node in registers context.
* @param[in] ctx the register context running.
*/
void pcilib_xml_init_nodeset_register_ctx(pcilib_register_context_t *ctx);

/**
* this function is used to register the xpath object containing all bankss node in banks context.
* @param[in] ctx the bank context running.
*/
void pcilib_xml_init_nodeset_bank_ctx(pcilib_register_bank_context_t *ctx);

/**
 * this function create a register structure from the results of xml parsing.
 * @param[out] myregister the register we want to create
 * @param[in] adress the adress of the future register
 * @param[in] offset the offset of the future register
 * @param[in] size the size of the future register
 * @param[in] defvalue the defaut value of the future register
 * @param[in] rwmask the rwmask of the future register
 * @param[in] mode the mode of the future register
 * @param[in] type the type of the future register
 * @param[in] bank the bank of the future register
 * @param[in]  name the name of the future register
 * @param[in] description the description of the future register
 * @param[in] node the current xmlNode in the xml of the future register
 *
 void pcilib_xml_create_register(pcilib_register_description_t *myregister,xmlChar* adress, xmlChar *offset, xmlChar *size, xmlChar *defvalue, xmlChar *rwmask, xmlChar *mode, xmlChar *type, xmlChar *bank, xmlChar *name, xmlChar *description, xmlNodePtr node);*/

#endif /*_XML_*/
