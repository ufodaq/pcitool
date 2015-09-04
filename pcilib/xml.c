/**
 * @file pcilib_xml.c
 * @version 1.0
 * 
 * @brief this file is the main source file for the implementation of dynamic registers using xml and several funtionalities for the "pci-tool" line command tool from XML files. the xml part has been implemented using libxml2
 * 
 * @details this code was meant to be evolutive as the XML files evolute. In this meaning, most of the xml parsing is realized with XPath expressions(when it was possible), so that changing the xml xould result mainly in changing the XPAth pathes present in the header file. In a case of a change in xml file, variables that need to be changed other than xpathes, will be indicated in the description of the function.
 
 Libxml2 considers blank spaces within the XML as node natively and this code as been produced considering blank spaces in the XML files. In case XML files would not contain blank spaces anymore, please change the code.

 In case the performance is not good enough, please consider the following : no more configuration file indicating where the files required are, hard code of formulas, and go to complete non evolutive code : get 1 access to xml file and context, and then make recursive descent to get all you need(investigation of libxml2 source code would be so needed to prove it's better to go recursive than just xpath).
 */
#define _XOPEN_SOURCE 700


#define REGISTERS_PATH ((xmlChar*)"/model/banks/bank/registers/register") /**<all standard registers nodes.*/
#define BITS_REGISTERS_PATH ((xmlChar*)"/model/banks/bank/registers/register/registers_bits/register_bits") /**<all bits registers nodes.*/
#define BANKS_PATH ((xmlChar*)"/model/banks/bank/bank_description") /**< path to complete nodes of banks.*/

#include "xml.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "pci.h"
#include "bank.h"
#include "register.h"
#include <libxml/xmlschemastypes.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <dirent.h>
#include <errno.h>

/** pcilib_xml_get_nodeset_from_xpath
 * this function takes a context from an AST and an XPath expression, to produce an object containing the nodes corresponding to the xpath expression
 * @param[in] doc the AST of the xml file
 * @param[in] xpath the xpath expression that will be evaluated
 *@return the nodeset from AST and xpath expression evaluation of the xml file
 */
static xmlXPathObjectPtr
pcilib_xml_get_nodeset_from_xpath(xmlXPathContextPtr doc, xmlChar *xpath){
	xmlXPathObjectPtr result;
	result=xmlXPathEvalExpression(xpath, doc); /**<the creation of the resulting object*/
	if(result==NULL){
	  pcilib_error("can't parse xpath expression %s",(char*)xpath);
	  return NULL;
	}
	
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
	  pcilib_warning("the parsing of %s xpath expression resulted in an empty nodeset",(char*)xpath);
	  xmlXPathFreeObject(result);
	  return NULL;
	}
	
	return result;
}


/**pcilib_xml_get_bank_node_from_register_node
 * this function get the bank xml node from a standards register xml node in the xml file
 *@param[in] mynode the register node we want to know the bank
 *@return the bank node
 */
static xmlNodePtr
pcilib_xml_get_bank_node_from_register_node(xmlNodePtr mynode){
  return xmlFirstElementChild(xmlFirstElementChild(mynode->parent->parent));
}



/** pcilib_xml_validate
 * function to validate the xml file against the xsd
 * @param[in] xml_filename path to the xml file
 * @param[in] xsd_filename path to the xsd file
 *@return an error code
 */
static int
pcilib_xml_validate(char* xml_filename, char* xsd_filename)
{
xmlDocPtr doc;
xmlSchemaPtr schema = NULL;
xmlSchemaParserCtxtPtr ctxt;
int ret=1;

/** we first parse the xsd file for AST with validation*/
ctxt = xmlSchemaNewParserCtxt(xsd_filename);
schema = xmlSchemaParse(ctxt);
xmlSchemaFreeParserCtxt(ctxt);

doc = xmlReadFile(xml_filename, NULL, 0);

if (doc == NULL)
{
	pcilib_error("Could not parse xml document ");
}
else
{
xmlSchemaValidCtxtPtr ctxt;
/** validation here*/
ctxt = xmlSchemaNewValidCtxt(schema);
ret = xmlSchemaValidateDoc(ctxt, doc);
xmlSchemaFreeValidCtxt(ctxt);
xmlFreeDoc(doc);
}

if(schema != NULL)
xmlSchemaFree(schema);
xmlSchemaCleanupTypes();

if(ret!=0) pcilib_warning("\nxml file \"%s\" does not validate against the schema, its register won't be used",xml_filename);

 return ret;
}



