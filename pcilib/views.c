#include <string.h>
#include "pci.h"
#include "pcilib.h"
#include <Python.h>
#include "views.h"
#include "error.h"
#include <strings.h>
#include <stdlib.h>

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
 * 
 */
static char*
pcilib_view_compute_formula(pcilib_t* ctx, char* formula,char* reg_value_string){
  char *src=(char*)formula; 
  char *reg,*regend;
  char *dst=malloc(6*strlen(src)*sizeof(char));
  char temp[66];
  pcilib_register_value_t value;
  int offset=0;
  
  /*we get recursively all registers of string , and if they are not equel to '@reg', then we get their value and put it in formula*/
  while(1){
    reg = strchr(src, '@');
    if (!reg) {
      strcpy(dst+offset, src);
      break;
    }
    regend = strchr(reg + 1, '@');
    if (!regend){
      pcilib_error("formula corresponding is malformed");
      return NULL;
    }
    strncpy(dst+offset, src, reg - src);
    offset+=reg-src;
    *regend = 0;
    /* Now (reg + 1) contains the proper register name, you can compare
it to reg/value and either get the value of current register or the
specified one. Add it to the register*/
    if(!(strcasecmp(reg+1,"value")) || !(strcasecmp(reg+1,"reg")) || !(strcasecmp(reg+1,"self"))){
      strncpy(dst+offset,reg_value_string,strlen(reg_value_string));
      offset+=strlen(reg_value_string);
    }else{
      pcilib_read_register(ctx, NULL,reg+1,&value);
      sprintf(temp,"%i",value);
      strncpy(dst+offset,temp,strlen(temp));
      offset+=strlen(temp);
    }      
  src = regend + 1;
  }
  return dst;
}




static int
pcilib_view_apply_formula(pcilib_t* ctx, char* formula, pcilib_register_value_t* reg_value)
{
  
  char reg_value_string[66]; /* to register reg_value as a string, need to check the length*/
  sprintf(reg_value_string,"%u",*reg_value);
  formula=pcilib_view_compute_formula(ctx,formula,reg_value_string);
  
  if(!(formula)){
    pcilib_error("computing of formula failed");
    return PCILIB_ERROR_INVALID_DATA;
  }

  /* evaluation of the formula*/
  *reg_value= pcilib_view_eval_formula(formula);
  return 0;
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
 
	    formula=malloc(strlen(unit_desc.transform_formula)*sizeof(char));
	    strcpy(formula,unit_desc.transform_formula);
	    pcilib_view_apply_formula(NULL,formula, value);

	    free(formula);
}



int pcilib_read_view(pcilib_t *ctx, const char *bank, const char *regname, const char *unit, size_t value_size, void *value)
{
  int i,j,k,err=0;
  pcilib_register_value_t temp_value;
  
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

  
  for(j=0;ctx->register_ctx[i].views[j].name;j++){
      if(!(strcasecmp("name",ctx->register_ctx[i].views[j].base_unit.name))){/*if we asked for the unit "name"*/
	err=ctx->register_ctx[i].views[j].op(ctx,ctx->register_ctx[i].views[j].parameters,value/*the command name*/,0,&temp_value,value_size,NULL);
	if(err){
	  pcilib_error("can't read from the register with the enum view");
	  return PCILIB_ERROR_FAILED;
	}
	return 0;
      }else if(!(strcasecmp(ctx->register_ctx[i].views[j].base_unit.name,(char*)unit))){/*in this case we asked for the name of the view in unit*/
	err=ctx->register_ctx[i].views[j].op(ctx,ctx->register_ctx[i].views[j].parameters,(char*)unit, 0, &temp_value,0,&(ctx->register_ctx[i].views[j]));
	if(err){
	  pcilib_error("can't read from the register with the formula view %s", unit);
	  return PCILIB_ERROR_FAILED;
	}
	*(pcilib_register_value_t*)value=temp_value;
	return 0;
      }else{
	for(k=0;ctx->register_ctx[i].views[j].base_unit.transforms[k].name;k++){
	  if(!(strcasecmp(ctx->register_ctx[i].views[j].base_unit.transforms[k].name,(char*)unit))){
	    err=ctx->register_ctx[i].views[j].op(ctx,ctx->register_ctx[i].views[j].parameters,(char*)unit, 0, &temp_value,0,&(ctx->register_ctx[i].views[j]));
	    if(err){
	      pcilib_error("can't write to the register with the formula view %s", unit);
	      return PCILIB_ERROR_FAILED;
	    }
	    *(pcilib_register_value_t*)value=temp_value;
	    return 0;
	  }
	}  
      }
  }

     pcilib_error("the view asked and the register do not correspond");
  return PCILIB_ERROR_NOTAVAILABLE;
}


