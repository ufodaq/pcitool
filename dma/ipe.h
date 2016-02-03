#ifndef _PCILIB_DMA_IPE_H
#define _PCILIB_DMA_IPE_H

#include <stdio.h>
#include "pcilib.h"
#include "version.h"

#define IPEDMA_PAGE_SIZE		4096l		/**< page size */
#define IPEDMA_DMA_PAGES		1024l		/**< number of DMA pages in the ring buffer to allocate */

#define IPEDMA_DMA_TIMEOUT 		100000l		/**< us, overrides PCILIB_DMA_TIMEOUT (actual hardware timeout is 50ms according to Lorenzo) */

pcilib_dma_context_t *dma_ipe_init(pcilib_t *ctx, const char *model, const void *arg);
void  dma_ipe_free(pcilib_dma_context_t *vctx);

int dma_ipe_get_status(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers);

int dma_ipe_start(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);
int dma_ipe_stop(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);

int dma_ipe_stream_read(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr);
double dma_ipe_benchmark(pcilib_dma_context_t *vctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);

#ifdef _PCILIB_EXPORT_C
static const pcilib_dma_api_description_t ipe_dma_api = {
    PCILIB_VERSION,
    dma_ipe_init,
    dma_ipe_free,
    dma_ipe_get_status,
    NULL,
    NULL,
    NULL,
    dma_ipe_start,
    dma_ipe_stop,
    NULL,
    dma_ipe_stream_read,
    dma_ipe_benchmark
};

static const pcilib_dma_engine_description_t ipe_dma_engines[] = {
    { 0, PCILIB_DMA_TYPE_PACKET, PCILIB_DMA_FROM_DEVICE, 32, "dma", NULL },
    { 0 }
};

