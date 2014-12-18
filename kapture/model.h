#ifndef _KAPTURE_MODEL_H
#define _KAPTURE_MODEL_H

#include <stdio.h>

#include "../pcilib.h"


#define KAPTURE_REGISTER_SPACE 0x9000

#ifdef _KAPTURE_C
pcilib_register_bank_description_t kapture_register_banks[] = {
//    { PCILIB_REGISTER_BANK0,    PCILIB_BAR0, 0x0200, PCILIB_DEFAULT_PROTOCOL    , KAPTURE_REGISTER_SPACE, KAPTURE_REGISTER_SPACE, PCILIB_LITTLE_ENDIAN, 32, PCILIB_LITTLE_ENDIAN, "0x%lx", "fpga", "KAPTURE Registers" },
    { PCILIB_REGISTER_BANK_DMA, PCILIB_BAR0, 0x0200, PCILIB_DEFAULT_PROTOCOL    , 0,                        0,                    PCILIB_LITTLE_ENDIAN, 32, PCILIB_LITTLE_ENDIAN, "0x%lx", "dma", "DMA Registers"},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

pcilib_register_description_t kapture_registers[] = {
{0,	0,	0,	0,	0,                        0,                  0,                        0,                     NULL, NULL}
};

pcilib_register_range_t kapture_register_ranges[] = {
    {0, 0, 0, 0}
};

pcilib_event_description_t kapture_events[] = {
    {PCILIB_EVENT0, "event", ""},
    {0, NULL, NULL}
};

pcilib_event_data_type_description_t kapture_data_types[] = {
    {PCILIB_EVENT_RAW_DATA, PCILIB_EVENT0, "raw", "raw data from kapture" },
    {0, 0, NULL, NULL}
};

#else
extern pcilib_register_description_t kapture_registers[];
extern pcilib_register_bank_description_t kapture_register_banks[];
extern pcilib_register_range_t kapture_register_ranges[];
extern pcilib_event_description_t kapture_events[];
extern pcilib_event_data_type_description_t kapture_data_types[];
#endif 


pcilib_context_t *kapture_init(pcilib_t *pcilib);
void kapture_free(pcilib_context_t *ctx);

int kapture_reset(pcilib_context_t *ctx);
int kapture_start(pcilib_context_t *ctx, pcilib_event_t event_mask, pcilib_event_flags_t flags);
int kapture_stop(pcilib_context_t *ctx, pcilib_event_flags_t flags);
int kapture_trigger(pcilib_context_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);
int kapture_stream(pcilib_context_t *vctx, pcilib_event_callback_t callback, void *user);
int kapture_next_event(pcilib_context_t *vctx, pcilib_timeout_t timeout, pcilib_event_id_t *evid, size_t info_size, pcilib_event_info_t *info);
int kapture_get(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size, void **buf);
int kapture_return(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, void *data);

#ifdef _KAPTURE_C
pcilib_event_api_description_t kapture_api = {
    "kapture",
    
    kapture_init,
    kapture_free,

    NULL,

    kapture_reset,
    kapture_start,
    kapture_stop,
    kapture_trigger,
    
    kapture_stream,
    kapture_next_event,
    kapture_get,
    kapture_return
};
#else
extern pcilib_event_api_description_t kapture_api;
#endif


#endif /* _KAPTURE_MODEL_H */