/** pcilib_xml_create_bank
 * 
 * this function create a bank structure from a xml bank node 
 * @param[out] mybank the created bank.
 * @param[in] my node the xml node used to create the bank
 * @param[in] doc the AST of the xml file, used for used lixbml functions
 */
static void
pcilib_xml_create_bank(pcilib_register_bank_description_t *mybank, xmlNodePtr mynode, xmlDocPtr doc, pcilib_t* pci){
		
    char* ptr;
    xmlNodePtr cur;
    xmlChar *value;
    static int i=0;
    int bar_ok=0;
    int k=0, name_found=0;

    cur=mynode->children; /** we place ourselves in the childrens of the bank node*/

    /** we iterate through all children, representing bank properties, to fill the structure*/
    while(cur!=NULL){
      /** we get each time the name of the node, corresponding to one property, and the value of the node*/
      value=xmlNodeListGetString(doc,cur->children,1);
	
	if(strcasecmp((char*)cur->name,"bar")==0){
	  mybank->bar=(pcilib_bar_t)(value[0]-'0');
	  bar_ok++;

	}else if(strcasecmp((char*)cur->name,"size")==0){
	    mybank->size=(size_t)strtol((char*)value,&ptr,0);
	
	}else if(strcasecmp((char*)cur->name,"protocol")==0){		
	  while(pci->protocols[k].name!=NULL){
	    if(strcasecmp(pci->protocols[k].name,(char*)value)==0){
	      mybank->protocol=pci->protocols[k].addr;
	      break;
	      k++;
	    }
	  }

	}else if(strcasecmp((char*)cur->name,"read_address")==0){		
	    mybank->read_addr=(uintptr_t)strtol((char*)value,&ptr,0);

	}else if(strcasecmp((char*)cur->name,"write_address")==0){
	      mybank->write_addr=(uintptr_t)strtol((char*)value,&ptr,0);
	
	}else if(strcasecmp((char*)cur->name,"word_size")==0){
	    mybank->access=(uint8_t)strtol((char*)value,&ptr,0);
	
	}else if(strcasecmp((char*)cur->name,"endianess")==0){		
	    if(strcasecmp((char*)value,"little")==0){
	      mybank->endianess=PCILIB_LITTLE_ENDIAN;
	    }else if(strcasecmp((char*)value,"big")==0){
	      mybank->endianess=PCILIB_BIG_ENDIAN;
	    }else if(strcasecmp((char*)value,"host")==0){
	      mybank->endianess=PCILIB_HOST_ENDIAN;
	    }		
	    mybank->raw_endianess=mybank->endianess;
	  
	}else if(strcasecmp((char*)cur->name,"format")==0){
	    mybank->format=(char*)value;

	}else if(strcasecmp((char*)cur->name,"name")==0){
	      mybank->name=(char*)value;
    
	}else if(strcasecmp((char*)cur->name,"description")==0){
		mybank->description=(char*)value;
	}

    cur=cur->next;
    }

    if(bar_ok==0) mybank->bar=PCILIB_BAR_NOBAR;
  
    while(pci->banks[k].name!=NULL){
	if(strcasecmp(pci->banks[k].name,mybank->name)==0){
	  mybank->addr=pci->banks[k].addr;
	  name_found++;
	  break;
	}
	k++;
    }
    if(name_found==0) {
      mybank->addr=PCILIB_REGISTER_BANK_DYNAMIC + i;
      i++;
    }
    pci->xml_banks[k]=mynode;
}
		  
	

/** pcilib_xml_initialize_banks
 * 
 * function to create the structures to store the banks from the AST 
 * @see pcilib_xml_create_bank(
 * @param[in] doc the AST of the xml file.
 * @param[in] pci the pcilib_t running, which will be filled
 */
