/**
 *
 * @file int.c
 * @author Guillermo Marcus
 * @date 2009-04-05
 * @brief Contains the interrupt handler.
 *
 */

/*
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.7  2008-01-11 10:18:28  marcus
 * Modified interrupt mechanism. Added atomic functions and queues, to address race conditions. Removed unused interrupt code.
 *
 * Revision 1.6  2007-11-04 20:58:22  marcus
 * Added interrupt generator acknowledge.
 * Fixed wrong operator.
 *
 * Revision 1.5  2007-10-31 15:42:21  marcus
 * Added IG ack for testing, may be removed later.
 *
 * Revision 1.4  2007-07-17 13:15:56  marcus
 * Removed Tasklets.
 * Using newest map for the ABB interrupts.
 *
 * Revision 1.3  2007-07-05 15:30:30  marcus
 * Added support for both register maps of the ABB.
 *
 * Revision 1.2  2007-05-29 07:50:18  marcus
 * Split code into 2 files. May get merged in the future again....
 *
 * Revision 1.1  2007/03/01 16:57:43  marcus
 * Divided driver file to ease the interrupt hooks for the user of the driver.
 * Modified Makefile accordingly.
 *
 */

#include <linux/version.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <stdbool.h>

#include "config.h"

#include "compat.h"

#include "pciDriver.h"

#include "common.h"

#include "int.h"

/*
 * The ID between IRQ_SOURCE in irq_outstanding and the actual source is arbitrary.
 * Therefore, be careful when communicating with multiple implementations. 
 */

/* IRQ_SOURCES */
#define ABB_IRQ_CH0 	        0
#define ABB_IRQ_CH1 	        1
#define ABB_IRQ_IG 	        2

/* See ABB user’s guide, register definitions (3.1) */
#define ABB_INT_ENABLE 	        (0x0010 >> 2)
#define ABB_INT_STAT 	        (0x0008 >> 2)

#define ABB_INT_CH1_TIMEOUT     (1 << 4)
#define ABB_INT_CH0_TIMEOUT     (1 << 5)
#define ABB_INT_IG  	        (1 << 2)
#define ABB_INT_CH0 	        (1 << 1) /* downstream */
#define ABB_INT_CH1 	        (1)     /* upstream */

#define ABB_CH0_CTRL  	        (108 >> 2)
#define ABB_CH1_CTRL  	        (72 >> 2)
#define ABB_CH_RESET 	        (0x0201000A)
#define ABB_IG_CTRL 	        (0x0080 >> 2)
#define ABB_IG_ACK 	        (0x00F0)

/**
 *
 * If IRQ-handling is enabled, this function will be called from pcidriver_probe
 * to initialize the IRQ handling (maps the BARs)
 *
 */
