#ifndef _PCILIB_DMA_NWL_H
#define _PCILIB_DMA_NWL_H

#include <stdio.h>
#include "../pcilib.h"

pcilib_dma_context_t *dma_nwl_init(pcilib_t *ctx, const char *model, const void *arg);
void  dma_nwl_free(pcilib_dma_context_t *vctx);

int dma_nwl_get_status(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers);

int dma_nwl_enable_irq(pcilib_dma_context_t *vctx, pcilib_irq_type_t type, pcilib_dma_flags_t flags);
int dma_nwl_disable_irq(pcilib_dma_context_t *vctx, pcilib_dma_flags_t flags);
int dma_nwl_acknowledge_irq(pcilib_dma_context_t *ctx, pcilib_irq_type_t irq_type, pcilib_irq_source_t irq_source);

int dma_nwl_start(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);
int dma_nwl_stop(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);

int dma_nwl_write_fragment(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *data, size_t *written);
int dma_nwl_stream_read(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr);
double dma_nwl_benchmark(pcilib_dma_context_t *vctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);

#ifdef _PCILIB_CONFIG_C
static const pcilib_dma_api_description_t nwl_dma_api = {
    dma_nwl_init,
    dma_nwl_free,
    dma_nwl_get_status,
    dma_nwl_enable_irq,
    dma_nwl_disable_irq,
    dma_nwl_acknowledge_irq,
    dma_nwl_start,
    dma_nwl_stop,
    dma_nwl_write_fragment,
    dma_nwl_stream_read,
    dma_nwl_benchmark
};

static pcilib_register_bank_description_t nwl_dma_banks[] = {
    { PCILIB_REGISTER_BANK_DMA, PCILIB_BAR0, 0xA000, PCILIB_REGISTER_PROTOCOL_DEFAULT    , 0,                        0,                        PCILIB_LITTLE_ENDIAN, 32, PCILIB_LITTLE_ENDIAN, "0x%lx", "dma", "DMA Registers"},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

static pcilib_register_description_t nwl_dma_registers[] = {
    {0x4000, 	0, 	32, 	0, 	0x00000011,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_control_and_status",  ""},
    {0x4000, 	0, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_interrupt_enable",  ""},
    {0x4000, 	1, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_interrupt_active",  ""},
    {0x4000, 	2, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_interrupt_pending",  ""},
    {0x4000, 	3, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_interrupt_mode",  ""},
    {0x4000, 	4, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_user_interrupt_enable",  ""},
    {0x4000, 	5, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_user_interrupt_active",  ""},
    {0x4000, 	16, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_s2c_interrupt_status",  ""},
    {0x4000, 	24, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_c2s_interrupt_status",  ""},
    {0x8000, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_design_version",  ""},
    {0x8000, 	0, 	4, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_subversion_number",  ""},
    {0x8000, 	4, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_version_number",  ""},
    {0x8000, 	28, 	4, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_targeted_device",  ""},
    {0x8200, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_transmit_utilization",  ""},
    {0x8200, 	0, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_transmit_sample_count",  ""},
    {0x8200, 	2, 	30, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_transmit_dword_count",  ""},
    {0x8204, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_receive_utilization",  ""},
    {0x8004, 	0, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_receive_sample_count",  ""},
    {0x8004, 	2, 	30, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_receive_dword_count",  ""},
    {0x8208, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_mwr",  ""},
    {0x8008, 	0, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_mwr_sample_count",  ""},
    {0x8008, 	2, 	30, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_mwr_dword_count",  ""},
    {0x820C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_cpld",  ""},
    {0x820C, 	0, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_cpld_sample_count",  ""},
    {0x820C, 	2, 	30, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_cpld_dword_count",  ""},
    {0x8210, 	0, 	12, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_cpld",  ""},
    {0x8214, 	0, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_cplh",  ""},
    {0x8218, 	0, 	12, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_npd",  ""},
    {0x821C, 	0, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_nph",  ""},
    {0x8220, 	0, 	12, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_pd",  ""},
    {0x8224, 	0, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_ph",  ""},
    {0,		0,	0,	0,	0x00000000,	0,                                           0,                        0, NULL, NULL}
};

#endif /* _PCILIB_CONFIG_C */

#ifdef _PCILIB_DMA_NWL_C 
 // DMA Engine Registers
#define NWL_MAX_DMA_ENGINE_REGISTERS 64
#define NWL_MAX_REGISTER_NAME 128
static char nwl_dma_engine_register_names[PCILIB_MAX_DMA_ENGINES * NWL_MAX_DMA_ENGINE_REGISTERS][NWL_MAX_REGISTER_NAME];
static pcilib_register_description_t nwl_dma_engine_registers[] = {
    {0x0000, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_engine_capabilities",  ""},
    {0x0000, 	0, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_present",  ""},
    {0x0000, 	1, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_direction",  ""},
    {0x0000, 	4, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_type",  ""},
    {0x0000, 	8, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_number",  ""},
    {0x0000, 	24, 	6, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_max_buffer_size",  ""},
    {0x0004, 	0, 	32, 	0, 	0x0000C100,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_engine_control",  ""},
    {0x0004, 	0, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_interrupt_enable",  ""},
    {0x0004, 	1, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_interrupt_active",  ""},
    {0x0004, 	2, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_descriptor_complete",  ""},
    {0x0004, 	3, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_descriptor_alignment_error",  ""},
    {0x0004, 	4, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_descriptor_fetch_error",  ""},
    {0x0004, 	5, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_sw_abort_error",  ""},
    {0x0004, 	8, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_enable",  ""},
    {0x0004, 	10, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_running",  ""},
    {0x0004, 	11, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_waiting",  ""},
    {0x0004, 	14, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_reset_request", ""},
    {0x0004, 	15, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_W   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_reset", ""},
    {0x0008, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_next_descriptor",  ""},
    {0x000C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_sw_descriptor",  ""},
    {0x0010, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_last_descriptor",  ""},
    {0x0014, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_active_time",  ""},
    {0x0018, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_wait_time",  ""},
    {0x001C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_counter",  ""},
    {0x001C, 	0, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_sample_count",  ""},
    {0x001C, 	2, 	30, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_dword_count",  ""},
    {0,		0,	0,	0,	0x00000000,	0,                                          0,                        0, NULL, NULL}
};

 // XRAWDATA registers
static pcilib_register_description_t nwl_xrawdata_registers[] = {
    {0x9100, 	0, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "xrawdata_enable_generator",  ""},
    {0x9104, 	0, 	16, 	0, 	0x00000000,	PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "xrawdata_packet_length",  ""},
    {0x9108, 	0, 	2, 	0, 	0x00000003,	PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "xrawdata_control",  ""},
    {0x9108, 	0, 	1, 	0, 	0x00000003,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "xrawdata_enable_checker",  ""},
    {0x9108, 	1, 	1, 	0, 	0x00000003,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "xrawdata_enable_loopback",  ""},
    {0x910C, 	0, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "xrawdata_data_mistmatch",  ""},
    {0,		0,	0,	0,	0x00000000,	0,                                            0,                        0, NULL, NULL}
};
#endif /* _PCILIB_DMA_NWL_C */

#endif /* _PCILIB_DMA_NWL_H */
