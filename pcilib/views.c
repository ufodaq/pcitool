#include <string.h>
#include "pci.h"
#include "pcilib.h"
#include <Python.h>
#include "views.h"
#include "error.h"
#include <strings.h>
#include <stdlib.h>
#include "unit.h" 

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
char*
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
pcilib_view_compute_plain_registers(pcilib_t* ctx, char* formula, int direction){
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
      if(direction==0){
	if((strcasecmp(substr,"reg"))){
	  /* we get the bank name associated to the register, and read its value*/
	  pcilib_read_register(ctx, pcilib_view_get_bank_from_reg_name(ctx, substr),substr,&value);
	  /* we put the value in formula*/
	  sprintf(temp,"%i",value);
	  formula = pcilib_view_formula_replace(formula,substr2,temp);
	}
      }
      else if(direction==1){
	if((strcasecmp(substr,"value"))){
	  /* we get the bank name associated to the register, and read its value*/
	  pcilib_read_register(ctx, pcilib_view_get_bank_from_reg_name(ctx, substr),substr,&value);
	  /* we put the value in formula*/
	  sprintf(temp,"%i",value);
	  formula = pcilib_view_formula_replace(formula,substr2,temp);
	}
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
  
   /* initialization of python interpreter*/
   Py_Initialize();
   
   /*compilation of the formula as python string*/
   PyCodeObject* code=(PyCodeObject*)Py_CompileString(formula,"test",Py_eval_input);
   PyObject* main_module = PyImport_AddModule("__parser__");
   PyObject* global_dict = PyModule_GetDict(main_module);
   PyObject* local_dict = PyDict_New();
   /*evaluation of formula*/
   PyObject* obj = PyEval_EvalCode(code, global_dict, local_dict);
   double c=PyFloat_AsDouble(obj);

   /* close interpreter*/
   Py_Finalize();
   pcilib_register_value_t value=(pcilib_register_value_t)c;
   return value;
}

/**
 * function to apply a unit for the views of type formula
 *@param[in] view - the view we want to get the units supported
  *@param[in] unit - the requested unit in which we want to get the value
 *@param[in,out] value - the number that needs to get transformed
 */
static void
pcilib_view_apply_unit(pcilib_transform_unit_t unit_desc, const char* unit,pcilib_register_value_t* value){
  char* formula;
  char temp[66];
  
	    formula=malloc(strlen(unit_desc.transform_formula)*sizeof(char));
	    strcpy(formula,unit_desc.transform_formula);
	    sprintf(temp,"%i",*value);
	    formula=pcilib_view_formula_replace(formula,"@self",temp); 
	    *value=(int)pcilib_view_eval_formula(formula);
}


static void
pcilib_view_apply_formula(pcilib_t* ctx, char* formula, pcilib_register_value_t reg_value, pcilib_register_value_t* out_value, int direction)
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
  formula=pcilib_view_compute_plain_registers(ctx,formula,direction);
  /* computation of @reg with register value*/
  if(direction==0) formula=pcilib_view_formula_replace(formula,"@reg",reg_value_string);
  else if (direction==1) formula=pcilib_view_formula_replace(formula,"@value",reg_value_string);
  /* evaluation of the formula*/
  *out_value= pcilib_view_eval_formula(formula);
}

