#ifndef _PCIDRIVER_BASE_H
#define _PCIDRIVER_BASE_H

#include "config.h"
#include "compat.h"
#include "debug.h"
#include "pcibus.h"
#include "sysfs.h"
#include "dev.h"
#include "int.h"

pcidriver_privdata_t *pcidriver_get_privdata(int devid);
void pcidriver_put_privdata(pcidriver_privdata_t *privdata);

#endif /* _PCIDRIVER_BASE_H */
