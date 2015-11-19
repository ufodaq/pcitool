#ifndef PCIDRIVER_H_
#define PCIDRIVER_H_

/**
 * This is a full rewrite of the pciDriver.
 * New default is to support kernel 2.6, using kernel 2.6 APIs.
 * 
 * This header defines the interface to the outside world.
 * 
 * $Revision: 1.6 $
 * $Date: 2008-01-24 14:21:36 $
 * 
 */

/*
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2008-01-11 10:15:14  marcus
 * Removed unused interrupt code.
 * Added intSource to the wait interrupt call.
 *
 * Revision 1.4  2006/11/17 18:44:42  marcus
 * Type of SG list can now be selected at runtime. Added type to sglist.
 *
 * Revision 1.3  2006/11/17 16:23:02  marcus
 * Added slot number to the PCI info IOctl.
 *
 * Revision 1.2  2006/11/13 12:29:09  marcus
 * Added a IOctl call, to confiure the interrupt response. (testing pending).
 * Basic interrupts are now supported.
 *
 * Revision 1.1  2006/10/10 14:46:52  marcus
 * Initial commit of the new pciDriver for kernel 2.6
 *
 * Revision 1.7  2006/10/06 15:18:06  marcus
 * Updated PCI info and PCI cmd
 *
 * Revision 1.6  2006/09/25 16:51:07  marcus
 * Added PCI config IOctls, and implemented basic mmap functions.
 *
 * Revision 1.5  2006/09/18 17:13:12  marcus
 * backup commit.
 *
 * Revision 1.4  2006/09/15 15:44:41  marcus
 * backup commit.
 *
 * Revision 1.3  2006/08/15 11:40:02  marcus
 * backup commit.
 *
 * Revision 1.2  2006/08/12 18:28:42  marcus
 * Sync with the laptop
 *
 * Revision 1.1  2006/08/11 15:30:46  marcus
 * Sync with the laptop
 *
 */

#include <linux/ioctl.h>

#define PCIDRIVER_INTERFACE_VERSION 1					/**< Driver API version, only the pcilib with the same driver interface version is allowed */

/* Identifies the PCI-E Xilinx ML605 */
#define PCIE_XILINX_VENDOR_ID 0x10ee
#define PCIE_ML605_DEVICE_ID 0x6024

/* Identifies the PCI-E IPE Hardware */
#define PCIE_IPECAMERA_DEVICE_ID 0x6081
#define PCIE_KAPTURE_DEVICE_ID 0x6028


/* Possible values for ioctl commands */

/* PCI mmap areas */
#define	PCIDRIVER_BAR0			0
#define	PCIDRIVER_BAR1			1
#define	PCIDRIVER_BAR2			2
#define	PCIDRIVER_BAR3			3
#define	PCIDRIVER_BAR4			4
#define	PCIDRIVER_BAR5			5

/* mmap mode of the device */
#define PCIDRIVER_MMAP_PCI		0
#define PCIDRIVER_MMAP_KMEM 		1

/* Direction of a DMA operation */
#define PCIDRIVER_DMA_BIDIRECTIONAL 	0
#define	PCIDRIVER_DMA_TODEVICE		1//PCILIB_KMEM_SYNC_TODEVICE
#define PCIDRIVER_DMA_FROMDEVICE	2//PCILIB_KMEM_SYNC_FROMDEVICE

/* Possible sizes in a PCI command */
#define PCIDRIVER_PCI_CFG_SZ_BYTE  	1
#define PCIDRIVER_PCI_CFG_SZ_WORD  	2
#define PCIDRIVER_PCI_CFG_SZ_DWORD 	3

/* Possible types of SG lists */
#define PCIDRIVER_SG_NONMERGED 		0
#define PCIDRIVER_SG_MERGED 		1

/* Maximum number of interrupt sources */
#define PCIDRIVER_INT_MAXSOURCES 	16

#define KMEM_REF_HW 		0x80000000				/**< Special reference to indicate hardware access */
#define KMEM_REF_COUNT		0x0FFFFFFF				/**< Mask of reference counter (mmap/munmap), couting in mmaped memory pages */

#define KMEM_MODE_REUSABLE	0x80000000				/**< Indicates reusable buffer */
#define KMEM_MODE_EXCLUSIVE	0x40000000				/**< Only a single process is allowed to mmap the buffer */
#define KMEM_MODE_PERSISTENT	0x20000000				/**< Persistent mode instructs kmem_free to preserve buffer in memory */
#define KMEM_MODE_COUNT		0x0FFFFFFF				/**< Mask of reuse counter (alloc/free) */

#define KMEM_FLAG_REUSE 		PCILIB_KMEM_FLAG_REUSE		/**< Try to reuse existing buffer with the same use & item */
#define KMEM_FLAG_EXCLUSIVE 		PCILIB_KMEM_FLAG_EXCLUSIVE	/**< Allow only a single application accessing a specified use & item */
#define KMEM_FLAG_PERSISTENT		PCILIB_KMEM_FLAG_PERSISTENT	/**< Sets persistent mode */
#define KMEM_FLAG_HW			PCILIB_KMEM_FLAG_HARDWARE	/**< The buffer may be accessed by hardware, the hardware access will not occur any more if passed to _free function */
#define KMEM_FLAG_FORCE			PCILIB_KMEM_FLAG_FORCE		/**< Force memory cleanup even if references are present */
#define KMEM_FLAG_MASS			PCILIB_KMEM_FLAG_MASS		/**< Apply to all buffers of selected use */
#define KMEM_FLAG_TRY			PCILIB_KMEM_FLAG_TRY		/**< Do not allocate buffers, try to reuse and fail if not possible */