/**
 * function to write to a register using a view
 */ 
int pcilib_write_view(pcilib_t *ctx, const char *bank, const char *regname, const char *unit, size_t value_size,void* value){
  int i,j,k;
   pcilib_register_value_t temp_value;
   int err;
   int next=1,ok=0;
   
  /* we get the index of the register to find the corresponding register context*/
  if((i=pcilib_find_register(ctx,bank,regname))==PCILIB_REGISTER_INVALID){
    pcilib_error("can't get the index of the register %s", regname);
    return PCILIB_ERROR_INVALID_REQUEST;
  }

  for(j=0;ctx->register_ctx[i].views[j].name;j++){
    if(!(strcasecmp(ctx->register_ctx[i].views[j].base_unit.name,"name"))){/*if we asked for the unit "name"*/
      err=ctx->register_ctx[i].views[j].op(ctx,ctx->register_ctx[i].views[j].parameters,(char*)unit/*the command name*/,1,&temp_value,0,&(ctx->register_ctx[i].views[j]));
      if(err){
	pcilib_error("can't write to the register with the enum view");
	return PCILIB_ERROR_FAILED;
      }
      ok=1;
      break;
    }else if(!(strcasecmp(ctx->register_ctx[i].views[j].base_unit.name,(char*)unit))){/*in this case we asked for then name of the view in unit*/
      //temp_value=*(pcilib_register_value_t*)value /*the value to put in the register*/;
      err=ctx->register_ctx[i].views[j].op(ctx,ctx->register_ctx[i].views[j].parameters, (char*)unit, 1, &temp_value,0,&(ctx->register_ctx[i].views[j]));
      if(err){
	pcilib_error("can't write to the register with the formula view %s", unit);
	return PCILIB_ERROR_FAILED;
      }
      ok=1;
      break;
    }else{
      for(k=0;ctx->register_ctx[i].views[j].base_unit.transforms[k].name;k++){
	if(!(strcasecmp(ctx->register_ctx[i].views[j].base_unit.transforms[k].name,(char*)unit))){
	  err=ctx->register_ctx[i].views[j].op(ctx,ctx->register_ctx[i].views[j].parameters, (char*)unit, 1, &temp_value,0,&(ctx->register_ctx[i].views[j]));
	  if(err){
	    pcilib_error("can't write to the register with the formula view %s", unit);
	    return PCILIB_ERROR_FAILED;
	  }
	  next=0;
	  ok=1;
	  break;
	}
      }
      if(next==0)break;
    }  
  }
  
  if(ok==1) {
    pcilib_write_register(ctx,bank,regname,temp_value);
    printf("value %i written in register\n",temp_value);
    return 0;
  }
  
  pcilib_error("the view asked and the register do not correspond");
  return PCILIB_ERROR_NOTAVAILABLE;
}

/**
 * always : viewval=view params=view params
 * write: name=enum command regval:the value corresponding to the command
 */
int operation_enum(pcilib_t *ctx, void *params, char* name, int view2reg, pcilib_register_value_t *regval, size_t viewval_size, void* viewval){
  int j,k;
  if(view2reg==1){
    for(j=0; ((pcilib_enum_t*)(params))[j].name;j++){
      if(!(strcasecmp(((pcilib_enum_t*)(params))[j].name,name))){
	*regval=((pcilib_enum_t*)(params))[j].value;
	return 0;
      }
    }
  }else if (view2reg==0){
    for(j=0; ((pcilib_enum_t*)(params))[j].name;j++){
      if (*regval<=((pcilib_enum_t*)(params))[j].max && *regval>=((pcilib_enum_t*)(params))[j].min){
	if(viewval_size<strlen(((pcilib_enum_t*)(params))[j].name)){
	    pcilib_error("the string to contain the enum command is too tight");
	    return PCILIB_ERROR_MEMORY;
	  }
	strncpy(name,((pcilib_enum_t*)(params))[j].name, strlen(((pcilib_enum_t*)(params))[j].name));
	k=strlen(((pcilib_enum_t*)(params))[j].name);
	name[k]='\0';
	return 0;
      }
    }
  } 
  return PCILIB_ERROR_INVALID_REQUEST;
}

