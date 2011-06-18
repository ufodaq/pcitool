#define _PCILIB_DMA_NWL_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "pci.h"
#include "pcilib.h"
#include "error.h"
#include "tools.h"
#include "nwl.h"

/* Common DMA registers */
#define REG_DMA_CTRL_STATUS     0x4000      /**< DMA Common Ctrl & Status */

/* These engine registers are applicable to both S2C and C2S channels. 
 * Register field mask and shift definitions are later in this file.
 */

#define REG_DMA_ENG_CAP         0x00000000  /**< DMA Engine Capabilities */
#define REG_DMA_ENG_CTRL_STATUS 0x00000004  /**< DMA Engine Control */
#define REG_DMA_ENG_NEXT_BD     0x00000008  /**< HW Next desc pointer */
#define REG_SW_NEXT_BD          0x0000000C  /**< SW Next desc pointer */
#define REG_DMA_ENG_LAST_BD     0x00000010  /**< HW Last completed pointer */
#define REG_DMA_ENG_ACTIVE_TIME 0x00000014  /**< DMA Engine Active Time */
#define REG_DMA_ENG_WAIT_TIME   0x00000018  /**< DMA Engine Wait Time */
#define REG_DMA_ENG_COMP_BYTES  0x0000001C  /**< DMA Engine Completed Bytes */

/* Register masks. The following constants define bit locations of various
 * control bits in the registers. For further information on the meaning of 
 * the various bit masks, refer to the hardware spec.
 *
 * Masks have been written assuming HW bits 0-31 correspond to SW bits 0-31 
 */

/** @name Bitmasks of REG_DMA_CTRL_STATUS register.
 * @{
 */
#define DMA_INT_ENABLE              0x00000001  /**< Enable global interrupts */
#define DMA_INT_DISABLE             0x00000000  /**< Disable interrupts */
#define DMA_INT_ACTIVE_MASK         0x00000002  /**< Interrupt active? */
#define DMA_INT_PENDING_MASK        0x00000004  /**< Engine interrupt pending */
#define DMA_INT_MSI_MODE            0x00000008  /**< MSI or Legacy mode? */
#define DMA_USER_INT_ENABLE         0x00000010  /**< Enable user interrupts */
#define DMA_USER_INT_ACTIVE_MASK    0x00000020  /**< Int - user interrupt */
#define DMA_USER_INT_ACK            0x00000020  /**< Acknowledge */
#define DMA_MPS_USED                0x00000700  /**< MPS Used */
#define DMA_MRRS_USED               0x00007000  /**< MRRS Used */
#define DMA_S2C_ENG_INT_VAL         0x00FF0000  /**< IRQ value of 1st 8 S2Cs */
#define DMA_C2S_ENG_INT_VAL         0xFF000000  /**< IRQ value of 1st 8 C2Ss */

/** @name Bitmasks of REG_DMA_ENG_CAP register.
 * @{
 */
/* DMA engine characteristics */
#define DMA_ENG_PRESENT_MASK    0x00000001  /**< DMA engine present? */
#define DMA_ENG_DIRECTION_MASK  0x00000002  /**< DMA engine direction */
#define DMA_ENG_C2S             0x00000002  /**< DMA engine - C2S */
#define DMA_ENG_S2C             0x00000000  /**< DMA engine - S2C */
#define DMA_ENG_TYPE_MASK       0x00000030  /**< DMA engine type */
#define DMA_ENG_BLOCK           0x00000000  /**< DMA engine - Block type */
#define DMA_ENG_PACKET          0x00000010  /**< DMA engine - Packet type */
#define DMA_ENG_NUMBER          0x0000FF00  /**< DMA engine number */
#define DMA_ENG_BD_MAX_BC       0x3F000000  /**< DMA engine max buffer size */


/* Shift constants for selected masks */
#define DMA_ENG_NUMBER_SHIFT        8
#define DMA_ENG_BD_MAX_BC_SHIFT     24

/** @name Bitmasks of REG_DMA_ENG_CTRL_STATUS register.
 * @{
 */
