#ifndef _PCILIB_DMA_IPE_H
#define _PCILIB_DMA_IPE_H

#include <stdio.h>
#include "pcilib.h"
#include "version.h"

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
    { PCILIB_REGISTER_BANK_DMA + 1, PCILIB_REGISTER_PROTOCOL_SOFTWARE_REGISTERS, PCILIB_BARUNDEF, 0, 0, 32, 0x0003, PCILIB_LITTLE_ENDIAN, PCILIB_LITTLE_ENDIAN, "0x%lx", "test", "test Registers"},
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
    {0x0028, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "mwr_perf",			"MWR Performance"},
    {0x003C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "cfg_lnk_width",		"Negotiated and max width of PCIe Link"},
    {0x003C, 	0, 	6, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "cfg_cap_max_lnk_width", 		"Max link width"},
    {0x003C, 	8, 	6, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "cfg_prg_max_lnk_width", 		"Negotiated link width"},
    {0x0040, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "cfg_payload_size",  		""},
    {0x0040, 	0, 	4, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "cfg_cap_max_payload_size",	"Max payload size"},
    {0x0040, 	8, 	3, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "cfg_prg_max_payload_size",	"Prog max payload size"},
    {0x0040, 	16, 	3, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "cfg_max_rd_req_size",		"Max read request size"},
    {0x0050, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "desc_mem_din",  		"Descriptor memory"},
    {0x0054, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "update_addr",  		"Address of progress register"},
    {0x0058, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "last_descriptor_read",	"Last descriptor read by the host"},
    {0x005C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "desc_mem_addr", 		"Number of descriptors configured"},
    {0x0060, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "update_thresh",  		"Update threshold of progress register"},
   {0x0001, 	0, 	32, 	0x1, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA +1, "test1",  		"ister"},
    {0x0002, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA +1, "test2",  		"ter"},
    {0x0003, 	0, 	32, 	0x020, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA +1, "test3",  		"er"},
    {0,		0,	0,	0,	0x00000000,	0,                                           0,                        0, NULL, 			NULL}
};
#endif /* _PCILIB_EXPORT_C */


#endif /* _PCILIB_DMA_IPE_H */
