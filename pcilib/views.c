#include <string.h>
#include "pci.h"
#include "pcilib.h"
#include <Python.h>
//#include "views.h"
#include "error.h"
#include <strings.h>
#include <stdlib.h>

/**
 *
 * replace a substring within a string by another
 * @param[in] txt - the string to be modified
 *@param[in] before - the substring in the string that will be replaced
 *@param[in] after - the new value of before substring
 *@return the modified txt string
 */
static char*
pcilib_view_formula_replace (const char *txt, const char *before, const char *after)
{
	const char *pos; 
	char *return_txt; 
	size_t pos_return_txt; 
	size_t len; 
	size_t allocated_size; 
	
	/*get the first occurence of before. then we need one time to be out of the loop to correctly set the diverses varaibles (malloc instead of realloc notably)*/
	pos = strstr (txt, before);
	 
	if (pos == NULL)
	{
		pcilib_warning("problem with a formula");
	}
 
	/* get the position of this occurence*/
	len = (size_t)pos - (size_t)txt;
	allocated_size = len + strlen (after) + 1;
	return_txt = malloc (allocated_size);
	pos_return_txt = 0;
	
	/* we copy there the in a newly allocated string the start of txt before the "before" occurence, and then we copy after instead*/
	strncpy (return_txt + pos_return_txt, txt, len);
	pos_return_txt += len;
	txt = pos + strlen (before);
	 
	len = strlen (after);
	strncpy (return_txt + pos_return_txt, after, len);
	pos_return_txt += len;
	
	/* we then iterate with the same principle to all occurences of before*/
	pos = strstr (txt, before);
	while (pos != NULL)
	{
		len = (size_t)pos - (size_t)txt;
		allocated_size += len + strlen (after);
		return_txt = (char *)realloc (return_txt, allocated_size);
		 
		strncpy (return_txt + pos_return_txt, txt, len);
		pos_return_txt += len;
		 
		txt = pos + strlen (before);
		 
		len = strlen (after);
		strncpy (return_txt + pos_return_txt, after, len);
		pos_return_txt += len;
		pos = strstr (txt, before);
	}
	/* put the rest of txt string at the end*/
	len = strlen (txt) + 1;
	allocated_size += len;
	return_txt = realloc (return_txt, allocated_size);
	strncpy (return_txt + pos_return_txt, txt, len);
	
	return return_txt;
}

/**
 * function used to get the substring of a string s, from the starting and ending indexes
  * @param[in] s string containing the substring we want to extract.
 * @param[in] start the start index of the substring.
 * @param[in] end the ending index of the substring.
 * @return the extracted substring.
 */
static char*
pcilib_view_str_sub (const char *s, unsigned int start, unsigned int end)
{
   char *new_s = NULL;

   if (s != NULL && start < end)
   {
      new_s = malloc (sizeof (*new_s) * (end - start + 2));
      if (new_s != NULL)
      {
         int i;

         for (i = start; i <= end; i++)
         {
            new_s[i-start] = s[i];
         }
         new_s[i-start] = '\0';
      }
      else
      {
         pcilib_error("insufficient memory for string manipulation\n");
         return NULL;
      }
   }
   return new_s;
}

/**
 * get the bank name associated with a register name
 */
static const char*
pcilib_view_get_bank_from_reg_name(pcilib_t* ctx,char* reg_name){
  int k;
  for(k=0;ctx->registers[k].bits;k++){
    if(!(strcasecmp(reg_name,ctx->registers[k].name))){
	return ctx->banks[pcilib_find_register_bank_by_addr(ctx,ctx->registers[k].bank)].name;
      }
  }
  return NULL;
}

/**
 * replace plain registers name in a formula by their value
 */
