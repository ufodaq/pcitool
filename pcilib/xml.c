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
#include "xml.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "pci.h"
#include "bank.h"
#include "register.h"
#include <libxml/xmlschemastypes.h>

#include <dirent.h>
#include <errno.h>

/** pcilib_xml_getdoc
 * this function takes a string and will create an abtract syntax tree from the xml file represented by the string
 * @param[in] filename the the name of the xml file containing registers and banks 
 */
static xmlDocPtr
pcilib_xml_getdoc(char* filename){
		xmlDocPtr doc;
		doc=xmlParseFile(filename); /**<the creation of the AST from a libxml2 funtion*/
		
		if (doc==NULL){
			pcilib_error("xml document not parsed successfully");
			return NULL;
		}
		
		return doc;
}

/** pcilib_xml_getsetproperty
 * this function takes a context from an AST and an XPath expression, to produce an object containing the nodes corresponding to the xpath expression
 * @param[in] doc the AST of the xml file
 * @param[in] xpath the xpath expression that will be evaluated
 */
static xmlXPathObjectPtr
pcilib_xml_getsetproperty(xmlXPathContextPtr doc, xmlChar *xpath){
	xmlXPathObjectPtr result;
	result=xmlXPathEvalExpression(xpath, doc); /**<the creation of the resulting object*/
	if(result==NULL){
		return NULL;
	}
	
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
		return NULL;
	}
	
	return result;
}

/** pcilib_xml_getcontext.
 * this function create a context in an AST (ie initialize XPAth for the AST).
 * @param[in] doc the AST of the xml file.
 */
static xmlXPathContextPtr
pcilib_xml_getcontext(xmlDocPtr doc){
  xmlXPathContextPtr context;
	  context= xmlXPathNewContext(doc); /**<the creation of the context using a libxml2's function */
	 if(context==NULL){
	    pcilib_error("warning : no context obtained for the xml document");
	     return NULL;
		}
	return context;
}


/** validation
 * function to validate the xml file against the xsd
 * @param[in] xml_filename path to the xml file
 * @param[in] xsd_filename path to the xsd file
 */
