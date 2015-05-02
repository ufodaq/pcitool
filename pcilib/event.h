#ifndef _PCILIB_EVENT_H
#define _PCILIB_EVENT_H

#include <pcilib.h>
#include <pcilib/version.h>
#include <pcilib/dma.h>

#define PCILIB_EVENT_INTERFACE_VERSION PCILIB_VERSION

typedef struct {
    size_t max_events;
    pcilib_timeout_t duration;
} pcilib_autostop_parameters_t;

typedef struct {
    pcilib_event_rawdata_callback_t callback;
    void *user;
} pcilib_rawdata_parameters_t;

typedef struct {
    size_t max_threads;
} pcilib_parallel_parameters_t;

typedef struct {
    pcilib_autostop_parameters_t autostop;
    pcilib_rawdata_parameters_t rawdata;
    pcilib_parallel_parameters_t parallel;
} pcilib_event_parameters_t;

struct pcilib_event_context_s {
    pcilib_event_parameters_t params;
    pcilib_t *pcilib;
};

typedef struct {
    pcilib_event_t evid;
    const char *name;
    const char *description;
} pcilib_event_description_t;

typedef struct {
    pcilib_event_data_type_t data_type;
    pcilib_event_t evid;
    const char *name;
    const char *description;
} pcilib_event_data_type_description_t;

typedef enum {
    PCILIB_STREAMING_STOP = 0, 		/**< stop streaming */
    PCILIB_STREAMING_CONTINUE = 1, 	/**< wait the default DMA timeout for a new data */
    PCILIB_STREAMING_WAIT = 2,		/**< wait the specified timeout for a new data */
    PCILIB_STREAMING_CHECK = 3,		/**< do not wait for the data, bail out imideatly if no data ready */
    PCILIB_STREAMING_FAIL = 4,		/**< fail if data is not available on timeout */
    PCILIB_STREAMING_REQ_FRAGMENT = 5,	/**< only fragment of a packet is read, wait for next fragment and fail if no data during DMA timeout */
    PCILIB_STREAMING_REQ_PACKET = 6,	/**< wait for next packet and fail if no data during the specified timeout */
    PCILIB_STREAMING_TIMEOUT_MASK = 3	/**< mask specifying all timeout modes */
} pcilib_streaming_action_t;

/*
 * get_data: This call is used by get_data and copy_data functions of public  
 * interface. When copy_data is the caller, the data parameter will be passed.
 * Therefore, depending on data the parameter, the function should behave
 * diferently. If get get_data function is used (buf == NULL), the caller is 
 * expected to call return_data afterwards. Otherwise, if buf != NULL and 
 * copy_data is used, the return call will not be executed.
 * Still, the get_data function is not obliged to return the data in the
 * passed buf, but a reference to the staticaly allocated memory may be 
 * returned instead. The copy can be managed by the envelope function.
 */

typedef struct {
    pcilib_version_t version;

    pcilib_context_t *(*init)(pcilib_t *ctx);
    void (*free)(pcilib_context_t *ctx);

    pcilib_dma_context_t *(*init_dma)(pcilib_context_t *ctx);

    int (*reset)(pcilib_context_t *ctx);

    int (*start)(pcilib_context_t *ctx, pcilib_event_t event_mask, pcilib_event_flags_t flags);
    int (*stop)(pcilib_context_t *ctx, pcilib_event_flags_t flags);
    int (*trigger)(pcilib_context_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);
    
    int (*stream)(pcilib_context_t *ctx, pcilib_event_callback_t callback, void *user);
    int (*next_event)(pcilib_context_t *ctx, pcilib_timeout_t timeout, pcilib_event_id_t *evid, size_t info_size, pcilib_event_info_t *info);

    int (*get_data)(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size, void **data);
    int (*return_data)(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, void *data);
} pcilib_event_api_description_t;


int pcilib_init_event_engine(pcilib_t *ctx);


#endif /* _PCILIB_EVENT_H */