int pcilib_read_view(pcilib_t *ctx, const char *bank, const char *regname, const char *unit, size_t value_size, void *value)
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
  err=pcilib_read_register_by_id(ctx,i,&temp_value);
  if(err){
    pcilib_error("can't read the register %s value before applying views : error %i",regname);
    return PCILIB_ERROR_INVALID_REQUEST;
  }
    /*in the case we don't ask for a view's name, we know it will be for views of type enum. Especially, it's faster to search directly on the values of those views instead of a given name.
      we iterate so through the views of type enum to verify if the value we have corresponds to an enum command*/
  if(!(unit)){
    for(j=0; ctx->register_ctx[i].enums[j].value;j++){
      if((temp_value >= ctx->register_ctx[i].enums[j].min) && (temp_value <= ctx->register_ctx[i].enums[j].max)){
	value_size=strlen(ctx->register_ctx[i].enums[j].name)*sizeof(char);
	value=(char*)realloc(value,sizeof(value_size));
	if(!(value)){
	  pcilib_error("can't allocate memory for the returning value of the view");
	  return PCILIB_ERROR_MEMORY;
	}
	/* in the case the value of register is between min and max, then we return the correponding enum command*/
	strncpy((char*)value,ctx->register_ctx[i].enums[j].name, strlen(ctx->register_ctx[i].enums[j].name));
	/* make sure the string is correctly terminated*/
	((char*)value)[value_size]='\0';
	return 0;
      }
    }
    pcilib_warning("the value of the register asked do not correspond to any enum views");
    return PCILIB_ERROR_NOTAVAILABLE;
  }
  
  /** in the other case we ask for a view of type formula. Indeed, wa can't directly ask for a formula, so we have to provide a name for those views*/
  j=0;
  if(!(strcasecmp(unit, ctx->register_ctx[i].formulas[0].base_unit.name))){
      formula=malloc(sizeof(char)*strlen(ctx->register_ctx[i].formulas[0].read_formula));
      if(!(formula)){
	pcilib_error("can't allocate memory for the formula");
	return PCILIB_ERROR_MEMORY;
      }
      strncpy(formula,ctx->register_ctx[i].formulas[0].read_formula,strlen(ctx->register_ctx[i].formulas[0].read_formula));
      pcilib_view_apply_formula(ctx,formula,temp_value,value,0);
      value_size=sizeof(int);
      return 0;
    }

  for(j=0; ctx->register_ctx[i].formulas[0].base_unit.other_units[j].name;j++){
    if(!(strcasecmp(ctx->register_ctx[i].formulas[0].base_unit.other_units[j].name,unit))){
      /* when we have found the correct view of type formula, we apply the formula, that get the good value for return*/
      formula=malloc(sizeof(char)*strlen(ctx->register_ctx[i].formulas[0].read_formula));
      if(!(formula)){
	pcilib_error("can't allocate memory for the formula");
	return PCILIB_ERROR_MEMORY;
      }
      strncpy(formula,ctx->register_ctx[i].formulas[0].read_formula,strlen(ctx->register_ctx[i].formulas[0].read_formula));
      pcilib_view_apply_formula(ctx,formula,temp_value,value,0);
      pcilib_view_apply_unit(ctx->register_ctx[i].formulas[0].base_unit.other_units[j],unit,value);
      value_size=sizeof(int);
      return 0;
    }
  }

  pcilib_warning("the view asked and the register do not correspond");
  return PCILIB_ERROR_NOTAVAILABLE;
}


/**
 * function to write to a register using a view
 */ 