static void
pcilib_xml_initialize_banks(pcilib_t* pci,xmlDocPtr doc){
	pcilib_register_bank_description_t mybank;

	xmlNodeSetPtr nodesetaddress=NULL;
	xmlNodePtr mynode;	
	xmlXPathContextPtr context;
        int i;

	mynode=malloc(sizeof(xmlNode));
	if(!(context= xmlXPathNewContext(doc))){
	    pcilib_error("can't get an AST from an xml file");
	    return;
	}
	/** we get the bank nodes using xpath expression*/
	nodesetaddress=pcilib_xml_get_nodeset_from_xpath(context,BANKS_PATH)->nodesetval;
	if(!(nodesetaddress)) return;
	if(nodesetaddress->nodeNr==0) return;

	pci->xml_banks=calloc(PCILIB_MAX_REGISTER_BANKS+1,sizeof(xmlNodePtr));
        if(!(pci->xml_banks)) pcilib_error("can't create bank xml nodes for pcilib_t struct");

	/** for each of the bank nodes, we create the associated structure, and  push it in the pcilib environnement*/
	for(i=0;i<nodesetaddress->nodeNr;i++){
	  mynode=nodesetaddress->nodeTab[i];
	  pcilib_xml_create_bank(&mybank,mynode,doc, pci);
	  pcilib_add_register_banks(pci,1,&mybank);
	}


}

/** pcilib_xml_registers_compare
 * function to compare registers by address for sorting registers
 * @param a one hypothetic register
 * @param b one other hypothetic register
 *@return the result of the comparison
 */
static int
pcilib_xml_compare_registers(void const *a, void const *b){
  const pcilib_register_description_t  *pa=a;
  const pcilib_register_description_t  *pb=b;

    return pa->addr - pb->addr;
}


/** pcilib_xml_arrange_registers
 * after the complete structure containing the registers has been created from the xml, this function rearrange them by address in order to add them in pcilib_open, which don't consider the adding of registers in order of address	
 * @param[in,out] registers the list of registers in : not ranged out: ranged.
 * @param[in] size the number of registers. 
 */
void pcilib_xml_arrange_registers(pcilib_register_description_t *registers,int size){
	pcilib_register_description_t* temp;
	temp=malloc(size*sizeof(pcilib_register_description_t));
	qsort(registers,size,sizeof(pcilib_register_description_t),pcilib_xml_compare_registers);
	free(temp);
}

/** pcilib_xml_create_register.
 * this function create a register structure from a xml register node.
 * @param[out] myregister the register we want to create
 * @param[in] type the type of the future register, because this property can't be parsed
 * @param[in] doc the AST of the xml file, required for some ibxml2 sub-fonctions
 * @param[in] mynode the xml node to create register from
 */
