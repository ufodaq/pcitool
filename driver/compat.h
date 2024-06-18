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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0) // or 6.3, to be checked (6.1 is definitively still old API)
# define get_user_pages_compat(vma, nr, pages) get_user_pages(vma, nr, FOLL_WRITE, pages)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
# define get_user_pages_compat(vma, nr, pages) get_user_pages(vma, nr, FOLL_WRITE, pages, NULL)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)
# define get_user_pages_compat(vma, nr, pages) get_user_pages(vma, nr, 1, 0, pages, NULL)
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,159))
    // Looks like some updates from 4.9 were backported to 4.4 LTS kernel. On Ubuntu, at least 4.4.159 needs this variant
# define get_user_pages_compat(vma, nr, pages) get_user_pages(current, current->mm, vma, nr, FOLL_WRITE, pages, NULL)
#else
# define get_user_pages_compat(vma, nr, pages) get_user_pages(current, current->mm, vma, nr, 1, 0, pages, NULL)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,5,0)
# define ioremap_nocache ioremap
// ioremap_nocache and ioremap are now platform independent and identical, as of
// Kernel Version 5.5
// https://lore.kernel.org/linux-mips/20191209194819.GA28157@lst.de/T/ 
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0)
# define pci_set_dma_mask(pdev, mask) dma_set_mask(&pdev->dev, mask)
# define pci_dma_mapping_error(pdev, dma_handle) dma_mapping_error(&pdev->dev, dma_handle)

# define pci_map_sg(pdev, sg, nents, dir) dma_map_sg(&pdev->dev, sg, nents, dir)
# define pci_unmap_sg(pdev, sg, nents, dir) dma_unmap_sg(&pdev->dev, sg, nents, dir)
# define pci_dma_sync_sg_for_cpu(pdev, sg, nents, dir) dma_sync_sg_for_cpu(&pdev->dev, sg, nents, dir)
# define pci_dma_sync_sg_for_device(pdev, sg, nents, dir) dma_sync_sg_for_device(&pdev->dev, sg, nents, dir)

# define pci_alloc_consistent(pdev, size, dma_handle) dma_alloc_coherent(&pdev->dev, size, dma_handle, GFP_KERNEL)
# define pci_free_consistent(pdev, size, vaddr, dma_handle) dma_free_coherent(&pdev->dev, size, vaddr, dma_handle)
# define pci_map_single(pdev, cpu_addr, size, direction) dma_map_single(&pdev->dev, cpu_addr, size, direction)
# define pci_unmap_single(pdev, dma_handle, size, direction) dma_unmap_single(&pdev->dev, dma_handle, size, direction)
# define pci_dma_sync_single_for_cpu(pdev, dma_handle, size, direction) dma_sync_single_for_cpu(&pdev->dev, dma_handle, size, direction)
# define pci_dma_sync_single_for_device(pdev, dma_handle, size, direction) dma_sync_single_for_device(&pdev->dev, dma_handle, size, direction)

# define PCI_DMA_TODEVICE DMA_TO_DEVICE
# define PCI_DMA_FROMDEVICE DMA_FROM_DEVICE
# define PCI_DMA_BIDIRECTIONAL DMA_BIDIRECTIONAL
# define PCI_DMA_NONE DMA_NONE
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0)
# define vma_flags_set_compat(vma, flags) { mmap_write_lock(vma->vm_mm); vm_flags_set(vma, flags); mmap_write_unlock(vma->vm_mm); }
#else
# define vma_flags_set_compat(vma, flags) { vma->vm_flags |= VM_RESERVED; }
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
# define class_create_compat(mod, nodename) class_create(nodename)
#else
# define class_create_compat(mod, nodename) class_create(mod, nodename)
#endif


#endif