static char*
pcilib_view_compute_plain_registers(pcilib_t* ctx, char* formula){
  int j,k;
  char *substr, *substr2;
  char temp[66];
  pcilib_register_value_t value;
  
  /*we get recursively all registers of string , and if they are not equel to '@reg', then we get their value and put it in formula*/
  for(j=0;j<strlen((char*)formula);j++){
    if(formula[j]=='@'){
      k=j+1;
      while((formula[k]!=' ' && formula[k]!=')' && formula[k]!='/' && formula[k]!='+' && formula[k]!='-' && formula[k]!='*' && formula[k]!='=') && (k<strlen((char*)formula))){
	k++;
      }
      substr2=pcilib_view_str_sub((char*)formula,j,k-1); /**< we get the name of the register+@*/
      substr=pcilib_view_str_sub(substr2,1,k-j/*length of substr2*/); /**< we get the name of the register*/
     
      if((strcasecmp(substr,"reg"))){
	  /* we get the bank name associated to the register, and read its value*/
	  pcilib_read_register(ctx, pcilib_view_get_bank_from_reg_name(ctx, substr),substr,&value);
	  /* we put the value in formula*/
	  sprintf(temp,"%i",value);
	  formula = pcilib_view_formula_replace(formula,substr2,temp);
      }

    }
  }
  return formula;
}

/**
 * this function calls the python script and the function "evaluate" in it to evaluate the given formula
 *@param[in] the formula to be evaluated
 *@return the integer value of the evaluated formula (maybe go to float instead)
 */
static pcilib_register_value_t
pcilib_view_eval_formula(char* formula){
   setenv("PYTHONPATH",".",1);
   PyObject *pName, *pModule, *pDict, *pFunc, *pValue, *presult=NULL;
   char* pythonfile;

   /* path to the python file, we may need a way to set it, maybe with a variable like PCILIB_PYTHON_PATH*/
   pythonfile="pythonscripts.py";
   
   /* initialization of python interpreter*/
   Py_Initialize();

   pName = PyUnicode_FromString(pythonfile);
   pModule = PyImport_Import(pName); /* get the python file*/
	if(!pModule) {
		pcilib_error("no python file found for python evaluation of formulas\n");
		PyErr_Print();
		return -1;
	}
   pDict = PyModule_GetDict(pModule); /*useless but needed*/

   pFunc = PyDict_GetItemString(pDict, (char*)"evaluate"); /*getting of the function "evaluate" in the script*/
   /* if the function is ok, then call it*/
   if (PyCallable_Check(pFunc))
   {
        /* execution of the function*/
       pValue=Py_BuildValue("(z)",formula);
       PyErr_Print();
       presult=PyObject_CallObject(pFunc,pValue);
       PyErr_Print();
   } else 
   {
       PyErr_Print();
   }
   /* remove memory*/
   Py_DECREF(pModule);
   Py_DECREF(pName);
   
   /* close interpreter*/
   Py_Finalize();
   
   return (pcilib_register_value_t)PyLong_AsUnsignedLong(presult); /*this function is due to python 3.3, as PyLong_AsInt was there in python 2.7 and is no more there, resulting in using cast futhermore*/
}


static void
pcilib_view_apply_formula(pcilib_t* ctx, char* formula, pcilib_register_value_t reg_value, pcilib_register_value_t* out_value)
{
  /* when applying a formula, we need to:
     1) compute the values of all registers present in plain name in the formulas and replace their name with their value : for example, if we have the formula" ((1./4)*(@reg - 1200)) if @freq==0 else ((3./10)*(@reg - 1000)) " we need to get the value of the register "freq"
     2) compute the "@reg" component, corresponding to the raw value of the register
     3) pass the formula to python interpreter for evaluation
     4) return the final evaluation
     
     remark: it might be worth to use python regular expression interpreter (the following return all words following @ in formula : 
         >>> import re
	 >>> m = re.search('(?<=@)\w+', formula)
	 >>> m.group(0)
  */
  
  char reg_value_string[66]; /* to register reg_value as a string, need to check the length*/
  sprintf(reg_value_string,"%u",reg_value);
  
  /*computation of plain registers in the formula*/
  formula=pcilib_view_compute_plain_registers(ctx,formula);
  /* computation of @reg with register value*/
  formula=pcilib_view_formula_replace(formula,"@reg",reg_value_string);
  
  /* evaluation of the formula*/
  *out_value= pcilib_view_eval_formula(formula);

}

