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

#include <libxml/tree.h>
#include "pcilib.h"

typedef struct{
  xmlDocPtr* docs;
}pcilib_xml_context_t;

/**
 * this function gets the xml files and validates them, before filling the pcilib_t struct with the registers and banks of those files
 *@param[in,out] pci the pcilib_t struct running that gets filled with banks and registers
 *@param[in] model the name of the model
 */
int pcilib_init_xml(pcilib_t* pci, char* model);

/** pcilib_clean_xml
 * this function free the xml parts of the pcilib_t running, and some libxml ashes
 * @param[in] pci the pcilib_t running
*/
void pcilib_clean_xml(pcilib_t* pci);

#endif /*_XML_*/