static int
validation(char* xml_filename, char* xsd_filename)
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
pcilib_xml_create_bank(pcilib_register_bank_description_t *mybank, xmlNodePtr mynode, xmlDocPtr doc){
		
    char* ptr;
    xmlNodePtr cur;
    xmlChar *value;

    cur=mynode->children; /** we place ourselves in the childrens of the bank node*/

    /** we iterate through all children, representing bank properties, to fill the structure*/
    while(cur!=NULL){
      /** we get each time the name of the node, corresponding to one property, and the value of the node*/
      value=xmlNodeListGetString(doc,cur->children,1);
	
	if(strcmp((char*)cur->name,"adress")==0){
	    if (strcmp((char*)value,"bank 0")==0){
	      mybank->addr=PCILIB_REGISTER_BANK0;
	    }else if (strcmp((char*)value,"bank 1")==0){
	      mybank->addr=PCILIB_REGISTER_BANK1;
	    }else if (strcmp((char*)value,"bank 2")==0){
	      mybank->addr=PCILIB_REGISTER_BANK2;
	    }else if (strcmp((char*)value,"bank 3")==0){
	      mybank->addr=PCILIB_REGISTER_BANK3;
	    }else if (strcmp((char*)value,"DMA bank")==0){
	      mybank->addr=PCILIB_REGISTER_BANK_DMA;
	    }else if (strcmp((char*)value,"DMAconf bank")==0){
	      mybank->addr=PCILIB_REGISTER_BANK_DMACONF;
	    }else if (strcmp((char*)value,"dynamic bank")==0){
	      mybank->addr=PCILIB_REGISTER_BANK_DYNAMIC;
	    }else{
	      mybank->addr=PCILIB_REGISTER_BANK_INVALID;
	    }

	}else if(strcmp((char*)cur->name,"bar")==0){
	    if(strcmp((char*)value,"0")==0){
	      mybank->bar=PCILIB_BAR0;
	    }else if(strcmp((char*)value,"1")==0){
	      mybank->bar=PCILIB_BAR1;
	    }else if(strcmp((char*)value,"no_bar")==0){
	      mybank->bar=PCILIB_BAR_NOBAR;
	    }else{
	      mybank->bar=PCILIB_BAR_INVALID;
	    }
	    mybank->bar=(pcilib_bar_t)strtol((char*)value,&ptr,0);
	
	}else if(strcmp((char*)cur->name,"size")==0){
	    mybank->size=(size_t)strtol((char*)value,&ptr,0);
	
	}else if(strcmp((char*)cur->name,"protocol")==0){		
	    if(strcmp((char*)value,"default")==0){
	      mybank->protocol=PCILIB_REGISTER_PROTOCOL_DEFAULT;
	    }else if(strcmp((char*)value,"0")==0){
	      mybank->protocol=PCILIB_REGISTER_PROTOCOL0;
	    }else if(strcmp((char*)value,"dma")==0){
	      mybank->protocol=PCILIB_REGISTER_PROTOCOL_DMA;
	    }else if(strcmp((char*)value,"dynamic")==0){
	      mybank->protocol=PCILIB_REGISTER_PROTOCOL_DYNAMIC;
	    }else if (strcmp((char*)value,"software")==0){
	      mybank->protocol=PCILIB_REGISTER_PROTOCOL_SOFTWARE;
	    }else mybank->protocol=PCILIB_REGISTER_PROTOCOL_INVALID;
	
	}else if(strcmp((char*)cur->name,"read_adress")==0){		
	    mybank->read_addr=(uintptr_t)strtol((char*)value,&ptr,0);

	}else if(strcmp((char*)cur->name,"write_adress")==0){
	      mybank->write_addr=(uintptr_t)strtol((char*)value,&ptr,0);
	
	}else if(strcmp((char*)cur->name,"word_size")==0){
	    mybank->access=(uint8_t)strtol((char*)value,&ptr,0);
	
	}else if(strcmp((char*)cur->name,"endianess")==0){		
	    if(strcmp((char*)value,"little")==0){
	      mybank->endianess=PCILIB_LITTLE_ENDIAN;
	    }else if(strcmp((char*)value,"big")==0){
	      mybank->endianess=PCILIB_BIG_ENDIAN;
	    }else if(strcmp((char*)value,"host")==0){
	      mybank->endianess=PCILIB_HOST_ENDIAN;
	    }		
	    mybank->raw_endianess=mybank->endianess;
	  
	}else if(strcmp((char*)cur->name,"format")==0){
	    mybank->format=(char*)value;

	}else if(strcmp((char*)cur->name,"name")==0){
	      mybank->name=(char*)value;
    
	}else if(strcmp((char*)cur->name,"description")==0){
		mybank->description=(char*)value;
	}

    cur=cur->next;
    }
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

	xmlNodeSetPtr nodesetadress=NULL;
	xmlNodePtr mynode;	
	pcilib_register_bank_description_t* banks;
	xmlXPathContextPtr context;
        int i;

	mynode=malloc(sizeof(xmlNode));
	context=pcilib_xml_getcontext(doc);
	
	/** we get the bank nodes using xpath expression*/
	nodesetadress=pcilib_xml_getsetproperty(context,BANKS_PATH)->nodesetval;
	if(nodesetadress->nodeNr>0) banks=calloc((nodesetadress->nodeNr),sizeof(pcilib_register_bank_description_t));
	else return;

	pci->banks_xml_nodes=calloc(nodesetadress->nodeNr,sizeof(xmlNodePtr));
        if(!(pci->banks_xml_nodes)) pcilib_warning("can't create bank xml nodes for pcilib_t struct");

	/** for each of the bank nodes, we create the associated structure*/
	for(i=0;i<nodesetadress->nodeNr;i++){
	  mynode=nodesetadress->nodeTab[i];
	  pcilib_xml_create_bank(&mybank,mynode,doc);
	  banks[i]=mybank;
	  pci->banks_xml_nodes[i]=mynode;
	}
	/** we push our banks structures in the pcilib environnement*/
    pcilib_add_register_banks(pci,nodesetadress->nodeNr,banks);
}

