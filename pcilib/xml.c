/**
 * @file pcilib_xml.c
 * @version 1.0
 *
 * @brief this file is the main source file for the implementation of dynamic registers using xml and several funtionalities for the "pci-tool" line command tool from XML files. the xml part has been implemented using libxml2
 *
 * @details registers and banks nodes are parsed using xpath expression, but their properties is get by recursively getting all properties nodes. In this sense, if a property changes, the algorithm has to be changed, but if it's registers or banks nodes, just the xpath expression modification should be enough.

 Libxml2 considers blank spaces within the XML as node natively and this code as been produced considering blank spaces in the XML files. In case XML files would not contain blank spaces anymore, please change the code.

 In case the performance is not good enough, please consider the following : hard code of formulas
 */
#define _XOPEN_SOURCE 700
#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <errno.h>
#include <alloca.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


#include <libxml/xmlschemastypes.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "config.h"

#include "pci.h"
#include "bank.h"
#include "register.h"
#include "xml.h"
#include "error.h"
#include "views.h"

#define BANKS_PATH ((xmlChar*)"/model/banks/bank/bank_description") 						/**< path to complete nodes of banks.*/
//#define REGISTERS_PATH ((xmlChar*)"/model/banks/bank/registers/register") 					/**< all standard registers nodes.*/
//#define BIT_REGISTERS_PATH ((xmlChar*)"/model/banks/bank/registers/register/registers_bits/register_bits") 	/**< all bits registers nodes.*/
#define REGISTERS_PATH ((xmlChar*)"../registers/register") 			/**< all standard registers nodes.*/
#define BIT_REGISTERS_PATH ((xmlChar*)"./registers_bits/register_bits") 	/**< all bits registers nodes.*/
#define VIEWS_PATH ((xmlChar*)"/model/views/view") 						/**< path to complete nodes of views.*/


static char *pcilib_xml_bank_default_format = "0x%lx";

typedef struct {
    pcilib_register_description_t base;
    pcilib_register_value_t min, max;
} pcilib_xml_register_description_t;

/*
static xmlNodePtr pcilib_xml_get_parent_bank_node(xmlDocPtr doc, xmlNodePtr node) {
    xmlNodePtr bank_node;
    bank_node = node->parent->parent;
	// bank_description is always a first node according to the XSD schema
    return xmlFirstElementChild(bank_node);
    
}

static xmlNodePtr pcilib_xml_get_parent_register_node(xmlDocPtr doc, xmlNodePtr node) {
    return node->parent->parent;
}
*/

/**
 * get the associated units of a view
 * this function is maybe completekly useless : we need to decide if we iterate directly in ctx or n view when we want to apply a unit. (in the second choice of course keep it).
*
static void
pcilib_get_associated_units(pcilib_t* ctx, pcilib_view_formula_t* myview){
  int i,j,k=2;
  for(i=0;myview->units[0].other_units.name[0];i++){
      for(j=0;ctx->units[j].name[0];i++){
	if(!(strcasecmp(myview->units[0].other_units.name,ctx->units[i].name))){
	  myview.units=realloc(myview.units,k*sizeof(pcilib_unit_t));
	  myview.units[k-1]=ctx->units[i];
	  k++;
	}
      }
  }
  }*/
			  
/**
 * get the associated views of a register, to fill its register context
 */