int pcilib_read_view(pcilib_t *ctx, const char *bank, const char *regname, const char *view/*, const char *unit*/, size_t value_size, void *value)
{
  int i,j,err=0;
  pcilib_register_value_t temp_value;
  char* formula;
  
  /* we get the index of the register to find the corresponding register context*/
  if((i=pcilib_find_register(ctx,bank,regname))==PCILIB_REGISTER_INVALID){
    pcilib_error("can't get the index of the register %s", regname);
    return PCILIB_ERROR_INVALID_REQUEST;
  }
  
  /* we get the value of the register, as we will apply the view on it*/
  if((err==pcilib_read_register_by_id(ctx,i,&temp_value))>0){
    pcilib_error("can't read the register %s value before applying views",regname);
  }
    /*in the case we don't ask for a view's name, we know it will be for views of type enum. Especially, it's faster to search directly on the values of those views instead of a given name.
      we iterate so through the views of type enum to verify if the value we have corresponds to an enum command*/
  if(!(view)){
    for(j=0; ctx->register_ctx[i].enums[j].value;j++){
      if((temp_value >= ctx->register_ctx[i].enums[j].min) && (temp_value <= ctx->register_ctx[i].enums[j].max)){
	value_size=strlen(ctx->register_ctx[i].enums[j].name)*sizeof(char);
	value=malloc(sizeof(value_size));
	if(!(value)){
	  pcilib_error("can't allocate memory for the returning value of the view %s",view);
	  return PCILIB_ERROR_MEMORY;
	}
	/* in the case the value of register is between min and max, then we return the correponding enum command*/
	strncpy((char*)value,ctx->register_ctx[i].enums[j].name, strlen(ctx->register_ctx[i].enums[j].name));
	return 0;
      }
    }
    pcilib_warning("the value of the register asked do not correspond to any enum views");
    return PCILIB_ERROR_NOTAVAILABLE;
  }
  
  /** in the other case we ask for a view of type formula. Indeed, wa can't directly ask for a formula, so we have to provide a name for those views*/
  j=0;
  while((ctx->register_ctx[i].formulas[j].name)){
    if(!(strcasecmp(ctx->register_ctx[i].formulas[j].name,view))){
      /* when we have found the correct view of type formula, we apply the formula, that get the good value for return*/
      formula=malloc(sizeof(char)*strlen(ctx->register_ctx[i].formulas[j].read_formula));
      strncpy(formula,ctx->register_ctx[i].formulas[j].read_formula,strlen(ctx->register_ctx[i].formulas[j].read_formula));
      //      pcilib_view_apply_formula(ctx, ctx->register_ctx[i].formulas[j].read_formula,temp_value,value);
      pcilib_view_apply_formula(ctx, formula,temp_value,value);
      value_size=sizeof(int);
      return 0;
    }
    j++;
   }

  pcilib_warning("the view asked and the register do not correspond");
  return PCILIB_ERROR_NOTAVAILABLE;
}


/**
 * function to write to a register using a view
 */ 
