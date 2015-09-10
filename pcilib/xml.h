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

#ifndef _PCILIB_XML_H
#define _PCILIB_XML_H

#include <pcilib.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xmlschemas.h>

#define PCILIB_MAX_MODEL_FILES 32

typedef struct pcilib_xml_s pcilib_xml_t;

struct pcilib_xml_s {
    size_t num_files;					/**< Number of currently loaded XML documents */
    
    xmlDocPtr docs[PCILIB_MAX_MODEL_FILES];		/**< Pointers to parsed XML documents */
    xmlXPathContextPtr xpath[PCILIB_MAX_MODEL_FILES];	/**< Per-file XPath context */
    
    xmlParserCtxtPtr parser;				/**< Pointer to the XML parser context */
    xmlSchemaPtr schema;    				/**< Pointer to the parsed xsd schema */
    xmlSchemaValidCtxtPtr validator;       		/**< Pointer to the XML validation context */

    xmlNodePtr bank_nodes[PCILIB_MAX_REGISTER_BANKS];	/**< pointer to xml nodes of banks in the xml file */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * this function gets the xml files and validates them, before filling the pcilib_t struct with the registers and banks of those files
 *@param[in,out] ctx the pcilib_t struct running that gets filled with banks and registers
 *@param[in] model the name of the model
 */
int pcilib_init_xml(pcilib_t *ctx, const char *model);

/** pcilib_free_xml
 * this function free the xml parts of the pcilib_t running, and some libxml ashes
 * @param[in] ctx the pcilib_t running
*/
void pcilib_free_xml(pcilib_t *ctx);


/** pcilib_process_xml
 * this function free the xml parts of the pcilib_t running, and some libxml ashes
 * @param[in] ctx the pcilib_t running
 * @param[in] location of XML files relative to the PCILIB_MODEL_DIR
*/
int pcilib_process_xml(pcilib_t *ctx, const char *location);

#ifdef __cplusplus
}
#endif

#endif /*_PCILIB_XML_H */