/* Interrupt activity and acknowledgement bits */
#define DMA_ENG_INT_ENABLE          0x00000001  /**< Enable interrupts */
#define DMA_ENG_INT_DISABLE         0x00000000  /**< Disable interrupts */
#define DMA_ENG_INT_ACTIVE_MASK     0x00000002  /**< Interrupt active? */
#define DMA_ENG_INT_ACK             0x00000002  /**< Interrupt ack */
#define DMA_ENG_INT_BDCOMP          0x00000004  /**< Int - BD completion */
#define DMA_ENG_INT_BDCOMP_ACK      0x00000004  /**< Acknowledge */
#define DMA_ENG_INT_ALERR           0x00000008  /**< Int - BD align error */
#define DMA_ENG_INT_ALERR_ACK       0x00000008  /**< Acknowledge */
#define DMA_ENG_INT_FETERR          0x00000010  /**< Int - BD fetch error */
#define DMA_ENG_INT_FETERR_ACK      0x00000010  /**< Acknowledge */
#define DMA_ENG_INT_ABORTERR        0x00000020  /**< Int - DMA abort error */
#define DMA_ENG_INT_ABORTERR_ACK    0x00000020  /**< Acknowledge */
#define DMA_ENG_INT_CHAINEND        0x00000080  /**< Int - BD chain ended */
#define DMA_ENG_INT_CHAINEND_ACK    0x00000080  /**< Acknowledge */

/* DMA engine control */
#define DMA_ENG_ENABLE_MASK         0x00000100  /**< DMA enabled? */
#define DMA_ENG_ENABLE              0x00000100  /**< Enable DMA */
#define DMA_ENG_DISABLE             0x00000000  /**< Disable DMA */
#define DMA_ENG_STATE_MASK          0x00000C00  /**< Current DMA state? */
#define DMA_ENG_RUNNING             0x00000400  /**< DMA running */
#define DMA_ENG_IDLE                0x00000000  /**< DMA idle */
#define DMA_ENG_WAITING             0x00000800  /**< DMA waiting */
#define DMA_ENG_STATE_WAITED        0x00001000  /**< DMA waited earlier */
#define DMA_ENG_WAITED_ACK          0x00001000  /**< Acknowledge */
#define DMA_ENG_USER_RESET          0x00004000  /**< Reset only user logic */
#define DMA_ENG_RESET               0x00008000  /**< Reset DMA engine + user */

#define DMA_ENG_ALLINT_MASK         0x000000BE  /**< To get only int events */

#define DMA_ENGINE_PER_SIZE     0x100   /**< Separation between engine regs */
#define DMA_OFFSET              0       /**< Starting register offset */
                                        /**< Size of DMA engine reg space */
#define DMA_SIZE                (MAX_DMA_ENGINES * DMA_ENGINE_PER_SIZE)

/*
pcilib_register_bank_description_t ipecamera_register_banks[] = {
    { PCILIB_REGISTER_DMABANK0, PCILIB_BAR0, 128, PCILIB_DEFAULT_PROTOCOL, DMA_NWL_OFFSET, DMA_NWL_OFFSET, PCILIB_LITTLE_ENDIAN, 32, PCILIB_LITTLE_ENDIAN, "%lx", "dma", "NorthWest Logick DMA Engine" },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

pcilib_register_description_t dma_nwl_registers[] = {
    {0, 	0, 	32, 	0, 	PCILIB_REGISTER_R , PCILIB_REGISTER_DMABANK, "dma_capabilities",  ""},
    {1, 	0, 	32, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_DMABANK, "dma_control",  ""},
};
*/

typedef struct {
    pcilib_dma_engine_description_t desc;
    char *base_addr;
} pcilib_nwl_engine_description_t;


struct nwl_dma_s {
    pcilib_t *pcilib;
    
    pcilib_register_bank_description_t *dma_bank;
    char *base_addr;
    
    pcilib_dma_t n_engines;
    pcilib_nwl_engine_description_t engines[PCILIB_MAX_DMA_ENGINES + 1];
};

#define nwl_read_register(var, ctx, base, reg) pcilib_datacpy(&var, base + reg, 4, 1, ctx->dma_bank->raw_endianess)
#define nwl_write_register(var, ctx, base, reg) pcilib_datacpy(base + reg, &var, 4, 1, ctx->dma_bank->raw_endianess)

static int nwl_read_engine_config(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, char *base) {
    uint32_t val;
    
    info->base_addr = base;
    
    nwl_read_register(val, ctx, base, REG_DMA_ENG_CAP);

    if ((val & DMA_ENG_PRESENT_MASK) == 0) return PCILIB_ERROR_NOTAVAILABLE;
    
    info->desc.addr = (val & DMA_ENG_NUMBER) >> DMA_ENG_NUMBER_SHIFT;
    
    switch (val & DMA_ENG_DIRECTION_MASK) {
	case  DMA_ENG_C2S:
	    info->desc.direction = PCILIB_DMA_FROM_DEVICE;
	break;
	default:
	    info->desc.direction = PCILIB_DMA_TO_DEVICE;
    }
    
    switch (val & DMA_ENG_TYPE_MASK) {
	case DMA_ENG_BLOCK:
	    info->desc.type = PCILIB_DMA_TYPE_BLOCK;
	break;
	case DMA_ENG_PACKET:
	    info->desc.type = PCILIB_DMA_TYPE_PACKET;
	break;
	default:
	    info->desc.type = PCILIB_DMA_TYPE_UNKNOWN;
    }
    
    info->desc.addr_bits = (val & DMA_ENG_BD_MAX_BC) >> DMA_ENG_BD_MAX_BC_SHIFT;
    
    return 0;
}