#define KMEM_FLAG_REUSED 		PCILIB_KMEM_FLAG_REUSE		/**< Indicates if buffer with specified use & item was already allocated and reused */
#define KMEM_FLAG_REUSED_PERSISTENT 	PCILIB_KMEM_FLAG_PERSISTENT	/**< Indicates that reused buffer was persistent before the call */
#define KMEM_FLAG_REUSED_HW 		PCILIB_KMEM_FLAG_HARDWARE	/**< Indicates that reused buffer had a HW reference before the call */

/* Types */

typedef struct {
    unsigned long version;				/**< pcilib version */
    unsigned long interface;				/**< driver interface version */
    unsigned long ioctls;				/**< number of supporterd ioctls */
    unsigned long reserved[5];				/**< reserved for the future use */
} pcilib_driver_version_t;

typedef struct {
    unsigned long iommu;
    unsigned long dma_mask;
} pcilib_device_state_t;

typedef struct {
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned short bus;
    unsigned short slot;
    unsigned short func;
    unsigned short devfn;
    unsigned char interrupt_pin;
    unsigned char interrupt_line;
    unsigned int irq;
    unsigned long bar_start[6];
    unsigned long bar_length[6];
    unsigned long bar_flags[6];
} pcilib_board_info_t;

typedef struct {
	unsigned long type;
	unsigned long pa;
	unsigned long size;
	unsigned long align;
	unsigned long use;
	unsigned long item;
	int flags;
	int handle_id;
} kmem_handle_t;

typedef struct {
	unsigned long addr;
	unsigned long size;
} umem_sgentry_t;

typedef struct {
	int handle_id;
	int type;
	int nents;
	umem_sgentry_t *sg;
} umem_sglist_t;

typedef struct {
	unsigned long vma;
	unsigned long size;
	int handle_id;
	int dir;
} umem_handle_t;

typedef struct {
	kmem_handle_t handle;
	int dir;
} kmem_sync_t;

typedef struct {
    unsigned long count;
    unsigned long timeout;	// microseconds
    unsigned int source;
} interrupt_wait_t;

typedef struct {
	int size;
	int addr;
	union {
		unsigned char byte;
		unsigned short word;
		unsigned int dword; 	/* not strict C, but if not can have problems */
	} val;
} pci_cfg_cmd;

/* ioctl interface */
/* See documentation for a detailed usage explanation */

/* 
 * one of the problems of ioctl, is that requires a type definition.
 * This type is only 8-bits wide, and half-documented in 
 * <linux-src>/Documentation/ioctl-number.txt.
 * previous SHL -> 'S' definition, conflicts with several devices,
 * so I changed it to be pci -> 'p', in the range 0xA0-BF
 */
#define PCIDRIVER_IOC_MAGIC 'p'
#define PCIDRIVER_IOC_BASE  0xA0

#define PCIDRIVER_IOC_MMAP_MODE		_IO(   PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 0 )
#define PCIDRIVER_IOC_MMAP_AREA		_IO(   PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 1 )
#define PCIDRIVER_IOC_KMEM_ALLOC	_IOWR( PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 2, kmem_handle_t * )
#define PCIDRIVER_IOC_KMEM_FREE		_IOW ( PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 3, kmem_handle_t * )
#define PCIDRIVER_IOC_KMEM_SYNC		_IOWR( PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 4, kmem_sync_t * )
#define PCIDRIVER_IOC_UMEM_SGMAP	_IOWR( PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 5, umem_handle_t * )
#define PCIDRIVER_IOC_UMEM_SGUNMAP	_IOW(  PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 6, umem_handle_t * )
#define PCIDRIVER_IOC_UMEM_SGGET	_IOWR( PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 7, umem_sglist_t * )
#define PCIDRIVER_IOC_UMEM_SYNC		_IOW(  PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 8, umem_handle_t * )
#define PCIDRIVER_IOC_WAITI		_IO(   PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 9 )

/* And now, the methods to access the PCI configuration area */
#define PCIDRIVER_IOC_PCI_CFG_RD  	_IOWR(  PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 10, pci_cfg_cmd * )
#define PCIDRIVER_IOC_PCI_CFG_WR  	_IOWR(  PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 11, pci_cfg_cmd * )
#define PCIDRIVER_IOC_PCI_INFO    	_IOWR(  PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 12, pcilib_board_info_t * )

/* Clear interrupt queues */
#define PCIDRIVER_IOC_CLEAR_IOQ		_IO(   PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 13 )

#define PCIDRIVER_IOC_VERSION		_IOR(  PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 14, pcilib_driver_version_t * )
#define PCIDRIVER_IOC_DEVICE_STATE	_IOR(  PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 15, pcilib_device_state_t * )
#define PCIDRIVER_IOC_DMA_MASK		_IO(   PCIDRIVER_IOC_MAGIC, PCIDRIVER_IOC_BASE + 16)

#define PCIDRIVER_IOC_MAX 16

#endif
