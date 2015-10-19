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
    xmlSchemaPtr schema;  				/**< Pointer to the parsed xsd schema */
    xmlSchemaValidCtxtPtr validator;       		/**< Pointer to the XML validation context */
    xmlSchemaPtr parts_schema; 				/**< Pointer to the parsed xsd schema capable of validating individual XML files - no check for cross-references */
    xmlSchemaValidCtxtPtr parts_validator;     		/**< Pointer to the XML validation context capable of validating individual XML files - no check for cross-references */

    xmlNodePtr bank_nodes[PCILIB_MAX_REGISTER_BANKS];	/**< pointer to xml nodes of banks in the xml file */
};

#ifdef __cplusplus
extern "C" {
#endif

/** pcilib_init_xml
 * Initializes XML stack and loads a default set of XML files. The default location for model XML files is
 * /usr/local/share/pcilib/models/<current_model>. This can be altered using CMake PCILIB_MODEL_DIR variable
 * while building or using PCILIB_MODEL_DIR environmental variable dynamicly.  More XML files can be added
 * later using pcilib_process_xml call.
 * @param[in,out] ctx - pcilib context
 * @param[in] model - the name of the model
 * @return - error or 0 on success
 */
int pcilib_init_xml(pcilib_t *ctx, const char *model);

/** pcilib_free_xml
 * this function free the xml parts of the pcilib_t running, and some libxml ashes
 * @param[in] ctx the pcilib_t running
 */
void pcilib_free_xml(pcilib_t *ctx);

/** pcilib_process_xml
 * Processes a bunch of XML files in the specified directory. During the initialization, all XML files
 * in the corresponding model directory will be loaded. This function allows to additionally load XML 
 * files from the specified subdirectories of the model directory. I.e. the XML files from the 
 * /usr/local/share/pcilib/models/<current_model>/<location> will be loaded. As with pcilib_init_xml,
 * the directory can be adjusted using CMake build configuration or PCILIB_MODEL_DIR environmental
 * variable.
 * @param[in] ctx - pcilib context
 * @param[in] location - Specifies sub-directory with XML files relative to the model directory.
 * @return - error or 0 on success
 */
int pcilib_process_xml(pcilib_t *ctx, const char *location);

/** pcilib_get_xml_attr
 * This is an internal function which returns a specified node attribute in the pcilib_value_t structure.
 * This function should not be used directly. Instead subsystem specific calls like pcilib_get_register_attr,
 * pcilib_get_property_attr, ...have to be used.
 * @param[in] ctx - pcilib context
 * @param[in] node - LibXML2 node
 * @param[in] attr - attribute name
 * @param[out] val - the result will be returned in this variable. Prior to first usage pcilib_value_t variable should be initalized to 0.
 * @return - error or 0 on success
 */
int pcilib_get_xml_attr(pcilib_t *ctx, pcilib_xml_node_t *node, const char *attr, pcilib_value_t *val);


#ifdef __cplusplus
}
#endif

#endif /*_PCILIB_XML_H */