static int nwl_stop_engine(nwl_dma_t *ctx, pcilib_dma_t dma) {
    uint32_t val;
    struct timeval start, cur;
    
    pcilib_nwl_engine_description_t *info = ctx->engines + dma;
    char *base = ctx->engines[dma].base_addr;
    
	// Disable IRQ
    nwl_read_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
    val &= ~(DMA_ENG_INT_ENABLE);
    nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);

     
	// Reseting 
    val = DMA_ENG_DISABLE|DMA_ENG_USER_RESET; nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
    gettimeofday(&start, NULL);
    do {
	nwl_read_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
        gettimeofday(&cur, NULL);
    } while ((val & (DMA_ENG_STATE_MASK|DMA_ENG_USER_RESET))&&(((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) < PCILIB_REGISTER_TIMEOUT));
    
    if (val & DMA_ENG_RESET) {
	pcilib_error("Timeout during reset of DMA engine %i", info->desc.addr);
	return PCILIB_ERROR_TIMEOUT;
    }
    

    val = DMA_ENG_RESET; nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
    gettimeofday(&start, NULL);
    do {
	nwl_read_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
        gettimeofday(&cur, NULL);
    } while ((val & DMA_ENG_RESET)&&(((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) < PCILIB_REGISTER_TIMEOUT));
    
    if (val & DMA_ENG_RESET) {
	pcilib_error("Timeout during reset of DMA engine %i", info->desc.addr);
	return PCILIB_ERROR_TIMEOUT;
    }

	// Acknowledge asserted engine interrupts    
    if (val & DMA_ENG_INT_ACTIVE_MASK) {
	val |= DMA_ENG_ALLINT_MASK;
	nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
    }
    
    return 0;
}

pcilib_dma_context_t *dma_nwl_init(pcilib_t *pcilib) {
    int i;
    int err;
    pcilib_dma_t n_engines;

    pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);
    
    nwl_dma_t *ctx = malloc(sizeof(nwl_dma_t));
    if (ctx) {
	memset(ctx, 0, sizeof(nwl_dma_t));
	ctx->pcilib = pcilib;
	pcilib_register_bank_t dma_bank = pcilib_find_bank_by_addr(pcilib, PCILIB_REGISTER_BANK_DMA);

	if (dma_bank == PCILIB_REGISTER_BANK_INVALID) {
	    pcilib_error("DMA Register Bank could not be found");
	    return NULL;
	}
	
	ctx->dma_bank = model_info->banks + dma_bank;
	ctx->base_addr = pcilib_resolve_register_address(pcilib, ctx->dma_bank->bar, ctx->dma_bank->read_addr);

	for (i = 0, n_engines = 0; i < 2 * PCILIB_MAX_DMA_ENGINES; i++) {
	    char *addr = ctx->base_addr + DMA_OFFSET + i * DMA_ENGINE_PER_SIZE;
	    err = nwl_read_engine_config(ctx, ctx->engines + n_engines, addr);
	    if (!err) err = nwl_stop_engine(ctx, n_engines);
	    if (!err) {
		ctx->engines[n_engines].base_addr = addr;
		pcilib_set_dma_engine_description(pcilib, n_engines, (pcilib_dma_engine_description_t*)(ctx->engines + n_engines));
		++n_engines;
	    }
	    
	}
	pcilib_set_dma_engine_description(pcilib, n_engines, NULL);
	
	ctx->n_engines = n_engines;
    }
    return (pcilib_dma_context_t*)ctx;
}

void  dma_nwl_free(pcilib_dma_context_t *vctx) {
    pcilib_dma_t i;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;
    if (ctx) {
	for (i = 0; i < ctx->n_engines; i++) nwl_stop_engine(vctx, i);
	free(ctx);
    }
}

int dma_nwl_read(pcilib_dma_context_t *vctx, pcilib_dma_t dma, size_t size, void *buf) {
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;
    printf("Reading dma: %i\n", dma);
}
