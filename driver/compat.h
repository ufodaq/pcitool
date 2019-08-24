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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
# define get_user_pages_compat(vma, nr, pages) get_user_pages(vma, nr, FOLL_WRITE, pages, NULL)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)
# define get_user_pages_compat(vma, nr, pages) get_user_pages(vma, nr, 1, 0, pages, NULL)
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,159))
    // Looks like some updates from 4.9 were backported to 4.4 LTS kernel. On Ubuntu, at least 4.4.159 needs this variant
# define get_user_pages_compat(vma, nr, pages) get_user_pages(current, current->mm, vma, nr, FOLL_WRITE, pages, NULL)
#else
# define get_user_pages_compat(vma, nr, pages) get_user_pages(current, current->mm, vma, nr, 1, 0, pages, NULL)
#endif


#endif
