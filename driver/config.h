#ifndef _PCIDRIVER_CONFIG_H
#define _PCIDRIVER_CONFIG_H

/* Debug messages */
//#define DEBUG

/* Enable/disable IRQ handling */
#define ENABLE_IRQ

/* Maximum number of interrupt sources */
#define PCIDRIVER_INT_MAXSOURCES		16

/* Maximum number of devices*/
#define MAXDEVICES 				4

/* Minor number */
#define MINORNR 				0

/* The name of the module */
#define MODNAME 				"pciDriver"

/* Node name of the char device */
#define NODENAME 				"fpga"
#define NODENAMEFMT 				"fpga%d"

/* Identifies the PCI-E Xilinx ML605 */
#define PCIE_XILINX_VENDOR_ID 			0x10ee
#define PCIE_ML605_DEVICE_ID 			0x6024

/* Identifies the PCI-E IPE Hardware */
#define PCIE_IPECAMERA_DEVICE_ID 		0x6081
#define PCIE_KAPTURE_DEVICE_ID 			0x6028


#endif /* _PCIDRIVER_CONFIG_H */