/*
 * next 2 functions are for the implementation of a merge sort algorithm, because we need a stable algorithm
 */
static void
pcilib_xml_topdownmerge(pcilib_register_description_t *A, int start, int middle, int end, pcilib_register_description_t *B){
 int i0,i1,j;
 i0= start;
 i1=middle;

 for(j=start;j<end;j++){
 	if((i0 < middle) && (i1>=end || A[i0].addr<=A[i1].addr)){
		B[j]=A[i0];
		i0++;
	}
	else{
		B[j]=A[i1];
		i1++;
	}
	}
}

static void
pcilib_xml_topdownsplitmerge(pcilib_register_description_t *A, int start, int end, pcilib_register_description_t *B){
	int middle;
	if(end-start <2)
		return;
	
	middle =(end+start)/2;
	pcilib_xml_topdownsplitmerge(A,start, middle,B);
	pcilib_xml_topdownsplitmerge(A,middle,end,B);
	pcilib_xml_topdownmerge(A,start,middle,end,B);
	memcpy(&A[start],&B[start],(end-start)*sizeof(pcilib_register_description_t));
}

/** pcilib_xml_arrange_registers
 * after the complete structure containing the registers has been created from the xml, this function rearrange them by address in order to add them in pcilib_open, which don't consider the adding of registers in order of adress	
 * @param[in,out] registers the list of registers in : not ranged out: ranged.
 * @param[in] size the number of registers. 
 */
void pcilib_xml_arrange_registers(pcilib_register_description_t *registers,int size){
	pcilib_register_description_t* temp;
	temp=malloc(size*sizeof(pcilib_register_description_t));
	pcilib_xml_topdownsplitmerge(registers,0,size,temp);
	free(temp);
}

/** pcilib_xml_create_register.
 * this function create a register structure from a xml register node.
 * @param[out] myregister the register we want to create
 * @param[in] type the type of the future register, because this property can't be parsed
 * @param[in] doc the AST of the xml file, required for some ibxml2 sub-fonctions
 * @param[in] mynode the xml node to create register from
 */
