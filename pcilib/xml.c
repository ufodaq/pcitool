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

#include "xml.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <Python.h>
#include "pci.h"
//#define VIEW_OK
//#define UNIT_OK

/**
 * pcilib_xml_getdoc
 * this function takes a string and will create an abtract syntax tree from the xml file represented by the string
 * @param[in] filename the the name of the xml file containing registers and banks 
 */
xmlDocPtr pcilib_xml_getdoc(char* filename){
		xmlDocPtr doc;
		doc=xmlParseFile(filename); /**<the creation of the AST from a libxml2 funtion*/
		
		if (doc==NULL){
			pcilib_error("xml document not parsed successfully");
			return NULL;
		}
		
		return doc;
}

/** pcilib_xml_getsetproperty
 *
 * this function takes a context from an AST and an XPath expression, to produce an object containing the nodes corresponding to the xpath expression
 * @param[in] doc the AST of the xml file
 * @param[in] xpath the xpath expression that will be evaluated
 */
xmlXPathObjectPtr pcilib_xml_getsetproperty(xmlXPathContextPtr doc, xmlChar *xpath){
	xmlXPathObjectPtr result;
	result=xmlXPathEvalExpression(xpath, doc); /**<the creation of the resulting object*/
	if(result==NULL){
//		pcilib_warning(" warning : the path is empty: %s",xpath);
		return NULL;
	}
	
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
//		pcilib_warning("warning : no results found for: %s",xpath);
		return NULL;
	}
	
	return result;
}

/** pcilib_xml_getcontext.
 * this function create a context in an AST (ie initialize XPAth for the AST).
 * @param[in] doc the AST of the xml file.
 */
xmlXPathContextPtr pcilib_xml_getcontext(xmlDocPtr doc){
  xmlXPathContextPtr context;
	  context= xmlXPathNewContext(doc); /**<the creation of the context using a libxml2's function */
	 if(context==NULL){
	    pcilib_error("warning : no context obtained for the xml document");
	     return NULL;
		}
	return context;
}


/** pcilib_xml_create_register.
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
 */