void pcilib_xml_create_register(pcilib_register_description_t *myregister,xmlNodePtr mynode, xmlDocPtr doc, xmlChar* type, pcilib_t* pci){
		
    char* ptr;
    xmlNodePtr cur;
    xmlChar *value=NULL;
    xmlNodePtr bank=NULL;
    int i=0;

    /**we get the children of the register xml nodes, that contains the properties for it*/
    cur=mynode->children;

    while(cur!=NULL){
	value=xmlNodeListGetString(doc,cur->children,1);
	/* we iterate throug each children to get the associated property
            note :the use of strtol permits to get as well hexadecimal and decimal values
		*/
	if(strcasecmp((char*)cur->name,"address")==0){	
		myregister->addr=(pcilib_register_addr_t)strtol((char*)value,&ptr,0);

	}else if(strcasecmp((char*)cur->name,"offset")==0){
		myregister->offset=(pcilib_register_size_t)strtol((char*)value,&ptr,0);

	}else if(strcasecmp((char*)cur->name,"size")==0){
		myregister->bits=(pcilib_register_size_t)strtol((char*)value,&ptr,0);

	}else if(strcasecmp((char*)cur->name,"default")==0){
		myregister->defvalue=(pcilib_register_value_t)strtol((char*)value,&ptr,0);

	}else if(strcasecmp((char*)cur->name,"rwmask")==0){
	    if(strcasecmp((char*)value,"all bits")==0){
	      myregister->rwmask=PCILIB_REGISTER_ALL_BITS;
	    }else if(strcasecmp((char*)value,"0")==0){
	      myregister->rwmask=0;
	    }else{
	      myregister->rwmask=(pcilib_register_value_t)strtol((char*)value,&ptr,0);
	    }

	}else if(strcasecmp((char*)cur->name,"mode")==0){
	  if(strcasecmp((char*)value,"R")==0){
	    myregister->mode=PCILIB_REGISTER_R;
	  }else if(strcasecmp((char*)value,"W")==0){
	    myregister->mode=PCILIB_REGISTER_W;
	  }else if(strcasecmp((char*)value,"RW")==0){
	    myregister->mode=PCILIB_REGISTER_RW;
	  }else if(strcasecmp((char*)value,"W")==0){
	    myregister->mode=PCILIB_REGISTER_W;
	  }else if(strcasecmp((char*)value,"RW1C")==0){
	    myregister->mode=PCILIB_REGISTER_RW1C;
	  }else if(strcasecmp((char*)value,"W1C")==0){
	    myregister->mode=PCILIB_REGISTER_W1C;
	  }

	}else if(strcasecmp((char*)cur->name,"name")==0){
	  myregister->name=(char*)value;

	}else if(strcasecmp((char*)cur->name,"value_min")==0){
	  /* not implemented yet*/	  //	  myregister->value_min=(pcilib_register_value_t)strtol((char*)value,&ptr,0);

	}else if(strcasecmp((char*)cur->name,"value_max")==0){
	  /* not implemented yet*/	  //	  myregister->value_max=(pcilib_register_value_t)strtol((char*)value,&ptr,0);	

	}else if(strcasecmp((char*)cur->name,"description")==0){
	  i++;
	  myregister->description=(char*)value;
	}
      
      cur=cur->next;
    }

    /** make sure we don't get the description from previous registers*/
    if(i==0) myregister->description=NULL;
    

    /** we then get properties that can not be parsed as the previous ones*/
	if(strcasecmp((char*)type,"standard")==0){
	  myregister->type=PCILIB_REGISTER_STANDARD;
	  bank=pcilib_xml_get_bank_node_from_register_node(mynode);
	  while(bank!=NULL){
	    value=xmlNodeListGetString(doc,bank->children,1);
	    if(strcasecmp((char*)bank->name,"name")==0){
	      break;
	    }
	    bank=bank->next;
	  }

	  int k=0;

	  while(pci->banks[k].name!=NULL){
	    printf("name %s\n",pci->banks[k].name);
	    if(strcasecmp(pci->banks[k].name,(char*)value)==0){
	      myregister->bank=pci->banks[k].addr;
	      printf("ok\n");
	      break;
	    }
	    k++;
	  }

	}else if(strcasecmp((char*)type,"bits")==0){
	  myregister->type=PCILIB_REGISTER_BITS;

	}else if(strcasecmp((char*)type,"fifo")==0){
  /* not implemented yet*/	  myregister->type=PCILIB_REGISTER_FIFO;
	}
      
	
}	

/** pcilib_xml_initialize_registers
 *
 * this function create a list of registers from an abstract syntax tree
 *
 * variables who need change if xml structure change : bank,description, sub_description, sub_address, sub_bank, sub_rwmask (we consider it to be equal to the standard register), sub_defvalue(same to standard register normally)
 * @param[in] doc the xpath context of the xml file.
 * @param[in] pci the pcilib_t struct running, that will get filled
 */
void pcilib_xml_initialize_registers(pcilib_t* pci,xmlDocPtr doc){
	
  xmlNodeSetPtr nodesetaddress=NULL, nodesetsubaddress=NULL;
	xmlNodePtr mynode;	
	xmlXPathContextPtr context;
	pcilib_register_description_t *registers=NULL;
	pcilib_register_description_t myregister;
	int i,j;

	context= xmlXPathNewContext(doc);
	if(!(context= xmlXPathNewContext(doc))){
	    pcilib_error("can't get an AST from an xml file");
	    return;
	}
	
	/** we first get the nodes of standard and bits registers*/
	nodesetaddress=pcilib_xml_get_nodeset_from_xpath(context,REGISTERS_PATH)->nodesetval;
	nodesetsubaddress=pcilib_xml_get_nodeset_from_xpath(context,BITS_REGISTERS_PATH)->nodesetval;
	if(!(nodesetaddress)) return;
	if(nodesetaddress->nodeNr>0)registers=calloc(nodesetaddress->nodeNr+nodesetsubaddress->nodeNr,sizeof(pcilib_register_description_t));
        else return;
	
	pci->xml_registers=calloc(nodesetaddress->nodeNr+nodesetsubaddress->nodeNr,sizeof(xmlNodePtr));
	if(!(pci->xml_registers)) pcilib_warning("can't create registers xml nodes in pcilib_t struct");
		
	/** we then iterate through standard registers nodes to create registers structures*/
	for(i=0;i<nodesetaddress->nodeNr;i++){
		mynode=nodesetaddress->nodeTab[i];
		pcilib_xml_create_register(&myregister,mynode,doc,(xmlChar*)"standard",pci);
		registers[i]=myregister;
		pci->xml_registers[i]=mynode;
	}
	
	j=i;
	if(!(nodesetsubaddress)) return;
	/** we then iterate through bits  registers nodes to create registers structures*/
	for(i=0;i<nodesetsubaddress->nodeNr;i++){
		mynode=nodesetsubaddress->nodeTab[i];
		pcilib_xml_create_register(&myregister,mynode->parent->parent,doc,(xmlChar*)"standard",pci);
		pcilib_xml_create_register(&myregister,mynode,doc,(xmlChar*)"bits",pci);
		registers[i+j]=myregister;
		pci->xml_registers[i+j]=mynode;
	}
	
	/**we arrange the register for them to be well placed for pci-l*/
	pcilib_xml_arrange_registers(registers,nodesetaddress->nodeNr+nodesetsubaddress->nodeNr);
	/**we fille the pcilib_t struct*/
        pcilib_add_registers(pci,nodesetaddress->nodeNr+nodesetsubaddress->nodeNr,registers);
}


