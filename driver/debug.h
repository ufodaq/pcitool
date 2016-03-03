#ifndef _PCIDRIVER_DEBUG_H
#define _PCIDRIVER_DEBUG_H

#include "config.h"

#ifdef DEBUG
#define mod_info( args... ) \
    do { printk( KERN_INFO "%s - %s : ", MODNAME , __FUNCTION__ );\
    printk( args ); } while(0)
#define mod_info_dbg( args... ) \
    do { printk( KERN_INFO "%s - %s : ", MODNAME , __FUNCTION__ );\
    printk( args ); } while(0)
#else
#define mod_info( args... ) \
    do { printk( KERN_INFO "%s: ", MODNAME );\
    printk( args ); } while(0)
#define mod_info_dbg( args... )
#endif

#define mod_crit( args... ) \
    do { printk( KERN_CRIT "%s: ", MODNAME );\
    printk( args ); } while(0)


#endif /* _PCIDRIVER_DEBUG_H */
