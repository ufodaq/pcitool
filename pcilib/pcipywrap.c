#include "pcilib.h"

//Remove unused headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <alloca.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <dlfcn.h>

#include <getopt.h>

#include <fastwriter.h>

#include "pcitool/sysinfo.h"
#include "pcitool/formaters.h"

#include "views/transform.h"
#include "views/enum.h"
#include "pci.h"
#include "plugin.h"
#include "config.h"
#include "tools.h"
#include "kmem.h"
#include "error.h"
#include "debug.h"
#include "model.h"
#include "locking.h"

pcilib_t* __ctx = 0;
pcilib_model_description_t *model_info = 0;

/*!
 * \brief присваивание указателя на устройство. Закрытая функция. Будет проходить при парсинге xml.
 * \param ctx
 */
void __initCtx(void* ctx)
{
    __ctx = ctx;
}

/*!
 * \brief создание хэндлера устройства, для тестирования скрипта не из программы.
 * \return
 */
void __createCtxInstance(const char *fpga_device, const char *model)
{
    __ctx = pcilib_open(fpga_device, model);
    model_info = pcilib_get_model_description(__ctx);
}

int read_register(const char *bank, const char *regname, void *value)
{
    int ret = pcilib_read_register(__ctx, bank, regname, (pcilib_register_value_t*)value);
    return ret;
}

void Error(const char *message, const char *attr, ...)
{
		printf("Catch error: %s, %s\n", message, attr);
}

int ReadRegister(const char *bank, const char *reg) {
    
    const char *view = NULL;
    const char *unit = NULL;
    const char *attr = NULL;
    pcilib_t *handle = __ctx;
    int i;
    int err;
    const char *format;

    pcilib_register_bank_t bank_id;
    pcilib_register_bank_addr_t bank_addr = 0;

    pcilib_register_value_t value;

	// Adding DMA registers
    pcilib_get_dma_description(handle);

    if (reg||view||attr) {
        pcilib_value_t val = {0};
        if (attr) {
            if (reg) err = pcilib_get_register_attr(handle, bank, reg, attr, &val);
            else if (view) err = pcilib_get_property_attr(handle, view, attr, &val);
            else if (bank) err = pcilib_get_register_bank_attr(handle, bank, attr, &val);
            else err = PCILIB_ERROR_INVALID_ARGUMENT;

            if (err) {
                if (err == PCILIB_ERROR_NOTFOUND)
                    Error("Attribute %s is not found", attr);
                else
                    Error("Error (%i) reading attribute %s", err, attr);
            }

            err = pcilib_convert_value_type(handle, &val, PCILIB_TYPE_STRING);
            if (err) Error("Error converting attribute %s to string", attr);

            printf("%s = %s", attr, val.sval);
            if ((val.unit)&&(strcasecmp(val.unit, "name")))
                printf(" %s", val.unit);
            printf(" (for %s)\n", (reg?reg:(view?view:bank)));
        } else if (view) {
            if (reg) {
                err = pcilib_read_register_view(handle, bank, reg, view, &val);
                if (err) Error("Error reading view %s of register %s", view, reg);
            } else {
                err = pcilib_get_property(handle, view, &val);
                if (err) Error("Error reading property %s", view);
            }

            if (unit) {
                err = pcilib_convert_value_unit(handle, &val, unit);
                if (err) {
                    if (reg) Error("Error converting view %s of register %s to unit %s", view, reg, unit);
                    else Error("Error converting property %s to unit %s", view, unit);
                }
            }
            
            err = pcilib_convert_value_type(handle, &val, PCILIB_TYPE_STRING);
            if (err) {
                if (reg) Error("Error converting view %s of register %s to string", view);
                else Error("Error converting property %s to string", view);
            }

            printf("%s = %s", (reg?reg:view), val.sval);
            if ((val.unit)&&(strcasecmp(val.unit, "name")))
                printf(" %s", val.unit);
            printf("\n");
        } else {
            pcilib_register_t regid = pcilib_find_register(handle, bank, reg);
            bank_id = pcilib_find_register_bank_by_addr(handle, model_info->registers[regid].bank);
            format = model_info->banks[bank_id].format;
            if (!format) format = "%lu";
            err = pcilib_read_register_by_id(handle, regid, &value);
            if (err) Error("Error reading register %s", reg);

	    printf("%s = ", reg);
	    printf(format, value);
	    printf("\n");
	}
    } else {
	if (model_info->registers) {
	    if (bank) {
		bank_id = pcilib_find_register_bank(handle, bank);
		bank_addr = model_info->banks[bank_id].addr;
	    }
	    
	    printf("Registers:\n");
	    for (i = 0; model_info->registers[i].bits; i++) {
		if ((model_info->registers[i].mode & PCILIB_REGISTER_R)&&((!bank)||(model_info->registers[i].bank == bank_addr))&&(model_info->registers[i].type != PCILIB_REGISTER_BITS)) { 
		    bank_id = pcilib_find_register_bank_by_addr(handle, model_info->registers[i].bank);
		    format = model_info->banks[bank_id].format;
		    if (!format) format = "%lu";

		    err = pcilib_read_register_by_id(handle, i, &value);
		    if (err) printf(" %s = error reading value", model_info->registers[i].name);
	    	    else {
			printf(" %s = ", model_info->registers[i].name);
			printf(format, value);
		    }

		    printf(" [");
		    printf(format, model_info->registers[i].defvalue);
		    printf("]");
		    printf("\n");
		}
	    }
	} else {
	    printf("No registers");
	}
	printf("\n");
    }

    return 0;
}