static const pcilib_register_bank_description_t ipe_dma_banks[] = {
    { PCILIB_REGISTER_BANK_DMA, PCILIB_REGISTER_PROTOCOL_DEFAULT, PCILIB_BAR0, 0, 0, 32, 0x0200, PCILIB_LITTLE_ENDIAN, PCILIB_LITTLE_ENDIAN, "0x%lx", "dma", "DMA Registers"},
    { PCILIB_REGISTER_BANK_DMA1, PCILIB_REGISTER_PROTOCOL_DEFAULT, PCILIB_BAR0, 0x9000, 0x9000, 32, 0x0100, PCILIB_LITTLE_ENDIAN, PCILIB_LITTLE_ENDIAN, "0x%lx", "dma9000", "Additional DMA Registers"},
    { PCILIB_REGISTER_BANK_DMACONF, PCILIB_REGISTER_PROTOCOL_SOFTWARE, PCILIB_BAR_NOBAR, 0, 0, 32, 0x1000, PCILIB_HOST_ENDIAN, PCILIB_HOST_ENDIAN, "0x%lx", "dmaconf", "DMA Configuration"},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

static const pcilib_register_description_t ipe_dma_registers[] = {
    {0x0000, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dcr",  			"Device Control Status Register"},
    {0x0000, 	0, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "reset_dma",  			""},
    {0x0000, 	16, 	4, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "datapath_width",			""},
    {0x0000, 	24, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "fpga_family",			""},
    {0x0004, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "ddmacr",  			"Device DMA Control Status Register"},
    {0x0004, 	0, 	1, 	0, 	0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mwr_start",  			"Start writting memory"},
    {0x0004, 	5, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mwr_relxed_order",  		""},
    {0x0004, 	6, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mwr_nosnoop",  			""},
    {0x0004, 	7, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mwr_int_dis",  			""},
    {0x0004, 	16, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mrd_start",  			""},
    {0x0004, 	21, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mrd_relaxed_order",  		""},
    {0x0004, 	22, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mrd_nosnoop",  			""},
    {0x0004, 	23, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mrd_int_dis",  			""},
    {0x000C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "mwr_size",  			"DMA TLP size"},
    {0x000C, 	0, 	16, 	0x20, 	0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mwr_len",  			"Max TLP size"},
    {0x000C, 	16, 	3, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mwr_tlp_tc",  			"TC for TLP packets"},
    {0x000C, 	19, 	1, 	0, 	0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mwr_64b_en",  			"Enable 64 bit memory addressing"},
    {0x000C, 	20, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mwr_phant_func_dis",		"Disable MWR phantom function"},
    {0x000C, 	24, 	8, 	0, 	0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "mwr_up_addr",  			"Upper address for 64 bit memory addressing"},
    {0x0010, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "mwr_count",  		"Write DMA TLP Count"},
    {0x0014, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "mwr_pattern",  		"DMA generator data pattern"},
    {0x0018, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_mode_flags",		"DMA operation mode"},
    {0x0018, 	0, 	4, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "pcie_gen",  			"PCIe version 2/3 depending on the used XILINX core"},
    {0x0018, 	4, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "streaming_dma", 			"Streaming mode (enabled/disabled)"},
    {0x0028, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "mwr_perf",			"MWR Performance"},
    {0x003C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "cfg_lnk_width",		"Negotiated and max width of PCIe Link"},
    {0x003C, 	0, 	6, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "cfg_cap_max_lnk_width", 		"Max link width"},
    {0x003C, 	8, 	6, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "cfg_prg_max_lnk_width", 		"Negotiated link width"},
    {0x0060, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "update_thresh",  		"Update threshold of progress register"},
	// software registers
    {0x0000, 	0, 	32, 	PCILIB_VERSION, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMACONF, "dma_version",	"Version of DMA engine"},
    {0x0004, 	0, 	32, 	IPEDMA_DMA_TIMEOUT, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMACONF, "dma_timeout",	"Default DMA timeout"},
    {0x0008, 	0, 	32, 	IPEDMA_DMA_PAGES,	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMACONF, "dma_pages",	"Number of buffers in DMA page ring"},
    {0x000C, 	0, 	32, 	IPEDMA_PAGE_SIZE,	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMACONF, "dma_page_size",	"Size of a page in DMA page ring (multiple of 4K)"},
    {0x0010, 	0, 	32, 	0,			0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMACONF, "dma_region_low",	"Low bits of static DMA I/O region"},
    {0x0014, 	0, 	32, 	0,			0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMACONF, "dma_region_hi",	"High bits of static DMA I/O region"},
    {0x0020, 	0, 	32, 	0,			0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMACONF, "ipedma_flags",	"DMA Control Register"},
    {0x0020, 	0, 	1, 	0,			0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS,	PCILIB_REGISTER_BANK_DMACONF, "ipedma_nosync",	"Do not synchronize DMA pages"},
    {0x0020, 	1, 	1, 	0,			0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS,	PCILIB_REGISTER_BANK_DMACONF, "ipedma_nosleep",	"Do not sleep while there is no data"},
    {0,		0,	0,	0,	0x00000000,	0,                                           0,                        0, NULL, 			NULL}
};
#endif /* _PCILIB_EXPORT_C */

#ifdef _PCILIB_DMA_IPE_C
static const pcilib_register_description_t ipe_dma_app_registers[] = {
    {0x0000,	0,	32,	0,	0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA1, "cntgen",			"Dummy counter"},
    {0x0000,	0,	1,	0,	0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA1, "enable_cntgen",			"Enable counting/fixed pattern dummy generator"},
    {0x0000,	4,	1,	0,	0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA1, "reset_cntgen",			"Reset dummy counter of dummy generator"},
    {0x0000,	8,	1,	0,	0xFFFFFFFF,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA1, "fix_cntgen",			"Enable fixed pattern mode of dummy generator"},
    {0x0004,	0,	32,	0,	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA1, "cntgen_pattern",		"Pattern for fixed pattern dummy generator"},
    {0x0008,	0,	32,	0,	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA1, "update_word0",		"Content of first 32-bit word in the progress register"},
    {0x0020,	0,	32,	0,	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA1, "dma_firmware",		"Version of DMA firmware"},
    {0,		0,	0,	0,	0x00000000,	0,                                           0,                        0, NULL, 			NULL}
};

static const pcilib_register_description_t ipe_dma_v2_registers[] = {
    {0x0040, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "cfg_payload_size",  		""},
    {0x0040, 	0, 	4, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "cfg_cap_max_payload_size",	"Max payload size"},
    {0x0040, 	8, 	3, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "cfg_prg_max_payload_size",	"Prog max payload size"},
    {0x0040, 	16, 	3, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "cfg_max_rd_req_size",		"Max read request size"},
    {0x0050, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "desc_mem_din",  		"Descriptor memory"},
    {0x0054, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "update_addr",  		"Address of progress register"},
    {0x0058, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "last_descriptor_read",	"Last descriptor read by the host"},
    {0x005C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "desc_mem_addr", 		"Number of descriptors configured"},
    {0,		0,	0,	0,	0x00000000,	0,                                           0,                        0, NULL, 			NULL}
};

static const pcilib_register_description_t ipe_dma_v3_registers[] = {
    {0x0040, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "desc_mem_addr", 		"Number of descriptors configured"},
    {0x0050, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "desc_mem_din_hi",  		"Descriptor memory (high bits)"},
    {0x0054, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "desc_mem_din_lo",  		"Descriptor memory (low bits)"},
    {0x0058, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "update_addr_hi",  		"Address of progress register (high bits)"},
    {0x005C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "update_addr_lo",  		"Address of progress register (low bits)"},
	// software registgers
    {0x0080, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMACONF, "last_descriptor_read",	"Last descriptor read by the host"},
    {0,		0,	0,	0,	0x00000000,	0,                                           0,                        0, NULL, 			NULL}
};
#endif /* _PCILIB_DMA_IPE_C */


#endif /* _PCILIB_DMA_IPE_H */
