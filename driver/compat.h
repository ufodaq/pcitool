/**
 *
 * @file compat.h
 * @author Michael Stapelberg
 * @date 2009-04-05
 * @brief Contains compatibility definitions for the different linux kernel versions to avoid
 * putting ifdefs all over the driver code.
 *
 */
#ifndef _COMPAT_H
#define _COMPAT_H

#include <linux/version.h>

/* Check macros and kernel version first */
#ifndef KERNEL_VERSION
# error "No KERNEL_VERSION macro! Stopping."
#endif

#ifndef LINUX_VERSION_CODE
# error "No LINUX_VERSION_CODE macro! Stopping."
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
# error "Linux 3.2 and latter are supported"
#endif

/* VM_RESERVED is removed in 3.7-rc1 */
#ifndef VM_RESERVED
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
# define __devinit
# define __devexit
# define __devinitdata
#endif

#define compat_lock_page __set_page_locked
#define compat_unlock_page __clear_page_locked


#define class_device_attribute device_attribute
#define CLASS_DEVICE_ATTR DEVICE_ATTR
#define class_device device
#define class_data dev
#define class_device_create(type, parent, devno, devpointer, nameformat, minor, privdata) \
		device_create(type, parent, devno, privdata, nameformat, minor)
#define class_device_create_file device_create_file
#define class_device_remove_file device_remove_file
#define class_device_destroy device_destroy
#define DEVICE_ATTR_COMPAT struct device_attribute *attr,
#define class_set_devdata dev_set_drvdata

#define sysfs_attr_def_name(name) dev_attr_##name
#define sysfs_attr_def_pointer privdata->class_dev
#define SYSFS_GET_FUNCTION(name) ssize_t name(struct device *dev, struct device_attribute *attr, char *buf)
#define SYSFS_SET_FUNCTION(name) ssize_t name(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
#define SYSFS_GET_PRIVDATA dev_get_drvdata(dev)

#define class_compat class


#define IRQ_HANDLER_FUNC(name) irqreturn_t name(int irq, void *dev_id)

#define request_irq(irq, irq_handler, modname, privdata) request_irq(irq, irq_handler, IRQF_SHARED, modname, privdata)


#define io_remap_pfn_range_compat(vmap, vm_start, bar_addr, bar_length, vm_page_prot) \
	io_remap_pfn_range(vmap, vm_start, (bar_addr >> PAGE_SHIFT), bar_length, vm_page_prot)

#define remap_pfn_range_compat(vmap, vm_start, bar_addr, bar_length, vm_page_prot) \
	remap_pfn_range(vmap, vm_start, (bar_addr >> PAGE_SHIFT), bar_length, vm_page_prot)

#define remap_pfn_range_cpua_compat(vmap, vm_start, cpua, size, vm_page_prot) \
	remap_pfn_range(vmap, vm_start, page_to_pfn(virt_to_page((void*)cpua)), size, vm_page_prot)


int pcidriver_pcie_get_mps(struct pci_dev *dev);
int pcidriver_pcie_set_mps(struct pci_dev *dev, int mps);

#endif