int pcidriver_probe_irq(pcidriver_privdata_t *privdata)
{
	unsigned char int_pin, int_line;
	unsigned long bar_addr, bar_len, bar_flags;
	int i;
	int err;

	for (i = 0; i < 6; i++)
		privdata->bars_kmapped[i] = NULL;

	for (i = 0; i < 6; i++) {
		bar_addr = pci_resource_start(privdata->pdev, i);
		bar_len = pci_resource_len(privdata->pdev, i);
		bar_flags = pci_resource_flags(privdata->pdev, i);

		/* check if it is a valid BAR, skip if not */
		if ((bar_addr == 0) || (bar_len == 0))
			continue;

		/* Skip IO regions (map only mem regions) */
		if (bar_flags & IORESOURCE_IO)
			continue;

		/* Check if the region is available */
		if ((err = pci_request_region(privdata->pdev, i, NULL)) != 0) {
			mod_info( "Failed to request BAR memory region.\n" );
			return err;
		}

		/* Map it into kernel space. */
		/* For x86 this is just a dereference of the pointer, but that is
		 * not portable. So we need to do the portable way. Thanks Joern!
		 */

		/* respect the cacheable-bility of the region */
		if (bar_flags & IORESOURCE_PREFETCH)
			privdata->bars_kmapped[i] = ioremap(bar_addr, bar_len);
		else
			privdata->bars_kmapped[i] = ioremap_nocache(bar_addr, bar_len);

		/* check for error */
		if (privdata->bars_kmapped[i] == NULL) {
			mod_info( "Failed to remap BAR%d into kernel space.\n", i );
			return -EIO;
		}
	}

	/* Initialize the interrupt handler for this device */
	/* Initialize the wait queues */
	for (i = 0; i < PCIDRIVER_INT_MAXSOURCES; i++) {
		init_waitqueue_head(&(privdata->irq_queues[i]));
		atomic_set(&(privdata->irq_outstanding[i]), 0);
	}

	/* Initialize the irq config */
	if ((err = pci_read_config_byte(privdata->pdev, PCI_INTERRUPT_PIN, &int_pin)) != 0) {
		/* continue without interrupts */
		int_pin = 0;
		mod_info("Error getting the interrupt pin. Disabling interrupts for this device\n");
	}

	/* Disable interrupts and activate them if everything can be set up properly */
	privdata->irq_enabled = 0;

	if (int_pin == 0)
		return 0;

	if ((err = pci_read_config_byte(privdata->pdev, PCI_INTERRUPT_LINE, &int_line)) != 0) {
		mod_info("Error getting the interrupt line. Disabling interrupts for this device\n");
		return 0;
	}

	/* Enable interrupts using MSI mode */
	if (!pci_enable_msi(privdata->pdev)) 
		privdata->msi_mode = 1;
	
	/* register interrupt handler */
	if ((err = request_irq(privdata->pdev->irq, pcidriver_irq_handler, MODNAME, privdata)) != 0) {
		mod_info("Error registering the interrupt handler. Disabling interrupts for this device\n");
		return 0;
	}

	privdata->irq_enabled = 1;
	mod_info("Registered Interrupt Handler at pin %i, line %i, IRQ %i\n", int_pin, int_line, privdata->pdev->irq );

	return 0;
}

/**
 *
 * Frees/cleans up the data structures, called from pcidriver_remove()
 *
 */
void pcidriver_remove_irq(pcidriver_privdata_t *privdata)
{
	/* Release the IRQ handler */
	if (privdata->irq_enabled != 0)
		free_irq(privdata->pdev->irq, privdata);
	
	if (privdata->msi_mode) {
		pci_disable_msi(privdata->pdev);
		privdata->msi_mode = 0;
	}

	pcidriver_irq_unmap_bars(privdata);
}

/**
 *
 * Unmaps the BARs and releases them
 *
 */
void pcidriver_irq_unmap_bars(pcidriver_privdata_t *privdata)
{
	int i;

	for (i = 0; i < 6; i++) {
		if (privdata->bars_kmapped[i] == NULL)
			continue;

		iounmap((void*)privdata->bars_kmapped[i]);
		pci_release_region(privdata->pdev, i);
	}
}

/**
 *
 * Acknowledges the receival of an interrupt to the card.
 *
 * @returns true if the card was acknowledget
 * @returns false if the interrupt was not for one of our cards
 *
 * @see check_acknowlegde_channel
 *
 */
static bool pcidriver_irq_acknowledge(pcidriver_privdata_t *privdata)
{
	int channel = 0;
//	volatile unsigned int *bar;
//	bar = privdata->bars_kmapped[0];
//	mod_info_dbg("interrupt registers. ISR: %x, IER: %x\n", bar[ABB_INT_STAT], bar[ABB_INT_ENABLE]);

	atomic_inc(&(privdata->irq_outstanding[channel]));
	wake_up_interruptible(&(privdata->irq_queues[channel]));
	
	return true;
}

/**
 *
 * Handles IRQs. At the moment, this acknowledges the card that this IRQ
 * was received and then increases the driver's IRQ counter.
 *
 * @see pcidriver_irq_acknowledge
 *
 */
IRQ_HANDLER_FUNC(pcidriver_irq_handler)
{
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)dev_id;

	if (!pcidriver_irq_acknowledge(privdata))
		return IRQ_NONE;

	privdata->irq_count++;
	return IRQ_HANDLED;
}