void pcilib_xml_create_register(pcilib_register_description_t *myregister,xmlNodePtr mynode, xmlDocPtr doc, xmlChar* type){
		
    char* ptr;
    xmlNodePtr cur;
    xmlChar *value;
    xmlChar *bank;

    /**we get the children of the register xml nodes, that contains the properties for it*/
    cur=mynode->children;

    while(cur!=NULL){
	value=xmlNodeListGetString(doc,cur->children,1);
	/* we iterate throug each children to get the associated property
            note :the use of strtol permits to get as well hexadecimal and decimal values
		*/
	if(strcmp((char*)cur->name,"adress")==0){	
		myregister->addr=(pcilib_register_addr_t)strtol((char*)value,&ptr,0);

	}else if(strcmp((char*)cur->name,"offset")==0){
		myregister->offset=(pcilib_register_size_t)strtol((char*)value,&ptr,0);

	}else if(strcmp((char*)cur->name,"size")==0){
		myregister->bits=(pcilib_register_size_t)strtol((char*)value,&ptr,0);

	}else if(strcmp((char*)cur->name,"default")==0){
		myregister->defvalue=(pcilib_register_value_t)strtol((char*)value,&ptr,0);

	}else if(strcmp((char*)cur->name,"rwmask")==0){
	    if(strcmp((char*)value,"all bits")==0){
	      myregister->rwmask=PCILIB_REGISTER_ALL_BITS;
	    }else if(strcmp((char*)value,"0")==0){
	      myregister->rwmask=0;
	    }else{
	      myregister->rwmask=(pcilib_register_value_t)strtol((char*)value,&ptr,0);
	    }

	}else if(strcmp((char*)cur->name,"mode")==0){
	  if(strcmp((char*)value,"R")==0){
	    myregister->mode=PCILIB_REGISTER_R;
	  }else if(strcmp((char*)value,"W")==0){
	    myregister->mode=PCILIB_REGISTER_W;
	  }else if(strcmp((char*)value,"RW")==0){
	    myregister->mode=PCILIB_REGISTER_RW;
	  }else if(strcmp((char*)value,"W")==0){
	    myregister->mode=PCILIB_REGISTER_W;
	  }else if(strcmp((char*)value,"RW1C")==0){
	    myregister->mode=PCILIB_REGISTER_RW1C;
	  }else if(strcmp((char*)value,"W1C")==0){
	    myregister->mode=PCILIB_REGISTER_W1C;
	  }

	}else if(strcmp((char*)cur->name,"name")==0){
	  myregister->name=(char*)value;

	}else if(strcmp((char*)cur->name,"value_min")==0){
	  /* not implemented yet*/	  //	  myregister->value_min=(pcilib_register_value_t)strtol((char*)value,&ptr,0);

	}else if(strcmp((char*)cur->name,"value_max")==0){
	  /* not implemented yet*/	  //	  myregister->value_max=(pcilib_register_value_t)strtol((char*)value,&ptr,0);	

	}else if(strcmp((char*)cur->name,"description")==0){
	  myregister->description=(char*)value;
	}
      
      cur=cur->next;
    }

    /** we then get properties that can not be parsed as the previous ones*/
	if(strcmp((char*)type,"standard")==0){
	  myregister->type=PCILIB_REGISTER_STANDARD;
	  bank=xmlNodeListGetString(doc,xmlFirstElementChild(xmlFirstElementChild(mynode->parent->parent))->xmlChildrenNode,1);	  /**<we get the bank adress node*/

	}else if(strcmp((char*)type,"bits")==0){
	  myregister->type=PCILIB_REGISTER_BITS;
	  bank=xmlNodeListGetString(doc,xmlFirstElementChild(xmlFirstElementChild(mynode->parent->parent->parent->parent))->xmlChildrenNode,1);/**<we get the bank adress node*/

	  /* we then get the properties from the parent standard regsiter, and that are not present for the bit register*/
	  cur=mynode->parent->parent->children; /**<get the parent standard register*/
	  while(cur!=NULL){
	    value=xmlNodeListGetString(doc,cur->children,1);
	    if(strcmp((char*)cur->name,"adress")==0){	
	      myregister->addr=(pcilib_register_addr_t)strtol((char*)value,&ptr,0);
	    }else if(strcmp((char*)cur->name,"default")==0){
	      myregister->defvalue=(pcilib_register_value_t)strtol((char*)value,&ptr,0);
	    }else if(strcmp((char*)cur->name,"rwmask")==0){
	      if(strcmp((char*)value,"all bits")==0){
		myregister->rwmask=PCILIB_REGISTER_ALL_BITS;
	      }else if(strcmp((char*)value,"0")==0){
		myregister->rwmask=0;
	      }else{
		myregister->rwmask=(pcilib_register_value_t)strtol((char*)value,&ptr,0);
	      }
	    }
	    cur=cur->next;
	  }

	}else if(strcmp((char*)type,"fifo")==0){
  /* not implemented yet*/	  myregister->type=PCILIB_REGISTER_FIFO;
	}

	if(strcmp((char*)bank,"bank 0")==0){
	  myregister->bank=PCILIB_REGISTER_BANK0;
	}else if(strcmp((char*)bank,"bank 1")==0){
	  myregister->bank=PCILIB_REGISTER_BANK1;
	}else if(strcmp((char*)bank,"bank 2")==0){
	  myregister->bank=PCILIB_REGISTER_BANK2;
	}else if(strcmp((char*)bank,"bank 3")==0){
	  myregister->bank=PCILIB_REGISTER_BANK3;
	}else if(strcmp((char*)bank,"DMA bank")==0){
	  myregister->bank=PCILIB_REGISTER_BANK_DMA;
	}else if (strcmp((char*)bank,"dynamic bank")==0){
	  myregister->addr=PCILIB_REGISTER_BANK_DYNAMIC;
	}else{
	  myregister->bank=PCILIB_REGISTER_BANK_INVALID;
	}
}	

