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
 * this function create the list of registers structures, that are manipulated during execution, from the xml file.
 * @param[in] doc the xpath context of the xml file.
 * @param[out] registers out: the list of the created registers.
 */
void pcilib_xml_initialize_registers(pcilib_t* pci, xmlDocPtr* doc);

/**
 * this functions initialize the structures containing banks, for use in the rest of execution, from the xml file.
 * @see pcilib_xml_create_bank(pcilib_register_bank_description_t *mybank,xmlChar* adress, xmlChar *bar, xmlChar *size, xmlChar *protocol,xmlChar *read_addr, xmlChar *write_addr, xmlChar *access, xmlChar *endianess, xmlChar *format, xmlChar *name,xmlChar *description, xmlNodePtr node).
 * @param[in] doc the AST of the xml file.
 * @param[in,out] mybanks the structure containing the banks.
 */
void pcilib_xml_initialize_banks(pcilib_t* pci,xmlDocPtr* doc);

/**
 * this function gets the xml files and validates them, before returning pre-AST of them to initialize functions
 *@param[out] docs the list of pre-AST of xml files parsed
 */
void pcilib_init_xml(xmlDocPtr* docs);

#endif /*_XML_*/