int pcilib_write_view(pcilib_t *ctx, const char *bank, const char *regname, const char *unit, size_t value_size,void* value){
   int i,j;
   pcilib_register_value_t temp_value;
   char *formula;
   int err;
   
  /* we get the index of the register to find the corresponding register context*/
  if((i=pcilib_find_register(ctx,bank,regname))==PCILIB_REGISTER_INVALID){
    pcilib_error("can't get the index of the register %s", regname);
    return PCILIB_ERROR_INVALID_REQUEST;
  }
 
  for(j=0;ctx>register_ctx[i].views[j].name;j++){
    if(!(strcasecmp(ctx->register_ctx[i].views[j].base_unit,unit))){
      err=ctx->register_ctx[i].views[j].op(ctx,ctx->register_ctx[i].views[j].parameters,unit,1,&temp_value,value);
      temp_value=ctx->register_ctx[i].views[value].value;
      if(err){
	pcilib_error("can't write to the register with the enum view");
	return PCILIB_ERROR_FAILED;
      }
      break;
    }else if(!(strcasecmp(ctx->register_ctx[i].views[j].name,unit))){
      err=ctx->register_ctx[i].views[j].op(ctx,ctx->register_ctx[i].views[j].parameters, unit, 1, &temp_value,0,&(ctx->register_ctx[i].views[j]));
      if(err){
	pcilib_error("can't write to the register with the formula view %s", unit);
	return PCILIB_ERROR_FAILED;
      }
      break;
    }
    pcilib_write_register(ctx,bank,regname,temp_value);
    return 0;
  }


    /*here, in the case of views of type enum, view will correspond to the enum command.
      we iterate so through the views of type enum to get the value corresponding to the enum command*
    for(j=0; ctx->register_ctx[i].enums[j].name;j++){
      /* we should maybe have another to do it there*
      if(!(strcasecmp(ctx->register_ctx[i].enums[j].name,unit))){
	pcilib_write_register(ctx,bank,regname,ctx->register_ctx[i].enums[j].value);
	return 0;
      }
    }
    
  /** in the other case we ask for a view of type formula. Indeed, wa can't directly ask for a formula, so we have to provide a name for those views in view, and the value we want to write in value*/
      /* j=0;
  if(!(strcasecmp(unit, ctx->register_ctx[i].formulas[0].base_unit.name))){
      formula=malloc(sizeof(char)*strlen(ctx->register_ctx[i].formulas[0].write_formula));
      strncpy(formula,ctx->register_ctx[i].formulas[0].write_formula,strlen(ctx->register_ctx[i].formulas[0].write_formula));
      pcilib_view_apply_formula(ctx,formula,*(pcilib_register_value_t*)value,&temp_value,1);
      pcilib_write_register(ctx,bank,regname,temp_value);
      return 0;
    }

  for(j=0; ctx->register_ctx[i].formulas[0].base_unit.other_units[j].name;j++){
    if(!(strcasecmp(ctx->register_ctx[i].formulas[0].base_unit.other_units[j].name,unit))){
      /* when we have found the correct view of type formula, we apply the formula, that get the good value for return*
      formula=malloc(sizeof(char)*strlen(ctx->register_ctx[i].formulas[0].write_formula));
      strncpy(formula,ctx->register_ctx[i].formulas[0].write_formula,strlen(ctx->register_ctx[i].formulas[0].write_formula));
      pcilib_view_apply_unit(ctx->register_ctx[i].formulas[0].base_unit.other_units[j],unit,value);
      pcilib_view_apply_formula(ctx,formula,*(pcilib_register_value_t*)value,&temp_value,1);
      /* we maybe need some error checking there , like temp_value >min and <max*
      pcilib_write_register(ctx,bank,regname,temp_value);
      return 0;
    }
  }*/
  pcilib_error("the view asked and the register do not correspond");
  return PCILIB_ERROR_NOTAVAILABLE;
}

/** viewval=enum command, params=current parameters of view, regval=enum value*/
int operation_enum(pcilib_t *ctx, void *params/*name*/, char* name, int read_or_write, pcilib_register_value_t *regval, size_t viewval_size, void* viewval){
  int j,k;
  if(read_or_write==1){
    for(j=0; ((pcilib_view_enum_t*)(params))[j].name;j++){
      if(!(strcasecmp(((pcilib_view_enum_t*)(params))[j].name,(char*)viewval))){
	return j;
      }
    }
  }else if (read_or_write==0){
    for(j=0; ((pcilib_view_enum_t*)(params))[j].name;j++){
      if (*regval<((pcilib_view_enum_t*)(params))[j].max && *regval>((pcilib_view_enum_t*)(params))[j].min){
	viewval=(char*)realloc(viewval,strlen(((pcilib_view_enum_t*)(params))[j].name)*sizeof(char));
	strncpy((char*)viewval,((pcilib_view_enum_t*)(params))[j].name, strlen(((pcilib_view_enum_t*)(params))[j].name));
	k=strlen(((pcilib_view_enum_t*)(params))[j].name);
	((char*)regval)[k]='\0';
	return 0;
      }
    }
  } 
  return -1;
}