/** pcilib_xml_initialize_registers
 *
 * this function create a list of registers from an abstract syntax tree
 *
 * variables who need change if xml structure change : bank,description, sub_description, sub_adress, sub_bank, sub_rwmask (we consider it to be equal to the standard register), sub_defvalue(same to standard register normally)
 * @param[in] doc the xpath context of the xml file.
 * @param[in] pci the pcilib_t struct running, that will get filled
 */
void pcilib_xml_initialize_registers(pcilib_t* pci,xmlDocPtr doc){
	
  xmlNodeSetPtr nodesetadress=NULL, nodesetsubadress=NULL;
	xmlChar *type=NULL;
	xmlNodePtr mynode;	
	xmlXPathContextPtr context;
	int number_registers;
	pcilib_register_description_t *registers=NULL;
	pcilib_register_description_t myregister;
	int i,j;

        context=pcilib_xml_getcontext(doc);
	
	/** we first get the nodes of standard and bits registers*/
	nodesetadress=pcilib_xml_getsetproperty(context,REGISTERS_PATH)->nodesetval;
	nodesetsubadress=pcilib_xml_getsetproperty(context,BITS_REGISTERS_PATH)->nodesetval;
	if(nodesetadress->nodeNr>0)registers=calloc(nodesetadress->nodeNr+nodesetsubadress->nodeNr,sizeof(pcilib_register_description_t));
        else return;
	
	pci->registers_xml_nodes=calloc(nodesetadress->nodeNr+nodesetsubadress->nodeNr,sizeof(xmlNodePtr));
	if(!(pci->registers_xml_nodes)) pcilib_warning("can't create registers xml nodes in pcilib_t struct");
		
	/** we then iterate through standard registers nodes to create registers structures*/
	for(i=0;i<nodesetadress->nodeNr;i++){
		type=(xmlChar*)"standard";
		mynode=nodesetadress->nodeTab[i];
		pcilib_xml_create_register(&myregister,mynode,doc,type);
		registers[i]=myregister;
		pci->registers_xml_nodes[i]=mynode;
	}
	
	j=i;
	/** we then iterate through bits  registers nodes to create registers structures*/
	for(i=0;i<nodesetsubadress->nodeNr;i++){
	        type=(xmlChar*)"bits";
		mynode=nodesetsubadress->nodeTab[i];
		pcilib_xml_create_register(&myregister,mynode,doc,type);
		registers[i+j]=myregister;
		pci->registers_xml_nodes[i+j]=mynode;
	}
	
	/**we arrange the register for them to be well placed for pci-l*/
	pcilib_xml_arrange_registers(registers,nodesetadress->nodeNr+nodesetsubadress->nodeNr);
	/**we fille the pcilib_t struct*/
        pcilib_add_registers(pci,number_registers,registers);
}


/** pcilib_init_xml
 * this function will initialize the registers and banks from the xml files
 * @param[in,out] ctx the pciilib_t running that gets filled with structures
 */

int pcilib_init_xml(pcilib_t* ctx){
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
    pwd=malloc((strlen(path)+strlen(ctx->model))*sizeof(char));
    sprintf(pwd,"%s%s/",path,ctx->model);
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
      err=validation(line[k],line_xsd);
      if(err==0){
	docs[k]=pcilib_xml_getdoc(line[k]);
	pcilib_xml_initialize_banks(ctx,docs[k]);
	pcilib_xml_initialize_registers(ctx,docs[k]);
      }
    }  
    
    //    free(path);
    free(pwd);
    free(line);
    free(line_xsd);
    free(docs);
    return 0;
}  