/**
 * pârams: view params unit=unit wanted regval:value before formula/after formula viewval=view
 */
int operation_formula(pcilib_t *ctx, void *params, char* unit, int view2reg, pcilib_register_value_t *regval, size_t viewval_size, void* viewval){
    int j=0;
    pcilib_register_value_t value=0;
    char* formula=NULL;

    if(view2reg==0){
      if(!(strcasecmp(unit,((pcilib_view_t*)viewval)->base_unit.name))){
	formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->read_formula));
	if(!(formula)){
	  pcilib_error("can't allocate memory for the formula");
	  return PCILIB_ERROR_MEMORY;
	}
	strncpy(formula,((pcilib_formula_t*)params)->read_formula,strlen(((pcilib_formula_t*)params)->read_formula));
	pcilib_view_apply_formula(ctx,formula,regval);
	return 0;
      }
      
      for(j=0; ((pcilib_view_t*)viewval)->base_unit.transforms[j].name;j++){
	if(!(strcasecmp(((pcilib_view_t*)viewval)->base_unit.transforms[j].name,unit))){
	  /* when we have found the correct view of type formula, we apply the formula, that get the good value for return*/
	  formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->read_formula));
	  if(!(formula)){
	    pcilib_error("can't allocate memory for the formula");
	    return PCILIB_ERROR_MEMORY;
	  }
	  strncpy(formula,((pcilib_formula_t*)params)->read_formula,strlen(((pcilib_formula_t*)params)->read_formula));
	  pcilib_view_apply_formula(ctx,formula, regval);
	  pcilib_view_apply_unit(((pcilib_view_t*)viewval)->base_unit.transforms[j],unit,regval);
	  return 0;
	}
      }
    }else if(view2reg==1){
      if(!(strcasecmp(unit, ((pcilib_view_t*)viewval)->base_unit.name))){
	formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->write_formula));
	strncpy(formula,((pcilib_formula_t*)params)->write_formula,strlen(((pcilib_formula_t*)params)->write_formula));
	pcilib_view_apply_formula(ctx,formula,regval);
	return 0;
    }
      
      for(j=0;((pcilib_view_t*)viewval)->base_unit.transforms[j].name;j++){
	if(!(strcasecmp(((pcilib_view_t*)viewval)->base_unit.transforms[j].name,unit))){
	  /* when we have found the correct view of type formula, we apply the formula, that get the good value for return*/
	  formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->write_formula));
	  strncpy(formula,((pcilib_formula_t*)params)->write_formula,strlen((( pcilib_formula_t*)params)->write_formula));
	  pcilib_view_apply_unit(((pcilib_view_t*)viewval)->base_unit.transforms[j],unit,&value);
	  pcilib_view_apply_formula(ctx,formula,regval);
	  /* we maybe need some error checking there , like temp_value >min and <max*/
	  return 0;
	}
      }
    }
    free(formula);
      return PCILIB_ERROR_INVALID_REQUEST;
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

int pcilib_add_units(pcilib_t *ctx, size_t n, const pcilib_unit_t* units) {
	
    pcilib_unit_t *units2;
    size_t size;

    if (!n) {
	for (n = 0; units[n].name[0]; n++);
    }

    if ((ctx->num_units + n + 1) > ctx->alloc_units) {
      for (size = ctx->alloc_units; size < 2 * (n + ctx->num_units + 1); size<<=1);

	units2 = (pcilib_unit_t*)realloc(ctx->units, size * sizeof(pcilib_unit_t));
	if (!units2) return PCILIB_ERROR_MEMORY;

	ctx->units = units2;
	ctx->alloc_units = size;
    }

    memcpy(ctx->units + ctx->num_units, units, n * sizeof(pcilib_unit_t));
    memset(ctx->units + ctx->num_units + n, 0, sizeof(pcilib_unit_t));

    ctx->num_units += n;
    
    return 0;
}