int pcilib_write_view(pcilib_t *ctx, const char *bank, const char *regname, const char *view, size_t value_size,void* value/*, const char *unit*/){
   int i,j;
   pcilib_register_value_t temp_value;
   char *formula;

  /* we get the index of the register to find the corresponding register context*/
  if((i=pcilib_find_register(ctx,bank,regname))==PCILIB_REGISTER_INVALID){
    pcilib_error("can't get the index of the register %s", regname);
    return PCILIB_ERROR_INVALID_REQUEST;
  }
  
    /*here, in the case of views of type enum, view will correspond to the enum command.
      we iterate so through the views of type enum to get the value corresponding to the enum command*/
    for(j=0; ctx->register_ctx[i].enums[j].value;j++){
      if(!(strcasecmp(ctx->register_ctx[i].enums[j].name,view))){
	pcilib_write_register(ctx,bank,regname,ctx->register_ctx[i].enums[j].value);
	return 0;
      }
    }
  
  /** in the other case we ask for a view of type formula. Indeed, wa can't directly ask for a formula, so we have to provide a name for those views in view, and the value we want to write in value*/
  j=0;
  while((ctx->register_ctx[i].formulas[j].name)){
    if(!(strcasecmp(ctx->register_ctx[i].formulas[j].name,view))){
      /* when we have found the correct view of type formula, we apply the formula, that get the good value for return*/
      formula=malloc(sizeof(char)*strlen(ctx->register_ctx[i].formulas[j].write_formula));
      strncpy(formula,ctx->register_ctx[i].formulas[j].write_formula,strlen(ctx->register_ctx[i].formulas[j].write_formula));
      //    pcilib_view_apply_formula(ctx, ctx->register_ctx[i].formulas[j].write_formula,(pcilib_register_value_t*)value,temp_value);
      pcilib_view_apply_formula(ctx,formula,*(pcilib_register_value_t*)value,&temp_value);
      pcilib_write_register(ctx,bank,regname,temp_value);
      return 0;
    }
    j++;
  }
  pcilib_warning("the view asked and the register do not correspond");
  return PCILIB_ERROR_NOTAVAILABLE;
 }

/**
 * function to populate ctx enum views, as we could do for registers or banks
 */
int pcilib_add_views_enum(pcilib_t *ctx, size_t n, const pcilib_view_enum2_t* views) {
	
    pcilib_view_enum2_t *views_enum;
    size_t size;

    if (!n) {
	for (n = 0; views[n].enums_list[0].value; n++);
    }

    if ((ctx->num_enum_views + n + 1) > ctx->alloc_enum_views) {
	for (size = ctx->alloc_enum_views; size < 2 * (n + ctx->num_enum_views + 1); size<<=1);

	views_enum = (pcilib_view_enum2_t*)realloc(ctx->enum_views, size * sizeof(pcilib_view_enum2_t));
	if (!views_enum) return PCILIB_ERROR_MEMORY;

	ctx->enum_views = views_enum;
	/* context + model part ????*/
	ctx->alloc_enum_views = size;
    }

    memcpy(ctx->enum_views + ctx->num_enum_views, views, n * sizeof(pcilib_view_enum2_t));
    memset(ctx->enum_views + ctx->num_enum_views + n, 0, sizeof(pcilib_view_enum2_t));

    ctx->num_enum_views += n;
    

    return 0;
}


/**
 * function to populate ctx formula views, as we could do for registers or banks
 */
int pcilib_add_views_formula(pcilib_t *ctx, size_t n, const pcilib_view_formula_t* views) {
	
    pcilib_view_formula_t *views_formula;
    size_t size;

    if (!n) {
	for (n = 0; views[n].name[0]; n++);
    }

    if ((ctx->num_formula_views + n + 1) > ctx->alloc_formula_views) {
	for (size = ctx->alloc_formula_views; size < 2 * (n + ctx->num_formula_views + 1); size<<=1);

	views_formula = (pcilib_view_formula_t*)realloc(ctx->formula_views, size * sizeof(pcilib_view_formula_t));
	if (!views_formula) return PCILIB_ERROR_MEMORY;

	ctx->formula_views = views_formula;
	/* context + model part?????*/
	ctx->alloc_formula_views = size;
    }

    memcpy(ctx->formula_views + ctx->num_formula_views, views, n * sizeof(pcilib_view_formula_t));
    memset(ctx->formula_views + ctx->num_formula_views + n, 0, sizeof(pcilib_view_formula_t));

    ctx->num_formula_views += n;
    

    return 0;
}