/** pcilib_init_xml
 * this function will initialize the registers and banks from the xml files
 * @param[in,out] ctx the pciilib_t running that gets filled with structures
 * @param[in] model the current model of ctx
 * @return an error code
 */

int pcilib_init_xml(pcilib_t* ctx, char* model){
    char *path,*pwd, **line, *line_xsd=NULL;
    int i=1,k;
    xmlDocPtr* docs;
    DIR* rep=NULL;
    struct dirent* file=NULL;
    int err;

    path=malloc(sizeof(char*));
    line=malloc(sizeof(char*));
    line[0]=NULL;
   
    /** we first get the env variable corresponding to the place of the xml files*/
    path=getenv("PCILIB_MODEL_DIR");
    if(path==NULL){
      pcilib_warning("can't find environment variable for xml files");
      return 1;
    }
    
    /** we then open the directory corresponding to the ctx model*/
    pwd=malloc((strlen(path)+strlen(model))*sizeof(char));
    sprintf(pwd,"%s%s/",path,model);
    if((rep=opendir(pwd))==NULL){
      pcilib_warning("could not open the directory for xml files: error %i\n",errno);
      return 1;
    }

    /** we iterate through the files of the directory, to get the xml files and the xsd file*/
    while((file=readdir(rep))!=NULL){
      if(strstr(file->d_name,".xml")!=NULL){
	line=realloc(line,i*sizeof(char*));
	line[i-1]=malloc((strlen(file->d_name)+strlen(pwd)+1)*sizeof(char));
	sprintf(line[i-1],"%s%s",pwd,file->d_name); /**< here i wanted to use realpath() function, but it was not working correctly*/
	i++;
      }
      if(strstr(file->d_name,".xsd")!=NULL){
	line_xsd=malloc((strlen(file->d_name)+strlen(pwd))*sizeof(char));	
	sprintf(line_xsd,"%s%s",pwd,file->d_name);
      }
    }

    if(line_xsd==NULL){
      pcilib_warning("no xsd file found");
      return 1;
    }

    if(line[0]==NULL){
      pcilib_warning("no xml file found");
      return 1;
    }
    
    /** for each xml file, we validate it, and get the registers and the banks*/
    docs=malloc((i-1)*sizeof(xmlDocPtr));
    for(k=0;k<i-1;k++){
      if((err=pcilib_xml_validate(line[k],line_xsd))==0){
	if((docs[k]=xmlParseFile(line[k]))){
	  pcilib_xml_initialize_banks(ctx,docs[k]);
	    pcilib_xml_initialize_registers(ctx,docs[k]);
	}
	else pcilib_error("xml document %s not parsed successdfullly\n",line[k]);
      }
    }  
    ctx->xml_context=malloc(sizeof(pcilib_xml_context_t));
    ctx->xml_context->docs=malloc(sizeof(xmlDocPtr)*(i-1));
    ctx->xml_context->docs=docs;

    free(pwd);
    free(line);
    free(line_xsd);

    return 0;
}  

/** pcilib_clean_xml
 * this function free the xml parts of the pcilib_t running, and some libxml ashes
 * @param[in] pci the pcilib_t running
*/
void pcilib_clean_xml(pcilib_t* pci){

  free(pci->xml_banks);
  free(pci->xml_registers);
  free(pci->xml_context);
  xmlCleanupParser();
  xmlMemoryDump();

}
