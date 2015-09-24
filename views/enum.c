#define _PCILIB_VIEW_ENUM_C

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "version.h"
#include "model.h"
#include "enum.h"

static void pcilib_enum_view_free(pcilib_t *ctx, pcilib_view_context_t *view) {
}

static int pcilib_enum_view_read(pcilib_t *ctx, pcilib_view_context_t *view, const pcilib_register_value_t *regval, pcilib_data_type_t viewval_type, size_t viewval_size, void *viewval) {
/*        for(j=0; ((pcilib_enum_t*)(params))[j].name; j++) {
            if (*regval<=((pcilib_enum_t*)(params))[j].max && *regval>=((pcilib_enum_t*)(params))[j].min) {
                if(viewval_size<strlen(((pcilib_enum_t*)(params))[j].name)) {
                    pcilib_error("the string to contain the enum command is too tight");
                    return PCILIB_ERROR_MEMORY;
                }
                strncpy((char*)viewval,((pcilib_enum_t*)(params))[j].name, strlen(((pcilib_enum_t*)(params))[j].name));
                k=strlen(((pcilib_enum_t*)(params))[j].name);
                ((char*)viewval)[k]='\0';
                return 0;
            }
        }
    }
    return PCILIB_ERROR_INVALID_REQUEST;*/
}

static int pcilib_enum_view_write(pcilib_t *ctx, pcilib_view_context_t *view, pcilib_register_value_t *regval, pcilib_data_type_t viewval_type, size_t viewval_size, const void *viewval) {
/*    int j,k;

    if(view2reg==1) {
        for(j=0; ((pcilib_enum_t*)(params))[j].name; j++) {
            if(!(strcasecmp(((pcilib_enum_t*)(params))[j].name,(char*)viewval))) {
                *regval=((pcilib_enum_t*)(params))[j].value;
                return 0;
            }
        }*/
}

const pcilib_view_api_description_t pcilib_enum_view_static_api =
  { PCILIB_VERSION, sizeof(pcilib_enum_view_description_t), NULL, NULL,  pcilib_enum_view_read,  pcilib_enum_view_write };
const pcilib_view_api_description_t pcilib_enum_view_xml_api =
  { PCILIB_VERSION, sizeof(pcilib_enum_view_description_t), NULL, pcilib_enum_view_free,  pcilib_enum_view_read,  pcilib_enum_view_write };