static int
pcilib_get_associated_views(pcilib_t* ctx, const char* reg_name,xmlXPathContextPtr xpath,pcilib_register_t id,int mode){

  /* i find this solution pretty elegant, but what about perf?*/
  char* VIEWS_NAME_PATH_BITS="/model/banks/bank/registers/register[./name=\"%s\"]/registers_bits/register_bits/views/view";
  char* VIEWS_NAME_PATH="/model/banks/bank/registers/register[./name=\"%s\"]/views/view";
  char* path;
  xmlXPathObjectPtr nodes;
  xmlNodeSetPtr nodeset;
  char* view_name;

  /*we get first the nodes corresponding to the given register*/
  if(mode==1) path=malloc(strlen(VIEWS_NAME_PATH)+strlen(reg_name));
  else path=malloc(strlen(VIEWS_NAME_PATH_BITS)+strlen(reg_name));
  if(!(path)){
    pcilib_error("can't allocate memory for getting path to get associated views of %s",reg_name);
    return PCILIB_ERROR_MEMORY;
  }

  if(mode==1) sprintf(path,VIEWS_NAME_PATH,reg_name);
  else sprintf(path,VIEWS_NAME_PATH_BITS,reg_name);
  nodes = xmlXPathEvalExpression((xmlChar*)path, xpath);
  nodeset = nodes->nodesetval;
    if (!xmlXPathNodeSetIsEmpty(nodeset)) {
      int i,k,l;
      /*if we correctly get a nodeset, then we iterate through the nodeset to get all views, using their names*/
	for (i = 0; i < nodeset->nodeNr; i++) {
	  view_name=(char*)nodeset->nodeTab[i]->children->content;

	  /* if the view name obtained is for an enum view, we get all pcilib_view_enum_t corresponding to the register*/
	  for(k=0; ctx->enum_views[k].enums_list[0].value; k++){
	    if(!(strcasecmp(view_name, ctx->enum_views[k].name))){
	      ctx->register_ctx[id].enums=malloc(sizeof(pcilib_view_enum_t));
	      
	      if(!(ctx->register_ctx[id].enums)){
		pcilib_error("error allocating memory for enum views in register context %i",id);
		return PCILIB_ERROR_MEMORY;
	      }

	      /*!!!!!!!!!!this loop here is buggy*/
	      for(l=0; ctx->enum_views[k].enums_list[l].value;l++){
		ctx->register_ctx[id].enums=realloc(ctx->register_ctx[id].enums,(l+1)*sizeof(pcilib_view_enum_t));
		ctx->register_ctx[id].enums[l]=ctx->enum_views[k].enums_list[l];
		//		printf("names %s %s\n",ctx->register_ctx[id].enums[l].name,ctx->enum_views[k].enums_list[l].name);
	      }
	      return 0;
	    }
	  }

	  /*here it is for formula, i assume we have only one formula view per register*/
	  for(k=0; ctx->formula_views[k].name[0];k++){
	    if(!(strcasecmp(view_name,ctx->formula_views[k].name))){
	      
	      ctx->register_ctx[id].formulas=malloc(sizeof(pcilib_view_formula_t));
	      if(!(ctx->register_ctx[id].formulas)){
		pcilib_error("error allocating memory for formula views in register context %i",id);
		return PCILIB_ERROR_MEMORY;
	      }

	      ctx->register_ctx[id].formulas[0]=ctx->formula_views[k];
	      break;
	    }
	  }

	}
    }
 
    xmlXPathFreeObject(nodes);
    return 0;
}