/** viewsval=the current view, params=current view parameters*/
int operation_formula(pcilib_t *ctx, void *params/*name*/, char* unit, int read_or_write, pcilib_register_value_t *regval, size_t viewval_size, void* viewval){
    int j=0;
    char* formula;
    pcilib_register_value_t value=0;

    if(read_or_write==0){
      if(!(strcasecmp(unit, ((pcilib_view_t*)viewval)->base_unit.name))){
	formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->read_formula));
	if(!(formula)){
	  pcilib_error("can't allocate memory for the formula");
	  return PCILIB_ERROR_MEMORY;
	}
	strncpy(formula,((pcilib_formula_t*)params)->read_formula,strlen(((pcilib_formula_t*)params)->read_formula));
	pcilib_view_apply_formula(ctx,formula,*regval,&value,0);
	return value;
      }
      
      for(j=0; ((pcilib_view_t*)viewval)->base_unit.other_units[j].name;j++){
	if(!(strcasecmp(((pcilib_view_t*)viewval)->base_unit.other_units[j].name,unit))){
	  /* when we have found the correct view of type formula, we apply the formula, that get the good value for return*/
	  formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->read_formula));
	  if(!(formula)){
	    pcilib_error("can't allocate memory for the formula");
	    return PCILIB_ERROR_MEMORY;
	  }
	  strncpy(formula,((pcilib_formula_t*)params)->read_formula,strlen(((pcilib_formula_t*)params)->read_formula));
	  pcilib_view_apply_formula(ctx,formula, *regval,&value,0);
	  pcilib_view_apply_unit(((pcilib_view_t*)viewval)->base_unit.other_units[j],unit,&value);
	  return value;
	}
      }
    }else if(read_or_write==1){
      j=0;
      if(!(strcasecmp(unit, ((pcilib_view_t*)viewval)->base_unit.name))){
	formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->write_formula));
	strncpy(formula,((pcilib_formula_t*)params)->write_formula,strlen(((pcilib_formula_t*)params)->write_formula));
	pcilib_view_apply_formula(ctx,formula,*regval,&value,1);
	return 0;
    }
      
      for(j=0;((pcilib_view_t*)viewval)->base_unit.other_units[j].name;j++){
	if(!(strcasecmp(((pcilib_view_t*)viewval)->base_unit.other_units[j].name,unit))){
	  /* when we have found the correct view of type formula, we apply the formula, that get the good value for return*/
	  formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->write_formula));
	  strncpy(formula,((pcilib_formula_t*)params)->write_formula,strlen((( pcilib_formula_t*)params)->write_formula));
	  pcilib_view_apply_unit(((pcilib_view_t*)viewval)->base_unit.other_units[j],unit,&value);
	  pcilib_view_apply_formula(ctx,formula,*regval,&value,1);
	  /* we maybe need some error checking there , like temp_value >min and <max*/
	  return 0;
	}
      }
    }
      return -1;
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
	for (n = 0; views[n].name; n++);
    }

    if ((ctx->num_formula_views + n + 1) > ctx->alloc_formula_views) {
      for (size = ctx->alloc_formula_views; size < 2 * (n + ctx->num_formula_views + 1); size<<=1);

	views_formula = (pcilib_view_formula_t*)realloc(ctx->formula_views, size * sizeof(pcilib_view_formula_t));
	if (!views_formula) return PCILIB_ERROR_MEMORY;

	ctx->formula_views = views_formula;
	ctx->alloc_formula_views = size;
    }

    memcpy(ctx->formula_views + ctx->num_formula_views, views, n * sizeof(pcilib_view_formula_t));
    memset(ctx->formula_views + ctx->num_formula_views + n, 0, sizeof(pcilib_view_formula_t));

    ctx->num_formula_views += n;
    return 0;
}

/**
 * function to populate ctx views, as we could do for registers or banks
 */
int pcilib_add_views(pcilib_t *ctx, size_t n, const pcilib_view_t* views) {
	
    pcilib_view_t *views2;
    size_t size;

    if (!n) {
	for (n = 0; views[n].name; n++);
    }

    if ((ctx->num_views + n + 1) > ctx->alloc_views) {
      for (size = ctx->alloc_views; size < 2 * (n + ctx->num_views + 1); size<<=1);

	views2 = (pcilib_view_t*)realloc(ctx->views, size * sizeof(pcilib_view_t));
	if (!views2) return PCILIB_ERROR_MEMORY;

	ctx->views = views2;
	ctx->alloc_views = size;
    }

    memcpy(ctx->views + ctx->num_views, views, n * sizeof(pcilib_view_t));
    memset(ctx->views + ctx->num_views + n, 0, sizeof(pcilib_view_t));

    ctx->num_views += n;
    return 0;
}
