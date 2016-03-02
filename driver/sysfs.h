#ifndef _PCIDRIVER_SYSFS_H
#define _PCIDRIVER_SYSFS_H

#include <linux/sysfs.h>

#include "dev.h"

int pcidriver_create_sysfs_attributes(pcidriver_privdata_t *privdata);
void pcidriver_remove_sysfs_attributes(pcidriver_privdata_t *privdata);

int pcidriver_sysfs_initialize_kmem(pcidriver_privdata_t *privdata, int id, struct device_attribute *sysfs_attr);
int pcidriver_sysfs_initialize_umem(pcidriver_privdata_t *privdata, int id, struct device_attribute *sysfs_attr);
void pcidriver_sysfs_remove(pcidriver_privdata_t *privdata, struct device_attribute *sysfs_attr);

#endif /* _PCIDRIVER_SYSFS_H */