static int pcilib_xml_parse_register(pcilib_t *ctx, pcilib_xml_register_description_t *xml_desc, xmlDocPtr doc, xmlNodePtr node, pcilib_register_bank_description_t *bdesc, int* views_ok) {
    pcilib_register_description_t *desc = (pcilib_register_description_t*)xml_desc;

    xmlNodePtr cur;
    char *value, *name;
    char *endptr;

    for (cur = node->children; cur != NULL; cur = cur->next) {
	if (!cur->children) continue;
	if (!xmlNodeIsText(cur->children)) continue;
	
	name = (char*)cur->name;
        value = (char*)cur->children->content;
        if (!value) continue;
	
        if (!strcasecmp((char*)name, "address")) {
    	    uintptr_t addr = strtol(value, &endptr, 0);
    	    if ((strlen(endptr) > 0)) {
    		pcilib_error("Invalid address (%s) is specified in the XML register description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
            desc->addr = addr;
        } else if(!strcasecmp(name, "offset")) {
    	    int offset = strtol(value, &endptr, 0);
	    if ((strlen(endptr) > 0)||(offset < 0)||(offset > (8 * sizeof(pcilib_register_value_t)))) {
    		pcilib_error("Invalid offset (%s) is specified in the XML register description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
            desc->offset = (pcilib_register_size_t)offset;
        } else if (!strcasecmp(name,"size")) {
    	    int size = strtol(value, &endptr, 0);
	    if ((strlen(endptr) > 0)||(size <= 0)||(size > (8 * sizeof(pcilib_register_value_t)))) {
    		pcilib_error("Invalid size (%s) is specified in the XML register description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
            desc->bits = (pcilib_register_size_t)size;
        } else if (!strcasecmp(name, "default")) {
    	    pcilib_register_value_t val  = strtol(value, &endptr, 0);
	    if ((strlen(endptr) > 0)) {
    		pcilib_error("Invalid default value (%s) is specified in the XML register description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
            desc->defvalue = val;
        } else if (!strcasecmp(name,"min")) {
    	    pcilib_register_value_t min = strtol(value, &endptr, 0);
	    if ((strlen(endptr) > 0)) {
    		pcilib_error("Invalid minimum value (%s) is specified in the XML register description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
    	    xml_desc->min = min;
        } else if (!strcasecmp(name, "max")) {
    	    pcilib_register_value_t max = strtol(value, &endptr, 0);
	    if ((strlen(endptr) > 0)) {
    		pcilib_error("Invalid minimum value (%s) is specified in the XML register description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
    	    xml_desc->max = max;
        } else if (!strcasecmp((char*)name,"rwmask")) {
            if (!strcasecmp(value, "all")) {
                desc->rwmask = PCILIB_REGISTER_ALL_BITS;
            } else if (!strcasecmp(value, "none")) {
                desc->rwmask = 0;
            } else {
    	        uintptr_t mask = strtol(value, &endptr, 0);
    		if ((strlen(endptr) > 0)) {
    		    pcilib_error("Invalid mask (%s) is specified in the XML register description", value);
    		    return PCILIB_ERROR_INVALID_DATA;
    		}
        	desc->rwmask = mask;
    	    }
        } else if (!strcasecmp(name, "mode")) {
            if (!strcasecmp(value, "R")) {
                desc->mode = PCILIB_REGISTER_R;
            } else if (!strcasecmp(value, "W")) {
                desc->mode = PCILIB_REGISTER_W;
            } else if (!strcasecmp(value, "RW")) {
                desc->mode = PCILIB_REGISTER_RW;
            } else if (!strcasecmp(value, "W")) {
                desc->mode = PCILIB_REGISTER_W;
            } else if (!strcasecmp(value, "RW1C")) {
                desc->mode = PCILIB_REGISTER_RW1C;
            } else if (!strcasecmp(value, "W1C")) {
                desc->mode = PCILIB_REGISTER_W1C;
            } else {
    		pcilib_error("Invalid access mode (%s) is specified in the XML register description", value);
    		return PCILIB_ERROR_INVALID_DATA;
            }
        } else if (!strcasecmp(name, "type")) {
            if (!strcasecmp(value, "fifo")) {
                desc->type = PCILIB_REGISTER_FIFO;
            } else {
        	pcilib_error("Invalid register type (%s) is specified in the XML register description", value);
        	return PCILIB_ERROR_INVALID_DATA;
            }
        } else if (!strcasecmp(name,"name")) {
            desc->name = value;
        } else if (!strcasecmp(name,"description")) {
            desc->description = value;
        }
	else if (!strcasecmp(name,"views")) {
            *views_ok=1;
        }
    }

    return 0;
}

static int pcilib_xml_create_register(pcilib_t *ctx, pcilib_register_bank_t bank, xmlXPathContextPtr xpath, xmlDocPtr doc, xmlNodePtr node) {
    int err;
    int views_ok=0;
    int h;

    xmlXPathObjectPtr nodes;
    xmlNodeSetPtr nodeset;

    pcilib_xml_register_description_t desc = {{0}};
    pcilib_xml_register_description_t fdesc;
    
    pcilib_register_t reg;

    desc.base.bank = ctx->banks[bank].addr;
    desc.base.rwmask = PCILIB_REGISTER_ALL_BITS;
    desc.base.mode = PCILIB_REGISTER_R;
    desc.base.type = PCILIB_REGISTER_STANDARD;
    
    err = pcilib_xml_parse_register(ctx, &desc, doc, node, &ctx->banks[bank],&views_ok);
    if (err) {
	pcilib_error("Error (%i) parsing an XML register", err);
	return err;
    }
    
    err = pcilib_add_registers(ctx, PCILIB_MODEL_MODIFICATION_FLAG_OVERRIDE, 1, &desc.base, &reg);
    if (err) {
	pcilib_error("Error (%i) adding a new XML register (%s) to the model", err, desc.base.name);
	return err;
    }

    ctx->register_ctx[reg].xml = node;    
    ctx->register_ctx[reg].min = desc.min;
    ctx->register_ctx[reg].max = desc.max;
    
    /* if the register had a node of type views, then we compute its associated registers. I do that here as i need the index for register context*/
    if(views_ok){
      err=pcilib_get_associated_views(ctx,desc.base.name,xpath,reg,1);
      if(err) pcilib_warning("can't get correctly the associated views of the register %s",desc.base.name);
    }

    xpath->node = node;
    nodes = xmlXPathEvalExpression(BIT_REGISTERS_PATH, xpath);
    xpath->node = NULL;
    
    if (!nodes) {
	xmlErrorPtr xmlerr = xmlGetLastError();

	if (xmlerr) pcilib_error("Failed to parse XPath expression %s, xmlXPathEvalExpression reported error %d - %s", BIT_REGISTERS_PATH, xmlerr->code, xmlerr->message);
	else pcilib_error("Failed to parse XPath expression %s", BIT_REGISTERS_PATH);
	return PCILIB_ERROR_FAILED;
    }

    nodeset = nodes->nodesetval;
    if (!xmlXPathNodeSetIsEmpty(nodeset)) {
	int i;
	
	for (i = 0; i < nodeset->nodeNr; i++) {
	    memset(&fdesc, 0, sizeof(pcilib_xml_register_description_t));

	    fdesc.base.bank = desc.base.bank;
	    fdesc.base.addr = desc.base.addr;
	    fdesc.base.mode = desc.base.mode;
	    fdesc.base.rwmask = desc.base.rwmask;
	    fdesc.base.type = PCILIB_REGISTER_BITS;
	    
    	    err = pcilib_xml_parse_register(ctx, &fdesc, doc, nodeset->nodeTab[i], &ctx->banks[bank],&views_ok);
    	    if (err) {
    		pcilib_error("Error parsing field in the XML register %s", desc.base.name);
    		continue;
    	    }
    	    
	    err = pcilib_add_registers(ctx, PCILIB_MODEL_MODIFICATION_FLAG_OVERRIDE, 1, &fdesc.base, &reg);
	    if (err) {
		pcilib_error("Error (%i) adding a new XML register (%s) to the model", err, fdesc.base.name);
		continue;
	    }
    	    
    	    ctx->register_ctx[reg].xml = nodeset->nodeTab[i];
	    ctx->register_ctx[reg].min = fdesc.min;
	    ctx->register_ctx[reg].max = fdesc.max;
	    if(views_ok){
	      err=pcilib_get_associated_views(ctx, desc.base.name,xpath,reg,0);
	      if(err) pcilib_warning("can't get correctly the associated views of the register %s",fdesc.base.name);
	    }
	}
    }
    xmlXPathFreeObject(nodes);

    return 0;
}

static int pcilib_xml_create_bank(pcilib_t *ctx, xmlXPathContextPtr xpath, xmlDocPtr doc, xmlNodePtr node) {
    int err;
    
    int override = 0;
    pcilib_register_bank_description_t desc = {0};
    pcilib_register_bank_t bank;
    xmlNodePtr cur;
    char *value, *name;
    char *endptr;

    xmlXPathObjectPtr nodes;
    xmlNodeSetPtr nodeset;


    desc.format = pcilib_xml_bank_default_format;
    desc.addr = PCILIB_REGISTER_BANK_DYNAMIC;
    desc.bar = PCILIB_BAR_NOBAR;
    desc.size = 0x1000;
    desc.protocol = PCILIB_REGISTER_PROTOCOL_DEFAULT;
    desc.access = 32;
    desc.endianess = PCILIB_HOST_ENDIAN;
    desc.raw_endianess = PCILIB_HOST_ENDIAN;

	// iterate through all children, representing bank properties, to fill the structure
    for (cur = node->children; cur != NULL; cur = cur->next) {
	if (!cur->children) continue;
	if (!xmlNodeIsText(cur->children)) continue;
	
	name = (char*)cur->name;
        value = (char*)cur->children->content;
        if (!value) continue;
        
        if (!strcasecmp(name, "bar")) {
    	    char bar = value[0]-'0';
    	    if ((strlen(value) != 1)||(bar < 0)||(bar > 5)) {
    		pcilib_error("Invalid BAR (%s) is specified in the XML bank description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
            desc.bar = (pcilib_bar_t)bar;
            override = 1;
        } else if (!strcasecmp(name,"size")) {
    	    long size = strtol(value, &endptr, 0);
    	    if ((strlen(endptr) > 0)||(size<=0)) {
    		pcilib_error("Invalid bank size (%s) is specified in the XML bank description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
            desc.size = (size_t)size;
            override = 1;
        } else if (!strcasecmp(name,"protocol")) {
	    pcilib_register_protocol_t protocol = pcilib_find_register_protocol_by_name(ctx, value);
	    if (protocol == PCILIB_REGISTER_PROTOCOL_INVALID) {
		pcilib_error("Unsupported protocol (%s) is specified in the XML bank description", value);
		return PCILIB_ERROR_NOTSUPPORTED;
	    }
	    desc.protocol = ctx->protocols[protocol].addr;
            override = 1;
        } else if (!strcasecmp(name,"address")) {
    	    uintptr_t addr = strtol(value, &endptr, 0);
    	    if ((strlen(endptr) > 0)) {
    		pcilib_error("Invalid address (%s) is specified in the XML bank description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
            desc.read_addr = addr;
            desc.write_addr = addr;
            override = 1;
        } else if (!strcasecmp(name,"read_address")) {
    	    uintptr_t addr = strtol(value, &endptr, 0);
    	    if ((strlen(endptr) > 0)) {
    		pcilib_error("Invalid address (%s) is specified in the XML bank description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
            desc.read_addr = addr;
            override = 1;
        } else if (!strcasecmp(name,"write_address")) {
    	    uintptr_t addr = strtol(value, &endptr, 0);
    	    if ((strlen(endptr) > 0)) {
    		pcilib_error("Invalid address (%s) is specified in the XML bank description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
            desc.write_addr = addr;
            override = 1;
        } else if(strcasecmp((char*)name,"word_size")==0) {
    	    int access = strtol(value, &endptr, 0);
    	    if ((strlen(endptr) > 0)||(access%8)||(access<=0)||(access>(8 * sizeof(pcilib_register_value_t)))) {
    		pcilib_error("Invalid word size (%s) is specified in the XML bank description", value);
    		return PCILIB_ERROR_INVALID_DATA;
    	    }
            desc.access = access;
            override = 1;
        } else if (!strcasecmp(name,"endianess")) {
            if (!strcasecmp(value,"little")) desc.endianess = PCILIB_LITTLE_ENDIAN;
            else if (!strcasecmp(value,"big")) desc.endianess = PCILIB_BIG_ENDIAN;
            else if (!strcasecmp(value,"host")) desc.endianess = PCILIB_HOST_ENDIAN;
            else {
    		pcilib_error("Invalid endianess (%s) is specified in the XML bank description", value);
    		return PCILIB_ERROR_INVALID_DATA;        
            }
            override = 1;
        } else if (!strcasecmp(name,"format")) {
            desc.format = value;
            override = 1;
        } else if (!strcasecmp((char*)name,"name")) {
            desc.name = value;
        } else if (!strcasecmp((char*)name,"description")) {
            desc.description = value;
            override = 1;
        } else if (!strcasecmp((char*)name,"override")) {
    	    override = 1;
        }
    }

    err = pcilib_add_register_banks(ctx, override?PCILIB_MODEL_MODIFICATION_FLAG_OVERRIDE:PCILIB_MODEL_MODIFICATION_FLAG_SKIP_EXISTING, 1, &desc, &bank);
    if (err) {
        pcilib_error("Error adding register bank (%s) specified in the XML bank description", desc.name);
        return err;
    }

    ctx->xml.bank_nodes[bank] = node;
    if (ctx->bank_ctx[bank]) {
	ctx->bank_ctx[bank]->xml = node;
    }

    xpath->node = node;
    nodes = xmlXPathEvalExpression(REGISTERS_PATH, xpath);
    xpath->node = NULL;
    
    if (!nodes) {
	xmlErrorPtr xmlerr = xmlGetLastError();
	if (xmlerr) pcilib_error("Failed to parse XPath expression %s, xmlXPathEvalExpression reported error %d - %s", REGISTERS_PATH, xmlerr->code, xmlerr->message);
	else pcilib_error("Failed to parse XPath expression %s", REGISTERS_PATH);
	return PCILIB_ERROR_FAILED;
    }

    nodeset = nodes->nodesetval;
    if (!xmlXPathNodeSetIsEmpty(nodeset)) {
	int i;
	for (i = 0; i < nodeset->nodeNr; i++) {
    	    err = pcilib_xml_create_register(ctx, bank, xpath, doc, nodeset->nodeTab[i]);
    	    if (err) pcilib_error("Error creating XML registers for bank %s", desc.name);
	}
    }
    xmlXPathFreeObject(nodes);

    return 0;
}

/*static int pcilib_xml_create_unit(pcilib_t *ctx, xmlXPathContextPtr xpath, xmlDocPtr doc, xmlNodePtr node) {
    int err;
    
    int override = 0;
    pcilib_unit_t desc = {0};
    xmlNodePtr cur;
    char *value, *name, *value2;
    char *endptr;

    xmlXPathObjectPtr nodes;
    xmlNodeSetPtr nodeset;
    xmlAttr *attr;
    int i=0;

      /* we get the attribute type of the view node*
    attr=node->properties;
    value=(char*)attr->children->content;
    desc.name=value;
    desc.other_units=malloc(sizeof(pcilib_transform_unit_t));
    
    for (cur = node->children; cur != NULL; cur = cur->next) {
	if (!cur->children) continue;
	if (!xmlNodeIsText(cur->children)) continue;
	
	name = (char*)cur->name;
        value = (char*)cur->children->content;
	attr= cur->properties;
	value2=(char*)attr->children->content;
        if (!value || !attr) continue;
        
        if (!strcasecmp(name, "convert_unit")) {
	  desc.other_units=realloc(des.other_units,sizeof((i+1)*sizeof(pcilib_transform_unit_t)));
	  desc.other_units[i].name=value2;
	  desc.other_units[i].transform_formula=value;
        }
    }

    err = pcilib_add_units(ctx, 1, &desc);
    if (err) {
        pcilib_error("Error adding unit (%s) specified in the XML", desc.name);
        return err;
    }

    return 0;
}
*/
/**
 * function that create a view from a view node, and populate ctx views list
 */
static int pcilib_xml_create_view(pcilib_t *ctx, xmlXPathContextPtr xpath, xmlDocPtr doc, xmlNodePtr node) {
    int err;
    
    pcilib_view_enum2_t complete_enum_desc={0};
    pcilib_view_enum_t enum_desc = {0};
    pcilib_view_formula_t formula_desc= {0};
    xmlNodePtr cur;
    char *value, *name;
    char *endptr;
    xmlAttr *attr;
    int i=0;
    int ok_min=0, ok_max=0;

    /*must i initialize? i think it's only needed if we want to include a description property*/ 
    enum_desc.name="default";
    enum_desc.value=0;
    enum_desc.min=0;
    enum_desc.max=0;

    complete_enum_desc.name="default enum";
    complete_enum_desc.description="default description";
    complete_enum_desc.enums_list=malloc(sizeof(pcilib_view_enum_t));
    if(!(complete_enum_desc.enums_list)){
      pcilib_error("can't allocate memory for the complete enum type");
      return PCILIB_ERROR_MEMORY;
    }
    complete_enum_desc.enums_list[0]=enum_desc;

    formula_desc.name="formula_default";
    formula_desc.read_formula="@reg";
    formula_desc.write_formula="@reg";
    formula_desc.description="default description";

    /* we get the attribute type of the view node*/
    attr=node->properties;
    value=(char*)attr->children->content;
    /* regarding the architecture, i decided to follow what has been done for registers and banks. but without the context*/
    /*if the view is of type enum, we get recursively its properties and then populate ctx enum views*/
    if(!(strcasecmp(value,"enum"))){
      for (cur = node->children; cur != NULL; cur = cur->next) {
	if (!cur->children) continue;
	if (!xmlNodeIsText(cur->children)) continue;
	
	name = (char*)cur->name;
	value = (char*)cur->children->content;
        if (!value) continue;
        
	if (!(strcasecmp((char*)name,"name"))) {
	  complete_enum_desc.name = value;

	}else if (!(strcasecmp((char*)name,"description"))) {
	  complete_enum_desc.description = value;

        }else if (!(strcasecmp((char*)name,"enum"))) {

	  complete_enum_desc.enums_list=realloc(complete_enum_desc.enums_list,(i+1)*sizeof(pcilib_view_enum_t));
	  complete_enum_desc.enums_list[i].name=value; 
	  
	  /* we need to iterate through the different attributes of an enum node to get all properties*/
	  for(attr=cur->properties; attr!=NULL;attr=attr->next){
	    if(!attr->children) continue;
	    if(!xmlNodeIsText(attr->children)) continue;
	    
	    name=(char*)attr->name;
	    value=(char*)attr->children->content;

	    if(!(strcasecmp(name,"value"))){
	       pcilib_register_value_t dat_value = strtol(value, &endptr, 0);
	       if ((strlen(endptr) > 0)) {
		 pcilib_error("Invalid value (%s) is specified in the XML enum node", value);
		 return PCILIB_ERROR_INVALID_DATA;
	       }
	       complete_enum_desc.enums_list[i].value=dat_value;
	    }else if(!(strcasecmp(name,"min"))){
	       pcilib_register_value_t dat_min = strtol(value, &endptr, 0);
	       if ((strlen(endptr) > 0)) {
		 pcilib_error("Invalid min (%s) is specified in the XML enum node", value);
		 return PCILIB_ERROR_INVALID_DATA;
	       }
	      complete_enum_desc.enums_list[i].min=dat_min;
	      ok_min=1;
	    }else if(!(strcasecmp(name,"max"))){
	       pcilib_register_value_t dat_max = strtol(value, &endptr, 0);
	       if ((strlen(endptr) > 0)) {
		 pcilib_error("Invalid max (%s) is specified in the XML enum node", value);
		 return PCILIB_ERROR_INVALID_DATA;
	       }
	      complete_enum_desc.enums_list[i].max=dat_max;
	      ok_max=1;
	    }
	    if(ok_min==0) complete_enum_desc.enums_list[i].min=complete_enum_desc.enums_list[i].value;
	    if(ok_max==0) complete_enum_desc.enums_list[i].max=complete_enum_desc.enums_list[i].value;
	    
	  }
	  i++;	
	}
      }
      err=pcilib_add_views_enum(ctx,1,&complete_enum_desc);
      if (err) {
	pcilib_error("Error (%i) adding a new enum view (%s) to the pcilib_t", err, complete_enum_desc.name);
	return err;
      }
   
      /* we do the same here but for a iew of type formula if the attribute gives formula*/
    }else if(!(strcasecmp(value,"formula"))){
      for (cur = node->children; cur != NULL; cur = cur->next) {
	if (!cur->children) continue;
	if (!xmlNodeIsText(cur->children)) continue;
	
	name = (char*)cur->name;
	value = (char*)cur->children->content;

        if (!value) continue;
        
	if (!(strcasecmp((char*)name,"name"))) {
	  formula_desc.name = value;
        }else if (!(strcasecmp((char*)name,"read_from_register"))) {
	  formula_desc.read_formula=value;
        }else if (!(strcasecmp((char*)name,"write_to_register"))) {
	  formula_desc.write_formula=value;
	}else if (!(strcasecmp((char*)name,"description"))) {
	  formula_desc.description=value;
	}
      }
      err=pcilib_add_views_formula(ctx,1,&formula_desc);
      if (err) {
	pcilib_error("Error (%i) adding a new formula view (%s) to the pcilib_t", err, formula_desc.name);
	return err;
      }

    }
 
    return 0;
}


/** pcilib_xml_initialize_banks
 *
 * function to create the structures to store the banks from the AST
 * @see pcilib_xml_create_bank
 * @param[in] doc the AST of the xml file.
 * @param[in] pci the pcilib_t running, which will be filled
 */
static int pcilib_xml_process_document(pcilib_t *ctx, xmlDocPtr doc, xmlXPathContextPtr xpath) {
  xmlXPathObjectPtr bank_nodes,views_nodes;
    xmlNodeSetPtr nodeset;
    int i;
    xmlErrorPtr xmlerr;

    views_nodes=xmlXPathEvalExpression(VIEWS_PATH,xpath);
    if(!views_nodes){
	xmlerr = xmlGetLastError();
	if (xmlerr) pcilib_error("Failed to parse XPath expression %s, xmlXPathEvalExpression reported error %d - %s", BANKS_PATH, xmlerr->code, xmlerr->message);
	else pcilib_error("Failed to parse XPath expression %s", BANKS_PATH);
	return PCILIB_ERROR_FAILED;
    }
    
        nodeset=views_nodes->nodesetval;
    if(!xmlXPathNodeSetIsEmpty(nodeset)){
      for(i=0;i < nodeset->nodeNr; i++){
	pcilib_xml_create_view(ctx,xpath,doc,nodeset->nodeTab[i]);
      }
    }
    xmlXPathFreeObject(views_nodes);    

    bank_nodes = xmlXPathEvalExpression(BANKS_PATH, xpath); 
    if (!bank_nodes) {
	xmlerr = xmlGetLastError();
	if (xmlerr) pcilib_error("Failed to parse XPath expression %s, xmlXPathEvalExpression reported error %d - %s", BANKS_PATH, xmlerr->code, xmlerr->message);
	else pcilib_error("Failed to parse XPath expression %s", BANKS_PATH);
	return PCILIB_ERROR_FAILED;
    }


    nodeset = bank_nodes->nodesetval;
    if (!xmlXPathNodeSetIsEmpty(nodeset)) {
	for (i = 0; i < nodeset->nodeNr; i++) {
    	    pcilib_xml_create_bank(ctx, xpath, doc, nodeset->nodeTab[i]);
	}
    }
    xmlXPathFreeObject(bank_nodes);
    
    return 0;
}

static int pcilib_xml_load_xsd(pcilib_t *ctx, char *xsd_filename) {
    int err;
    
    xmlSchemaParserCtxtPtr ctxt;

    /** we first parse the xsd file for AST with validation*/
    ctxt = xmlSchemaNewParserCtxt(xsd_filename);
    if (!ctxt) {
	xmlErrorPtr xmlerr = xmlGetLastError();
	if (xmlerr) pcilib_error("xmlSchemaNewParserCtxt reported error %d - %s", xmlerr->code, xmlerr->message);
	else pcilib_error("Failed to create a parser for XML schemas");
	return PCILIB_ERROR_FAILED;
    }
    
    ctx->xml.schema = xmlSchemaParse(ctxt);
    if (!ctx->xml.schema) {
	xmlErrorPtr xmlerr = xmlGetLastError();
	xmlSchemaFreeParserCtxt(ctxt);
	if (xmlerr) pcilib_error("Failed to parse XML schema, xmlSchemaParse reported error %d - %s", xmlerr->code, xmlerr->message);
	else pcilib_error("Failed to parse XML schema");
	return PCILIB_ERROR_INVALID_DATA;
    }
    
    xmlSchemaFreeParserCtxt(ctxt);

    ctx->xml.validator  = xmlSchemaNewValidCtxt(ctx->xml.schema);
    if (!ctx->xml.validator) {
	xmlErrorPtr xmlerr = xmlGetLastError();
	if (xmlerr) pcilib_error("xmlSchemaNewValidCtxt reported error %d - %s", xmlerr->code, xmlerr->message);
	else pcilib_error("Failed to create a validation context");
	return PCILIB_ERROR_FAILED;
    }
    
    err = xmlSchemaSetValidOptions(ctx->xml.validator, XML_SCHEMA_VAL_VC_I_CREATE);
    if (err) {
	xmlErrorPtr xmlerr = xmlGetLastError();
	if (xmlerr) pcilib_error("xmlSchemaSetValidOptions reported error %d - %s", xmlerr->code, xmlerr->message);
	else pcilib_error("Failed to configure the validation context to populate default attributes");
	return PCILIB_ERROR_FAILED;
    }

    return 0;
}

static int pcilib_xml_load_file(pcilib_t *ctx, const char *path, const char *name) {
    int err;
    char *full_name;
    xmlDocPtr doc;
    xmlXPathContextPtr xpath;

    full_name = (char*)alloca(strlen(path) + strlen(name) + 2);
    if (!name) {
	pcilib_error("Error allocating %zu bytes of memory in stack to create a file name", strlen(path) + strlen(name) + 2);
	return PCILIB_ERROR_MEMORY;
    }

    sprintf(full_name, "%s/%s", path, name);

    doc = xmlCtxtReadFile(ctx->xml.parser, full_name, NULL, 0);
    if (!doc) {
	xmlErrorPtr xmlerr = xmlCtxtGetLastError(ctx->xml.parser);
	if (xmlerr) pcilib_error("Error parsing %s, xmlCtxtReadFile reported error %d - %s", full_name, xmlerr->code, xmlerr->message);
	else pcilib_error("Error parsing %s", full_name);
	return PCILIB_ERROR_INVALID_DATA;
    }

    err = xmlSchemaValidateDoc(ctx->xml.validator, doc);
    if (err) {
	xmlErrorPtr xmlerr = xmlCtxtGetLastError(ctx->xml.parser);
	xmlFreeDoc(doc);
	if (xmlerr) pcilib_error("Error validating %s, xmlSchemaValidateDoc reported error %d - %s", full_name, xmlerr->code, xmlerr->message);
	else pcilib_error("Error validating %s", full_name);
	return PCILIB_ERROR_VERIFY;
    }

    xpath = xmlXPathNewContext(doc);
    if (!xpath) {
	xmlErrorPtr xmlerr = xmlGetLastError();
	xmlFreeDoc(doc);
	if (xmlerr) pcilib_error("Document %s: xmlXpathNewContext reported error %d - %s for document %s", full_name, xmlerr->code, xmlerr->message);
	else pcilib_error("Error creating XPath context for %s", full_name);
	return PCILIB_ERROR_FAILED;
    }

	// This can only partially fail... Therefore we need to keep XML and just return the error...
    err = pcilib_xml_process_document(ctx, doc, xpath);

    if (ctx->xml.num_files == PCILIB_MAX_MODEL_FILES) {
	xmlFreeDoc(doc);
        xmlXPathFreeContext(xpath);
	pcilib_error("Too many XML files for a model, only up to %zu are supported", PCILIB_MAX_MODEL_FILES);
	return PCILIB_ERROR_TOOBIG;
    }

    ctx->xml.docs[ctx->xml.num_files] = doc;
    ctx->xml.xpath[ctx->xml.num_files] = xpath;
    ctx->xml.num_files++;

    return err;
}


int pcilib_process_xml(pcilib_t *ctx, const char *location) {
    int err;

    DIR *rep;
    struct dirent *file = NULL;
    char *model_dir, *model_path;

    model_dir = getenv("PCILIB_MODEL_DIR");
    if (!model_dir) model_dir = PCILIB_MODEL_DIR;

    model_path = (char*)alloca(strlen(model_dir) + strlen(location) + 2);
    if (!model_path) return PCILIB_ERROR_MEMORY;

    sprintf(model_path, "%s/%s", model_dir, location);

    rep = opendir(model_path);
    if (!rep) return PCILIB_ERROR_NOTFOUND;

    while ((file = readdir(rep)) != NULL) {
	size_t len = strlen(file->d_name);
	if ((len < 4)||(strcasecmp(file->d_name + len - 4, ".xml"))) continue;
	if (file->d_type != DT_REG) continue;
	
	err = pcilib_xml_load_file(ctx, model_path, file->d_name);
	if (err) pcilib_error("Error processing XML file %s", file->d_name);
    }

    closedir(rep);

    return 0;
}


/** pcilib_init_xml
 * this function will initialize the registers and banks from the xml files
 * @param[in,out] ctx the pciilib_t running that gets filled with structures
 * @param[in] model the current model of ctx
 * @return an error code
 */
int pcilib_init_xml(pcilib_t *ctx, const char *model) {
    int err;

    char *model_dir;
    char *xsd_path;

    struct stat st;

    model_dir = getenv("PCILIB_MODEL_DIR");
    if (!model_dir) model_dir = PCILIB_MODEL_DIR;

    xsd_path = (char*)alloca(strlen(model_dir) + 16);
    if (!xsd_path) return PCILIB_ERROR_MEMORY;

    sprintf(xsd_path, "%s/model.xsd", model_dir);
    if (stat(xsd_path, &st)) {
	pcilib_info("XML models are not present");
	return PCILIB_ERROR_NOTFOUND;
    }

    ctx->xml.parser = xmlNewParserCtxt();
    if (!ctx->xml.parser) {
	xmlErrorPtr xmlerr = xmlGetLastError();
	if (xmlerr) pcilib_error("xmlNewParserCtxt reported error %d (%s)", xmlerr->code, xmlerr->message);
	else pcilib_error("Failed to create an XML parser context");
	return PCILIB_ERROR_FAILED;
    }

    err = pcilib_xml_load_xsd(ctx, xsd_path);
    if (err) return err;

    return pcilib_process_xml(ctx, model);
}

/** pcilib_free_xml
 * this function free the xml parts of the pcilib_t running, and some libxml ashes
 * @param[in] pci the pcilib_t running
*/
void pcilib_free_xml(pcilib_t *ctx) {
    int i;

    memset(ctx->xml.bank_nodes, 0, sizeof(ctx->xml.bank_nodes));
    for (i = 0; i < ctx->num_banks; i++) {
	if (ctx->bank_ctx[i]) 
	    ctx->bank_ctx[i]->xml = NULL;
    }
    
    for (i = 0; i < ctx->num_reg; i++) {
	ctx->register_ctx[i].xml = NULL;
    }
    
    for (i = 0; i < ctx->xml.num_files; i++) {
	if (ctx->xml.docs[i]) {
	    xmlFreeDoc(ctx->xml.docs[i]);
	    ctx->xml.docs[i] = NULL;
	}
	if (ctx->xml.xpath[i]) {
	    xmlXPathFreeContext(ctx->xml.xpath[i]);
	    ctx->xml.xpath[i] = NULL;
	}
    }
    ctx->xml.num_files = 0;

    if (ctx->xml.validator) {
	xmlSchemaFreeValidCtxt(ctx->xml.validator);
	ctx->xml.validator = NULL;
    }
    
    if (ctx->xml.schema) {
	xmlSchemaFree(ctx->xml.schema);
	ctx->xml.schema = NULL;
    
    }

    if (ctx->xml.parser) {
	xmlFreeParserCtxt(ctx->xml.parser);
	ctx->xml.parser = NULL;
    }
/*
    xmlSchemaCleanupTypes();
    xmlCleanupParser();
    xmlMemoryDump();
*/
}