void pcilib_xml_create_register(pcilib_register_description_t *myregister,xmlChar* adress, xmlChar *offset, xmlChar *size, xmlChar *defvalue, xmlChar *rwmask, xmlChar *mode, xmlChar *type, xmlChar *bank, xmlChar *name, xmlChar *description){
		
		char* ptr;
        
		 /* we recreate here each sub property of the register structure given the results of xml parsing
            note :the use of strtol permits to get as well hexadecimal and decimal values
		*/
        
		myregister->addr=(pcilib_register_addr_t)strtol((char*)adress,&ptr,0);
		myregister->offset=(pcilib_register_size_t)strtol((char*)offset,&ptr,0);
		myregister->bits=(pcilib_register_size_t)strtol((char*)size,&ptr,0);
		myregister->defvalue=(pcilib_register_value_t)strtol((char*)defvalue,&ptr,0);
		
		if(strcmp((char*)rwmask,"all bits")==0){
			myregister->rwmask=PCILIB_REGISTER_ALL_BITS;
		}else if(strcmp((char*)rwmask,"0")==0){
			myregister->rwmask=0;
		}else{
			myregister->rwmask=(pcilib_register_value_t)strtol((char*)rwmask,&ptr,0);
		}
		
		if(strcmp((char*)mode,"R")==0){
			myregister->mode=PCILIB_REGISTER_R;
		}else if(strcmp((char*)mode,"W")==0){
			myregister->mode=PCILIB_REGISTER_W;
		}else if(strcmp((char*)mode,"RW")==0){
			myregister->mode=PCILIB_REGISTER_RW;
		}else if(strcmp((char*)mode,"W")==0){
			myregister->mode=PCILIB_REGISTER_W;
		}else if(strcmp((char*)mode,"RW1C")==0){
			myregister->mode=PCILIB_REGISTER_RW1C;
		}else if(strcmp((char*)mode,"W1C")==0){
			myregister->mode=PCILIB_REGISTER_W1C;
		}

		if(strcmp((char*)type,"standard")==0){
			myregister->type=PCILIB_REGISTER_STANDARD;
		}else if(strcmp((char*)type,"bits")==0){
			myregister->type=PCILIB_REGISTER_BITS;
		}else if(strcmp((char*)type,"fifo")==0){
			myregister->type=PCILIB_REGISTER_FIFO;
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
		
		myregister->name=(char*)name;
		myregister->description=(char*)description;
}	

/** pcilib_xml_getnumberbanks
 *
 * get the number of banks nodes in the AST, used to have some malloc in pci
 * @param[in] doc the Xpath context of the xml file.
 * @return the number of banks.
 */
int pcilib_xml_getnumberbanks(xmlXPathContextPtr doc){
	xmlNodeSetPtr nodesetadress=NULL;
	xmlXPathObjectPtr temp;
	temp=pcilib_xml_getsetproperty(doc,BANK_ADDR_PATH); /**< we first get a structure containing all the banks nodes*/
	if(temp!=NULL) nodesetadress=temp->nodesetval;
	else pcilib_error("no bank in the xml file");
	return nodesetadress->nodeNr; /**< we then return the number of said nodes */
}

/** pcilib_xml_create_bank
 * 
 * this function create a bank structure from the results of xml parsing
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
 */
void pcilib_xml_create_bank(pcilib_register_bank_description_t *mybank,xmlChar* adress, xmlChar *bar, xmlChar *size, xmlChar *protocol, xmlChar *read_addr, xmlChar *write_addr, xmlChar *access, xmlChar *endianess, xmlChar *format, xmlChar *name,xmlChar *description){
		
		char* ptr;

		/** we recreate each sub property of banks' structure given the results of xml parsing
            note : strtol is used here to get hexadecimal and decimal values from the xml file as well*/

		if (strcmp((char*)adress,"bank 0")==0){
				mybank->addr=PCILIB_REGISTER_BANK0;
		}else if (strcmp((char*)adress,"bank 1")==0){
			    mybank->addr=PCILIB_REGISTER_BANK1;
	    }else if (strcmp((char*)adress,"bank 2")==0){
	            mybank->addr=PCILIB_REGISTER_BANK2;
	    }else if (strcmp((char*)adress,"bank 3")==0){
	            mybank->addr=PCILIB_REGISTER_BANK3;
	    }else if (strcmp((char*)adress,"DMA bank")==0){
	            mybank->addr=PCILIB_REGISTER_BANK_DMA;
	    }else if (strcmp((char*)adress,"DMAconf bank")==0){
	            mybank->addr=PCILIB_REGISTER_BANK_DMACONF;
	    }else if (strcmp((char*)adress,"dynamic bank")==0){
	            mybank->addr=PCILIB_REGISTER_BANK_DYNAMIC;
		}else{
				mybank->addr=PCILIB_REGISTER_BANK_INVALID;
		}
		
		if(strcmp((char*)bar,"0")==0){
			mybank->bar=PCILIB_BAR0;
		}else if(strcmp((char*)bar,"1")==0){
			mybank->bar=PCILIB_BAR1;
		}else if(strcmp((char*)bar,"no_bar")==0){
			mybank->bar=PCILIB_BAR_NOBAR;
		}else{
		  mybank->bar=PCILIB_BAR_INVALID;
		}
		
		mybank->bar=(pcilib_bar_t)strtol((char*)bar,&ptr,0);
		mybank->size=(size_t)strtol((char*)size,&ptr,0);
		
		if(strcmp((char*)protocol,"default")==0){
			mybank->protocol=PCILIB_REGISTER_PROTOCOL_DEFAULT;
		}else if(strcmp((char*)protocol,"0")==0){
			mybank->protocol=PCILIB_REGISTER_PROTOCOL0;
		}else if(strcmp((char*)protocol,"dma")==0){
			mybank->protocol=PCILIB_REGISTER_PROTOCOL_DMA;
		}else if(strcmp((char*)protocol,"dynamic")==0){
			mybank->protocol=PCILIB_REGISTER_PROTOCOL_DYNAMIC;
		}else if (strcmp((char*)protocol,"software")==0){
			mybank->protocol=PCILIB_REGISTER_PROTOCOL_SOFTWARE;
		}else mybank->protocol=PCILIB_REGISTER_PROTOCOL_INVALID;
		
		mybank->read_addr=(uintptr_t)strtol((char*)read_addr,&ptr,0);
		mybank->write_addr=(uintptr_t)strtol((char*)write_addr,&ptr,0);
		mybank->access=(uint8_t)strtol((char*)access,&ptr,0);
		
		if(strcmp((char*)endianess,"little")==0){
			mybank->endianess=PCILIB_LITTLE_ENDIAN;
		}else if(strcmp((char*)endianess,"big")==0){
			mybank->endianess=PCILIB_BIG_ENDIAN;
		}else if(strcmp((char*)endianess,"host")==0){
			mybank->endianess=PCILIB_HOST_ENDIAN;
		}		
		mybank->format=(char*)format;
		mybank->raw_endianess=mybank->endianess;

		mybank->name=(char*)name;
		mybank->description=(char*)description;
}	

/** pcilib_xml_initialize_banks
 * 
 * function to create the structures to store the banks from the AST 
 * @see pcilib_xml_create_bank(pcilib_register_bank_description_t *mybank,xmlChar* adress, xmlChar *bar, xmlChar *size, xmlChar *protocol,xmlChar *read_addr, xmlChar *write_addr, xmlChar *access, xmlChar *endianess, xmlChar *format, xmlChar *name,xmlChar *description, xmlNodePtr node).
 * @param[in] doc the AST of the xml file.
 * @param[in,out] mybanks the structure containing the banks.
 */
void pcilib_xml_initialize_banks(pcilib_t* pci, xmlDocPtr doc, pcilib_register_bank_description_t* mybanks){
	pcilib_register_bank_description_t mybank;

	xmlNodeSetPtr nodesetadress=NULL,nodesetbar=NULL,nodesetsize=NULL,nodesetprotocol=NULL,nodesetread_addr=NULL,nodesetwrite_addr=NULL,nodesetaccess=NULL,nodesetendianess=NULL,nodesetformat=NULL,nodesetname=NULL,nodesetdescription=NULL;
	xmlChar *adress=NULL,*bar=NULL,*size=NULL,*protocol=NULL,*read_addr=NULL,*write_addr=NULL,*access=NULL,*endianess=NULL,*format=NULL,*name=NULL,*description=NULL;
	xmlNodePtr mynode;	
	
	xmlXPathContextPtr context;
	context=pcilib_xml_getcontext(doc);

	int i;
	
	mynode=malloc(sizeof(xmlNode));

	xmlXPathObjectPtr temp;
	
	/** we first get the nodes corresponding to the properties we want
         * note: here a recursive algorithm may be more efficient but less evolutive*/
	temp=pcilib_xml_getsetproperty(context,BANK_ADDR_PATH);
	if(temp!=NULL) nodesetadress=temp->nodesetval;
	else pcilib_error("there is no adress for banks in the xml");
	
	temp=pcilib_xml_getsetproperty(context,BANK_BAR_PATH);
	if(temp!=NULL) nodesetbar=temp->nodesetval;
	else pcilib_error("there is no bar for banks in the xml");

	temp=pcilib_xml_getsetproperty(context,BANK_SIZE_PATH);
	if(temp!=NULL) nodesetsize=temp->nodesetval;
	else pcilib_error("there is no size for banks in the xml");

	temp=pcilib_xml_getsetproperty(context,BANK_PROTOCOL_PATH);
	if(temp!=NULL) nodesetprotocol= temp->nodesetval;
	else pcilib_error("there is no protocol for banks in the xml");

	temp=pcilib_xml_getsetproperty(context,BANK_READ_ADDR_PATH);
	if(temp!=NULL) nodesetread_addr=temp->nodesetval;
	else pcilib_error("there is no read_adress for banks in the xml");
	
	temp=pcilib_xml_getsetproperty(context,BANK_WRITE_ADDR_PATH);
	if(temp!=NULL)nodesetwrite_addr=temp->nodesetval;
	else pcilib_error("there is no write_adress for banks in the xml");

	temp=pcilib_xml_getsetproperty(context,BANK_ACCESS_PATH);
	if(temp!=NULL)nodesetaccess=temp->nodesetval;
	else pcilib_error("there is no access for banks in the xml");

	temp=pcilib_xml_getsetproperty(context,BANK_ENDIANESS_PATH);
	if(temp!=NULL) nodesetendianess=temp->nodesetval;
	else pcilib_error("there is no endianess for banks in the xml");

	temp=pcilib_xml_getsetproperty(context,BANK_FORMAT_PATH);
	if(temp!=NULL) nodesetformat=temp->nodesetval;
	else pcilib_error("there is no format for banks in the xml");

	temp=pcilib_xml_getsetproperty(context,BANK_NAME_PATH);
	if(temp!=NULL)nodesetname=temp->nodesetval;
	else pcilib_error("there is no name for banks in the xml");

	temp=pcilib_xml_getsetproperty(context,BANK_DESCRIPTION_PATH);
	if(temp!=NULL)nodesetdescription=temp->nodesetval;
	
	pci->banks_xml_nodes=calloc(nodesetadress->nodeNr,sizeof(xmlNodePtr));
	if(!(pci->banks_xml_nodes)) pcilib_warning("can't create bank xml nodes for pcilib_t struct");
	
	for(i=0;i<nodesetadress->nodeNr;i++){
	  /** we then get each node from the structures above*/
		adress=xmlNodeListGetString(doc,nodesetadress->nodeTab[i]->xmlChildrenNode, 1);
		bar=xmlNodeListGetString(doc,nodesetbar->nodeTab[i]->xmlChildrenNode, 1);
		size=xmlNodeListGetString(doc,nodesetsize->nodeTab[i]->xmlChildrenNode, 1);
		protocol=xmlNodeListGetString(doc,nodesetprotocol->nodeTab[i]->xmlChildrenNode, 1);
		read_addr=xmlNodeListGetString(doc,nodesetread_addr->nodeTab[i]->xmlChildrenNode, 1);
		write_addr=xmlNodeListGetString(doc,nodesetwrite_addr->nodeTab[i]->xmlChildrenNode, 1);
		access=xmlNodeListGetString(doc,nodesetaccess->nodeTab[i]->xmlChildrenNode, 1);
		endianess=xmlNodeListGetString(doc,nodesetendianess->nodeTab[i]->xmlChildrenNode, 1);
		format=xmlNodeListGetString(doc,nodesetformat->nodeTab[i]->xmlChildrenNode, 1);
		name=xmlNodeListGetString(doc,nodesetname->nodeTab[i]->xmlChildrenNode, 1);
		description=xmlNodeListGetString(doc,nodesetdescription->nodeTab[i]->xmlChildrenNode, 1);

		mynode=nodesetadress->nodeTab[i]->parent;

		/** the following function will create the given structure for banks*/
		pcilib_xml_create_bank(&mybank,adress,bar,size,protocol,read_addr,write_addr,access,endianess,format, name, description);
		mybanks[i]=mybank;
		pci->banks_xml_nodes[i]=mynode;

	}

}



/** pcilib_xml_getnumberregisters
 *
 * get the number of registers in the xml document, for further mallocs
 * @param[in] doc the xpath context of the xml file.
 * @return the numbers of registers in xml file.
 */
int pcilib_xml_getnumberregisters(xmlXPathContextPtr doc){
	xmlNodeSetPtr nodesetadress=NULL,nodesetsuboffset=NULL;
	xmlXPathObjectPtr temp,temp2;
	temp=pcilib_xml_getsetproperty(doc,ADRESS_PATH); /**<we get the structure with all standards registers here*/
	if(temp!=NULL) nodesetadress=temp->nodesetval; /**<we get the structure with all standards registers here*/
	else {
		pcilib_error("no registers found in the xml file");
		return 0;
	}

	temp2=pcilib_xml_getsetproperty(doc,SUB_OFFSET_PATH);/**< we get the structure with all bits registers here */
	if(temp2!=NULL) nodesetsuboffset=temp2->nodesetval;/**< we get the structure with all bits registers here */
	else nodesetsuboffset->nodeNr=0;
	return nodesetadress->nodeNr + nodesetsuboffset->nodeNr; /**<we sum the number of nodes in the first structure to the number of nodes in the second one*/
}

/** pcilib_xml_initialize_registers
 *
 * this function create a list of registers from an abstract syntax tree
 *
 * variables who need change if xml structure change : bank,description, sub_description, sub_adress, sub_bank, sub_rwmask (we consider it to be equal to the standard register), sub_defvalue(same to standard register normally)
 * @param[in] doc the xpath context of the xml file.
 * @param[in,out] registers in: initialized list out: the list of the created registers.
 */
void pcilib_xml_initialize_registers(pcilib_t* pci, xmlDocPtr doc,pcilib_register_description_t *registers){
	
	xmlNodeSetPtr nodesetadress=NULL,nodesetoffset=NULL,nodesetdefvalue=NULL,nodesetrwmask=NULL,nodesetsize=NULL,nodesetmode=NULL,nodesetname=NULL;
	xmlChar *adress=NULL,*offset=NULL,*defvalue=NULL,*rwmask=NULL,*size=NULL,*mode=NULL,*name=NULL,*bank=NULL,*type=NULL,*description=NULL;
	xmlNodePtr mynode;	
	xmlNodePtr tempnode;	
	xmlXPathContextPtr context;
	context=pcilib_xml_getcontext(doc);

	mynode=malloc(sizeof(xmlNode));
	
	xmlXPathObjectPtr temp;

	/** get the sructures containing each property of standard regsiter */
	temp=pcilib_xml_getsetproperty(context,ADRESS_PATH);
	if(temp!=NULL)nodesetadress=temp->nodesetval;
	else pcilib_error("no adress for registers found in xml");
	
	temp=pcilib_xml_getsetproperty(context,OFFSET_PATH);
	if(temp!=NULL)nodesetoffset=temp->nodesetval;
	else pcilib_error("no offset for registers found in xml");

	temp=pcilib_xml_getsetproperty(context,DEFVALUE_PATH);
	if(temp!=NULL)nodesetdefvalue=temp->nodesetval;
	else pcilib_error("no defvalue for registers found in xml");

	temp=pcilib_xml_getsetproperty(context,RWMASK_PATH);
	if(temp!=NULL)nodesetrwmask=temp->nodesetval;
	else pcilib_error("no rwmask for registers found in xml");

	temp=pcilib_xml_getsetproperty(context,SIZE_PATH);
	if(temp!=NULL)nodesetsize=temp->nodesetval;
	else pcilib_error("no size for registers found in xml");

	temp=pcilib_xml_getsetproperty(context,MODE_PATH);
	if(temp!=NULL)nodesetmode=temp->nodesetval;
	else pcilib_error("no mode for registers found in xml");

	temp=pcilib_xml_getsetproperty(context,NAME_PATH);
	if(temp!=NULL)nodesetname=temp->nodesetval;
	else pcilib_error("no name for registers found in xml");
	
	xmlNodeSetPtr nodesetsuboffset=NULL,nodesetsubsize=NULL,nodesetsubmode=NULL,nodesetsubname=NULL;
	xmlChar *subadress=NULL,*suboffset=NULL,*subdefvalue=NULL,*subrwmask=NULL,*subsize=NULL,*submode=NULL,*subname=NULL,*subbank=NULL,*subtype=NULL,*subdescription=NULL;
	
	/**get the structures containing each property of bits registers */
	temp=pcilib_xml_getsetproperty(context,SUB_OFFSET_PATH);
	if(temp!=NULL)nodesetsuboffset=temp->nodesetval;
	else pcilib_error("no offset for registers bits found in xml");

	temp=pcilib_xml_getsetproperty(context,SUB_SIZE_PATH);
	if(temp!=NULL)nodesetsubsize=temp->nodesetval;
	else pcilib_error("no size for registers bits found in xml");

	temp=pcilib_xml_getsetproperty(context,SUB_MODE_PATH);
	if(temp!=NULL)nodesetsubmode=temp->nodesetval;
	else pcilib_error("no mode for registers bits found in xml");

	temp=pcilib_xml_getsetproperty(context,SUB_NAME_PATH);
	if(temp!=NULL)nodesetsubname=temp->nodesetval;
	else pcilib_error("no name for registers bits found in xml");
	
	
	pcilib_register_description_t myregister;
	
	int i,j;
	
	pci->registers_xml_nodes=calloc(nodesetadress->nodeNr+nodesetsuboffset->nodeNr,sizeof(xmlNodePtr));
	if(!(pci->registers_xml_nodes)) pcilib_warning("can't create registers xml nodes in pcilib_t struct");
		
	for(i=0;i<nodesetadress->nodeNr;i++){
	  /** get each sub property of each standard registers*/
		adress=xmlNodeListGetString(doc,nodesetadress->nodeTab[i]->xmlChildrenNode, 1);
		tempnode=xmlFirstElementChild(xmlFirstElementChild(nodesetadress->nodeTab[i]->parent->parent->parent));
		if(strcmp("adress",(char*)tempnode->name)==0) bank=xmlNodeListGetString(doc,tempnode->xmlChildrenNode,1);
		else pcilib_error("the xml file is malformed");
		offset=xmlNodeListGetString(doc,nodesetoffset->nodeTab[i]->xmlChildrenNode, 1);
		size=xmlNodeListGetString(doc,nodesetsize->nodeTab[i]->xmlChildrenNode, 1);
		defvalue=xmlNodeListGetString(doc,nodesetdefvalue->nodeTab[i]->xmlChildrenNode, 1);
		rwmask=xmlNodeListGetString(doc,nodesetrwmask->nodeTab[i]->xmlChildrenNode, 1);
		mode=xmlNodeListGetString(doc,nodesetmode->nodeTab[i]->xmlChildrenNode, 1);
		name=xmlNodeListGetString(doc,nodesetname->nodeTab[i]->xmlChildrenNode, 1);
		type=(xmlChar*)"standard";
		if(nodesetname->nodeTab[i]->next->next!= NULL && strcmp((char*)nodesetname->nodeTab[i]->next->next->name,"description")==0){
			description=xmlNodeListGetString(doc,nodesetname->nodeTab[i]->next->next->xmlChildrenNode,1);
		}else{
			description=(xmlChar*)"";
		}
		mynode=nodesetadress->nodeTab[i]->parent;
		/**creation of a register with the given previous properties*/
		pcilib_xml_create_register(&myregister,adress,offset,size,defvalue,rwmask,mode, type, bank, name, description);
		registers[i]=myregister;
		pci->registers_xml_nodes[i]=mynode;
	}
	
	j=i;
	
	for(i=0;i<nodesetsuboffset->nodeNr;i++){
	  /** we get there each subproperty of each bits register*/
		tempnode=xmlFirstElementChild(nodesetsuboffset->nodeTab[i]->parent->parent->parent);
		if(strcmp((char*)tempnode->name,"adress")==0)subadress=xmlNodeListGetString(doc,tempnode->xmlChildrenNode, 1);
		else pcilib_error("xml file is malformed");
		tempnode= xmlFirstElementChild(xmlFirstElementChild(nodesetsuboffset->nodeTab[i]->parent->parent->parent->parent->parent));
		if(strcmp((char*)tempnode->name,"adress")==0) subbank=xmlNodeListGetString(doc,tempnode->xmlChildrenNode,1);
		else pcilib_error("xml file is malformed");
		suboffset=xmlNodeListGetString(doc,nodesetsuboffset->nodeTab[i]->xmlChildrenNode, 1);
		subsize=xmlNodeListGetString(doc,nodesetsubsize->nodeTab[i]->xmlChildrenNode, 1);
		tempnode=xmlFirstElementChild(nodesetsuboffset->nodeTab[i]->parent->parent->parent)->next->next->next->next->next->next;
		if(strcmp((char*)tempnode->name,"default")==0)subdefvalue=xmlNodeListGetString(doc,tempnode->xmlChildrenNode, 1);
		else pcilib_error("xml file is malformed");
		subrwmask=xmlNodeListGetString(doc,nodesetsuboffset->nodeTab[i]->parent->parent->prev->prev->prev->prev->prev->prev->xmlChildrenNode, 1);
		submode=xmlNodeListGetString(doc,nodesetsubmode->nodeTab[i]->xmlChildrenNode, 1);
		subname=xmlNodeListGetString(doc,nodesetsubname->nodeTab[i]->xmlChildrenNode, 1);
		subtype=(xmlChar*)"bits";
		if(nodesetsubname->nodeTab[i]->next->next!= NULL && strcmp((char*)nodesetsubname->nodeTab[i]->next->next->name,"sub_description")==0){
			subdescription=xmlNodeListGetString(doc,nodesetsubname->nodeTab[i]->next->next->xmlChildrenNode,1);
		}else{
			subdescription=(xmlChar*)"";
		}
		mynode=nodesetsuboffset->nodeTab[i]->parent;
		/** creation of a bits register given the previous properties*/
		pcilib_xml_create_register(&myregister,subadress,suboffset,subsize,subdefvalue,subrwmask,submode, subtype, subbank, subname, subdescription);
		registers[i+j]=myregister;
		pci->registers_xml_nodes[i+j]=mynode;
	}
}


/*
 * next 3 functions are for the implementation of a merge sort algorithm
 */
void topdownmerge(pcilib_register_description_t *A, int start, int middle, int end, pcilib_register_description_t *B){
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

void copyarray(pcilib_register_description_t *B, int start,int  end, pcilib_register_description_t *A){
	int k;
	for (k=start; k<end;k++) A[k]=B[k];
}

void topdownsplitmerge(pcilib_register_description_t *A, int start, int end, pcilib_register_description_t *B){
	int middle;
	if(end-start <2)
		return;
	
	middle =(end+start)/2;
	topdownsplitmerge(A,start, middle,B);
	topdownsplitmerge(A,middle,end,B);
	topdownmerge(A,start,middle,end,B);
	copyarray(B,start,end,A);
}

/** pcilib_xml_arrange_registers
 *
 * after the complete structure containing the registers has been created from the xml, this function rearrange them by address in order to add them in pcilib_open, which consider the adding of registers in order of adress	
 * @param[in,out] registers the list of registers in : not ranged out: ranged.
 * @param[in] size the number of registers. 
 */
void pcilib_xml_arrange_registers(pcilib_register_description_t *registers,int size){
	pcilib_register_description_t* temp;
	temp=malloc(size*sizeof(pcilib_register_description_t));
	topdownsplitmerge(registers,0,size,temp);
}

#include <libxml/xmlschemastypes.h>
/** validation
 *
 * function to validate the xml file against the xsd, so that it does not require extern software
 */
void validation()
{
xmlDocPtr doc;
xmlSchemaPtr schema = NULL;
xmlSchemaParserCtxtPtr ctxt;
  
char *XMLFileName;
 pcilib_xml_read_config(&XMLFileName,3);
char *XSDFileName;
    pcilib_xml_read_config(&XSDFileName,12);
int ret=1;

ctxt = xmlSchemaNewParserCtxt(XSDFileName);

schema = xmlSchemaParse(ctxt);
xmlSchemaFreeParserCtxt(ctxt);

doc = xmlReadFile(XMLFileName, NULL, 0);

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

//! free the resource
if(schema != NULL)
xmlSchemaFree(schema);

xmlSchemaCleanupTypes();
/** print results */
if (ret == 0)
{
	printf("xml file validates\n");
}
else
{
	printf("xml file does not validate against the schema\n");
}

}

/** pcilib_xml_read_config
 *
 * function to get the config we want at line j, in order to be able to have a configuration file with pwd to files we want
* @param[in,out] xmlfile the string representating a pwd to a file we want to access in:uninitilized out: the pwd.
* @param[in] i the line at which the function should read the file to get the pwd.
 */
void pcilib_xml_read_config(char** xmlfile, int j){
	FILE *fp;

	fp=fopen("config.txt","r");
	char line[60];
	memset(line,'\0',60);
	int i=0,k;
	char *temp1;
	if(fp==NULL) pcilib_error("can't find the configuration file: it must be near the executable");

	while(fgets(line,60,fp)!=NULL){
		if(i==j) {
			k=0;
			temp1=calloc((strlen(line)),sizeof(char));
			while(line[k]!='\n'){
				temp1[k]=line[k];
				k++;
			}
			temp1[k]='\0';
			*xmlfile=temp1;
		}
		i++;
		memset(line,'\0',60);
	}
}
/* to include or not?*/
/** pcilib_xml_init_nodeset_register_ctx
 *
 * function to get all registers nodes in a structure and put it in registers context
* @param[in] ctx the register context running.
 *
void pcilib_xml_init_nodeset_register_ctx(pcilib_register_context_t *ctx){
	char* xmlfile;	
	pcilib_xml_read_config(&xmlfile,3);
    xmlDocPtr doc;
    doc=pcilib_xml_getdoc(xmlfile);
    xmlXPathContextPtr context;
    context=pcilib_xml_getcontext(doc);

	xmlNodeSetPtr registers;
	registers = pcilib_xml_getsetproperty(context, REGISTERS_PATH)->nodesetval;
	ctx->registers_nodes=registers;
}

** pcilib_xml_init_nodeset_bank_ctx
 *
 *function to get all banks nodes in a structure and put it in banks context
* @param[in] ctx the bank context running.
 *
void pcilib_xml_init_nodeset_bank_ctx(pcilib_register_bank_context_t *ctx){
	char* xmlfile;	
	pcilib_xml_read_config(&xmlfile,3);
    xmlDocPtr doc;
    doc=pcilib_xml_getdoc(xmlfile);
    xmlXPathContextPtr context;
    context=pcilib_xml_getcontext(doc);

	xmlNodeSetPtr banks;
	banks = pcilib_xml_getsetproperty(context, BANKS_PATH)->nodesetval;
	ctx->banks_nodes=banks;

}
*/

#ifdef VIEW_OK
/* pcilib_xml_getnumberformulaviews
 * 
 * function to get the number of views of type formula, for further malloc
 */
int pcilib_xml_getnumberformulaviews(xmlXPathContextPtr doc){
	int j;
	xmlNodeSetPtr nodeviewsset=NULL;
	nodeviewsset=pcilib_xml_getsetproperty(doc,VIEW_FORMULA_PATH)->nodesetval;
	j=nodeviewsset->nodeNr;
	return j;
	
}

/* pcilib_xml_getnumberenumviews
 *
 * function to get the number of views of type enum, for further malloc
 */
int pcilib_xml_getnumberenumviews(xmlXPathContextPtr doc){
	int j;
	   xmlNodeSetPtr nodeviewsset=NULL;
	nodeviewsset=pcilib_xml_getsetproperty(doc,VIEW_ENUM_PATH)->nodesetval;
	j=nodeviewsset->nodeNr;
	return j;
	
	
}

/* pcilib_xml_initialize_viewsformula
 * 
 * function to create the structures to store the views of type formula from the xml
 *
 * values that need to be changed when xml file change: name, formula,
 */
void pcilib_xml_initialize_viewsformula(xmlDocPtr doc, pcilib_view_formula_ptr_t* myviewsformula){
	xmlNodeSetPtr viewsetformula, viewformula, viewreverseformula, viewunit, viewformulaname, nodesetviewregister,nodesetviewregisterbit,nodesetregister,nodesetsubregister;
	xmlChar *formula, *name,*reverseformula,*registername,*bankname,*unit;
	int i,j,registernumber,viewnumber,k,l,externregisternumber,enregistrement,u;

    char *substr;
	
	xmlXPathContextPtr context;
	context=pcilib_xml_getcontext(doc);

	viewsetformula=pcilib_xml_getsetproperty(context,VIEW_FORMULA_PATH)->nodesetval;
    viewformula=pcilib_xml_getsetproperty(context,VIEW_FORMULA_FORMULA_PATH)->nodesetval;
    viewreverseformula=pcilib_xml_getsetproperty(context,VIEW_FORMULA_REVERSE_FORMULA_PATH)->nodesetval;
    viewunit=pcilib_xml_getsetproperty(context,VIEW_FORMULA_UNIT_PATH)->nodesetval;
    viewformulaname=pcilib_xml_getsetproperty(context,VIEW_FORMULA_NAME_PATH)->nodesetval;
	


viewnumber=0;
	
    
	nodesetviewregisterbit=pcilib_xml_getsetproperty(context,VIEW_BITS_REGISTER)->nodesetval;
    nodesetviewregister=pcilib_xml_getsetproperty(context,VIEW_STANDARD_REGISTER)->nodesetval;
	nodesetregister=pcilib_xml_getsetproperty(context,NAME_PATH)->nodesetval;
    nodesetsubregister=pcilib_xml_getsetproperty(context,SUB_NAME_PATH)->nodesetval;

	myviewsformula->viewsformula=calloc(myviewsformula->size,sizeof(pcilib_view_formula_t));
	for(i=0;i<viewsetformula->nodeNr;i++){
		registernumber=0;
			name=xmlNodeListGetString(doc,viewformulaname->nodeTab[i]->xmlChildrenNode,1);
			formula=xmlNodeListGetString(doc,viewformula->nodeTab[i]->xmlChildrenNode,1);
			reverseformula=xmlNodeListGetString(doc,viewreverseformula->nodeTab[i]->xmlChildrenNode,1);
			unit=xmlNodeListGetString(doc,viewunit->nodeTab[i]->xmlChildrenNode,1);
	
			xmlXPathObjectPtr temp,temp2;

           	char* path;
			path=malloc((strlen((char*)name)+51)*sizeof(char));
			sprintf(path, VIEW_STANDARD_REGISTER_PLUS,(char*)name);
			temp=pcilib_xml_getsetproperty(context,(xmlChar*)path);
			if(temp!=NULL) nodesetviewregister= temp->nodesetval;	

			char* path2;
			path2=malloc((strlen((char*)name)+strlen(VIEW_BITS_REGISTER_PLUS))*sizeof(char));
			sprintf(path2, VIEW_BITS_REGISTER_PLUS,(char*)name);
			temp2= pcilib_xml_getsetproperty(context,(xmlChar*)path2);
			if(temp2!=NULL)	nodesetviewregisterbit= temp2->nodesetval;
			
		    j=0;
			if(temp!=NULL) j+=nodesetviewregister->nodeNr;
			if(temp2!=NULL) j+=nodesetviewregisterbit->nodeNr;
		   
			myviewsformula->viewsformula[viewnumber].depending_registers=calloc((j),sizeof(pcilib_register_for_read_t));
			myviewsformula->viewsformula[viewnumber].number_depending_registers=j;
            registernumber=0;
			if (temp!=NULL){
				for(j=0; j<nodesetviewregister->nodeNr;j++){
					myviewsformula->viewsformula[viewnumber].depending_registers[registernumber].name=(char*)xmlNodeListGetString(doc,nodesetviewregister->nodeTab[j]->parent->parent->last->prev->prev->prev->xmlChildrenNode,1);
					bankname=xmlNodeListGetString(doc,xmlFirstElementChild(nodesetviewregister->nodeTab[j]->parent->parent->parent->parent)->last->prev->prev->prev->xmlChildrenNode,1);
					myviewsformula->viewsformula[viewnumber].depending_registers[registernumber].bank_name=(char*)bankname;
					registernumber++;
				}
			}
			
			if(temp2!=NULL){
				for(j=0; j<nodesetviewregisterbit->nodeNr;j++){
					myviewsformula->viewsformula[viewnumber].depending_registers[registernumber].name=(char*)xmlNodeListGetString(doc,nodesetviewregisterbit->nodeTab[j]->parent->parent->last->prev->prev->prev->xmlChildrenNode,1);
					bankname=xmlNodeListGetString(doc,(xmlFirstElementChild(nodesetviewregisterbit->nodeTab[j]->parent->parent->parent->parent->parent->parent))->last->prev->prev->prev->xmlChildrenNode,1);
					myviewsformula->viewsformula[viewnumber].depending_registers[registernumber].bank_name=(char*)bankname;	   
				   registernumber++;
				}
			}
	
			myviewsformula->viewsformula[viewnumber].name=(char*) name;
			myviewsformula->viewsformula[viewnumber].formula=(char*) formula;
			myviewsformula->viewsformula[viewnumber].reverseformula=(char*) reverseformula;
			myviewsformula->viewsformula[viewnumber].unit=(char*) unit;

            /* we then get the extern registers for the formula, which names are directly put in the formula*/
			externregisternumber=0;
			k=0;	
			u=0;
            myviewsformula->viewsformula[viewnumber].extern_registers=calloc(1,sizeof(pcilib_register_for_read_t));
			myviewsformula->viewsformula[viewnumber].number_extern_registers=0;

            /* we first get the name of a register put in the formula, by getting indexes of start and end for this name*/
			for(j=0;j<strlen((char*)formula);j++){
				if(formula[j]=='@'){
					k=j+1;
					while((formula[k]!=' ' && formula[k]!=')' && formula[k]!='/' && formula[k]!='+' && formula[k]!='-' && formula[k]!='*' && formula[k]!='=') && (k<strlen((char*)formula))){
						k++;
					}
					enregistrement=1;
                    substr=str_sub((char*)formula,j+1,k-1);
                    /* we verify the register is not the depending register*/
					if(strcmp("reg",substr)!=0){
                        
                        /* we then get recursively each standard register and test if it's the extern register or not*/
						for(l=0;l<nodesetregister->nodeNr;l++){
							registername=xmlNodeListGetString(doc,nodesetregister->nodeTab[l]->xmlChildrenNode,1);
							if(strcmp((char*)registername,substr)==0){
								bankname=xmlNodeListGetString(doc,(xmlFirstElementChild(nodesetregister->nodeTab[l]->parent->parent->parent))->last->prev->prev->prev->xmlChildrenNode,1);              
                                /*before registering the extern register, as it matches, we tast if the said register was already registered or not*/
								for(u=0;u<externregisternumber;u++){
									if(strcmp(myviewsformula->viewsformula[viewnumber].extern_registers[u].name,(char*)registername)==0){
										enregistrement=0;                     
									}
								}
								if(enregistrement==1){
									myviewsformula->viewsformula[viewnumber].number_extern_registers++;
									myviewsformula->viewsformula[viewnumber].extern_registers=realloc(myviewsformula->viewsformula[viewnumber].extern_registers,myviewsformula->viewsformula[viewnumber].number_extern_registers*sizeof(pcilib_register_for_read_t));
									myviewsformula->viewsformula[viewnumber].extern_registers[externregisternumber].name=(char*)registername;
									myviewsformula->viewsformula[viewnumber].extern_registers[externregisternumber].bank_name=(char*)bankname;                          
                                    enregistrement=0;    
									externregisternumber++;
								}                         
								break;
							}
						}
						if(enregistrement==1){
	                        /* we get recursively each bits register and test if it's the extern register of the formula or not*/
                            for(l=0;l<nodesetsubregister->nodeNr;l++){
	    						registername=xmlNodeListGetString(doc,nodesetsubregister->nodeTab[l]->xmlChildrenNode, 1);
	    						if(strcmp((char*)registername,substr)==0){
	    							bankname=xmlNodeListGetString(doc,(xmlFirstElementChild(nodesetsubregister->nodeTab[l]->parent->parent->parent->parent->parent))->last->prev->prev->prev->xmlChildrenNode,1);
                                       /* check to test if the register has already been registered or not*/
	    							for(u=0;u<externregisternumber;u++){
	    								if(strcmp(myviewsformula->viewsformula[viewnumber].extern_registers[u].name,(char*)registername)==0){
	    									enregistrement=0;
	    									break;
	    								}
	    							}
	    							if(enregistrement==1){
                                        myviewsformula->viewsformula[viewnumber].number_extern_registers++;
										myviewsformula->viewsformula[viewnumber].extern_registers=realloc(myviewsformula->viewsformula[viewnumber].extern_registers,myviewsformula->viewsformula[viewnumber].number_extern_registers*sizeof(pcilib_register_for_read_t));
	    								myviewsformula->viewsformula[viewnumber].extern_registers[externregisternumber].name=(char*)registername;
	    								myviewsformula->viewsformula[viewnumber].extern_registers[externregisternumber].bank_name=(char*)bankname;
    									externregisternumber++;
                                        enregistrement=0;
							    	}
							    	break;
							    }
						    }
                        }
					}
				}
			}
			viewnumber++;
	}
}

/* pcilib_xml_initialize_viewsenum
 * 
 * function to create the structures to store the views of type enum from the xml
 *
 * values that need to be changed when xml file change: current,
*
 */
void pcilib_xml_initialize_viewsenum(xmlDocPtr doc, pcilib_view_enum_ptr_t* myviewsenum){
	xmlNodeSetPtr viewsetenum,viewsetregister3, viewsetregister4;
	xmlChar *name;
	int i,k,viewnumber,j,registernumber;

	xmlXPathContextPtr context;
	context=pcilib_xml_getcontext(doc);
	
	xmlXPathObjectPtr temp,temp2;

	xmlNodePtr current;
	viewsetenum=pcilib_xml_getsetproperty(context,VIEW_ENUM_PATH)->nodesetval;
	xmlAttr *attr2,*attr3;
	viewnumber=0;
	int enumnumber=0;

	myviewsenum->viewsenum=calloc(myviewsenum->size,sizeof(pcilib_view_enum_t));	
        for(i=0;i<viewsetenum->nodeNr;i++){
			enumnumber=0;
			k=0;
			registernumber=0;
			name=xmlNodeListGetString(doc,xmlFirstElementChild(viewsetenum->nodeTab[i])->xmlChildrenNode,1);
			myviewsenum->viewsenum[viewnumber].name=(char*)name;
			myviewsenum->viewsenum[viewnumber].registerenum=calloc(1,sizeof(char*));
			myviewsenum->viewsenum[viewnumber].number_registerenum=0;
			registernumber=0;
			
			char* path;
			path=malloc((strlen((char*)name)+51)*sizeof(char));
			sprintf(path, VIEW_STANDARD_REGISTER_PLUS,(char*)name);
			temp=pcilib_xml_getsetproperty(context,(xmlChar*)path);
			if(temp!=NULL) viewsetregister3= temp->nodesetval;	

			char* path2;
			path2=malloc((strlen((char*)name)+strlen(VIEW_BITS_REGISTER_PLUS))*sizeof(char));
			sprintf(path2, VIEW_BITS_REGISTER_PLUS,(char*)name);
			temp2= pcilib_xml_getsetproperty(context,(xmlChar*)path2);
			if(temp2!=NULL)	viewsetregister4= temp2->nodesetval;
			
			j=0;
			if(temp!=NULL) j+=viewsetregister3->nodeNr;
			if(temp2!=NULL) j+=viewsetregister4->nodeNr;
			myviewsenum->viewsenum[viewnumber].registerenum=malloc(j*sizeof(char*));
			myviewsenum->viewsenum[viewnumber].number_registerenum=j;
			if(temp!=NULL){
				for(j=0; j<viewsetregister3->nodeNr;j++){
						myviewsenum->viewsenum[viewnumber].registerenum[registernumber]=(char*)xmlNodeListGetString(doc,viewsetregister3->nodeTab[j]->parent->parent->last->prev->prev->prev->xmlChildrenNode,1);
						registernumber++;
				}
			}
			if(temp2!=NULL){
				for(j=0; j<viewsetregister4->nodeNr;j++){
						myviewsenum->viewsenum[viewnumber].registerenum[registernumber]=(char*)xmlNodeListGetString(doc,viewsetregister4->nodeTab[j]->parent->parent->last->prev->prev->prev->xmlChildrenNode,1);
						registernumber++;
					}
				}
			
			
			current=xmlFirstElementChild(viewsetenum->nodeTab[i])->next->next->next->next;
			while(current!=viewsetenum->nodeTab[i]->last->prev){
				k++;
				current=current->next->next;
			}
             
			myviewsenum->viewsenum[viewnumber].myenums=calloc((k+1),sizeof(pcilib_enum_t));
			myviewsenum->viewsenum[viewnumber].number_enums=k+1;
            
            /* we get here each enum for a said view except the last one*/
			current=xmlFirstElementChild(viewsetenum->nodeTab[i])->next->next;
			while(current!=viewsetenum->nodeTab[i]->last->prev->prev->prev){
				attr2 = current->properties;
				myviewsenum->viewsenum[viewnumber].myenums[enumnumber].command=(char*)xmlNodeListGetString(doc,current->xmlChildrenNode,1);
				myviewsenum->viewsenum[viewnumber].myenums[enumnumber].value=(char*)attr2->children->content;
				attr2=attr2->next;
                /* and we take the properties that are given about range*/
				if(attr2!=NULL){
					if(strcmp((char*)attr2->name,"min")==0){
						myviewsenum->viewsenum[viewnumber].myenums[enumnumber].min=(char*)attr2->children->content;
						attr3=attr2->next;
						if(attr3 !=NULL){
							if(strcmp((char*)attr3->name,"max")==0){
								myviewsenum->viewsenum[viewnumber].myenums[enumnumber].max=(char*)attr3->children->content;
							}else pcilib_error("problem in xml file at enum");
										
						}else{
							myviewsenum->viewsenum[viewnumber].myenums[enumnumber].max=myviewsenum->viewsenum[viewnumber].myenums[enumnumber].value;
						}
									
					}else if(strcmp((char*)attr2->name,"max")==0){
						myviewsenum->viewsenum[viewnumber].myenums[enumnumber].min=myviewsenum->viewsenum[viewnumber].myenums[enumnumber].value;
						myviewsenum->viewsenum[viewnumber].myenums[enumnumber].max=(char*)attr2->children->content;
					}else pcilib_error("problem in xml file at enum");
				}else{
					myviewsenum->viewsenum[viewnumber].myenums[enumnumber].min=myviewsenum->viewsenum[viewnumber].myenums[enumnumber].value;
					myviewsenum->viewsenum[viewnumber].myenums[enumnumber].max=myviewsenum->viewsenum[viewnumber].myenums[enumnumber].value;
				}
				enumnumber++;
				current=current->next->next;
			}
            /* get the last enum and the given properties */
			myviewsenum->viewsenum[viewnumber].myenums[enumnumber].command=(char*)xmlNodeListGetString(doc,current->xmlChildrenNode,1);
			attr2 = current->properties;
			myviewsenum->viewsenum[viewnumber].myenums[enumnumber].value=(char*)attr2->children->content;
			attr2=attr2->next;
			if(attr2!=NULL){
				if(strcmp((char*)attr2->name,"min")==0){
					myviewsenum->viewsenum[viewnumber].myenums[enumnumber].min=(char*)attr2->children->content;
						attr3=attr2->next;
						if(attr3 !=NULL){
							if(strcmp((char*)attr3->name,"max")==0){
								myviewsenum->viewsenum[viewnumber].myenums[enumnumber].max=(char*)attr3->children->content;
							}else pcilib_error("problem in xml file at enum");
						}else{
							myviewsenum->viewsenum[viewnumber].myenums[enumnumber].max=myviewsenum->viewsenum[viewnumber].myenums[enumnumber].value;
						}
								
				}else if(strcmp((char*)attr2->name,"max")==0){
					myviewsenum->viewsenum[viewnumber].myenums[enumnumber].min=myviewsenum->viewsenum[viewnumber].myenums[enumnumber].value;
					myviewsenum->viewsenum[viewnumber].myenums[enumnumber].max=(char*)attr2->children->content;
				}else pcilib_error("problem in xml file at enum");
			}else{
				myviewsenum->viewsenum[viewnumber].myenums[enumnumber].min=myviewsenum->viewsenum[viewnumber].myenums[enumnumber].value;
				myviewsenum->viewsenum[viewnumber].myenums[enumnumber].max=myviewsenum->viewsenum[viewnumber].myenums[enumnumber].value;
			}

			viewnumber++;

	}
}
#endif

#ifdef UNIT_OK

void pclib_xml_initialize_units(xmlDocPtr doc, pcilib_units_ptr_t* unitsptr){

  xmlNodeSetPtr nodesetbaseunit=NULL, nodesettransformunit=NULL;
  xmlXPathContextptr context;
  context=pcilib_xml_getcontext(doc);
  xmlXPathObjectPtr temp;
  int i,j;
  char *path;

  temp=pcilib_xml_getsetproperty(context, BASE_UNIT_PATH);
  if(temp!=NULL) nodesetbaseunit=temp->nodesetval;
  else pcilib_error("the unit xml file is wrong");
  
  unitsptr->list_unit=malloc(nodesetbaseunit->nodeNr*sizeof(pcilib_unit_t));
  unitsptr->size=nodesetbaseunit->nodeNr; 
  
 for(i=0; i< nodesetbaseunit->NodeNr; i++){
   unitsptr->list_unit[i].name=nodesetbaseunit->nodeTab[i]->properties->children->content;
   path=malloc((strlen(unitsptr->list_unit[i].name)+35)*sizeof(char));
   sprintf(path, TRANSFORM_UNIT_PATH, unitsptr->list_unit[i].name);
   temp=pcilib_xml_getsetproperty(context, path);
   if(temp!=NULL){
     unitsptr->list_unit[i].size_trans_units=temp->nodeNr;
     unitsptr->list_unit[i].other_units=malloc(temp->nodeNr*sizeof(pcilib_transform_unit_t));
     for(j=0;j<temp->nodeNr;j++){
       unitsptr->list_unit[i].other_units[j].name=temp->nodeTab[j]->properties->children->content;
       unitsptr->list_unit[i].other_units[j].transform_formula=(char*)xmlNodeListGetString(doc,temp->nodeTab[j]->xmlChildrenNode,1);
     }
   }
 }
 
}
#endif
