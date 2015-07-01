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
 * @param[in] node the current xmlNode in the xml of the future register
 */
void pcilib_xml_create_register(pcilib_register_description_t *myregister,xmlChar* adress, xmlChar *offset, xmlChar *size, xmlChar *defvalue, xmlChar *rwmask, xmlChar *mode, xmlChar *type, xmlChar *bank, xmlChar *name, xmlChar *description, xmlNodePtr node){
		
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
		/*should we include those xmlnodes?*/
		//myregister->xmlNode=node;
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

/** pcilib_xml_initialize_banks
 * 
 * function to create the structures to store the banks from the AST 
 * @see pcilib_xml_create_bank(pcilib_register_bank_description_t *mybank,xmlChar* adress, xmlChar *bar, xmlChar *size, xmlChar *protocol,xmlChar *read_addr, xmlChar *write_addr, xmlChar *access, xmlChar *endianess, xmlChar *format, xmlChar *name,xmlChar *description, xmlNodePtr node).
 * @param[in] doc the AST of the xml file.
 * @param[in,out] mybanks the structure containing the banks.
 */
void pcilib_xml_initialize_banks(xmlDocPtr doc, pcilib_register_bank_description_t* mybanks){
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
		pcilib_xml_create_bank(&mybank,adress,bar,size,protocol,read_addr,write_addr,access,endianess,format, name, description,mynode);
		mybanks[i]=mybank;
	}

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
 * @param[in] node the xmlNodeptr referring to the bank_description node of the bank that will be created.
 */
void pcilib_xml_create_bank(pcilib_register_bank_description_t *mybank,xmlChar* adress, xmlChar *bar, xmlChar *size, xmlChar *protocol, xmlChar *read_addr, xmlChar *write_addr, xmlChar *access, xmlChar *endianess, xmlChar *format, xmlChar *name,xmlChar *description, xmlNodePtr node){
		
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
		}else if(strcmp((char*)bar,"ipecamera_register")==0){
			mybank->bar=PCILIB_BAR1;
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
		
		mybank->raw_endianess=mybank->endianess;

		mybank->name=(char*)name;
		mybank->description=(char*)description;
		/* to include or not?*/
		//mybank->xmlNode=node;
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
void pcilib_xml_initialize_registers(xmlDocPtr doc,pcilib_register_description_t *registers){
	
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
		pcilib_xml_create_register(&myregister,adress,offset,size,defvalue,rwmask,mode, type, bank, name, description,mynode);
		registers[i]=myregister;
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
		pcilib_xml_create_register(&myregister,subadress,suboffset,subsize,subdefvalue,subrwmask,submode, subtype, subbank, subname, subdescription,mynode);
		registers[i+j]=myregister;
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
