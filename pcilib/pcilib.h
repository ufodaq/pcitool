#ifndef _PCITOOL_PCILIB_H
#define _PCITOOL_PCILIB_H

#include <sys/time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

typedef struct pcilib_s pcilib_t;
typedef struct pcilib_event_context_s pcilib_context_t;

typedef uint32_t pcilib_version_t;

typedef uint8_t pcilib_bar_t;			/**< Type holding the PCI Bar number */
typedef uint16_t pcilib_register_t;		/**< Type holding the register position within the field listing registers in the model */
typedef uint16_t pcilib_view_t;			/**< Type holding the register view position within view listing in the model */
typedef uint16_t pcilib_unit_t;			/**< Type holding the value unit position within unit listing in the model */
typedef uint32_t pcilib_register_addr_t;	/**< Type holding the register address within address-space of BARs */
typedef uint8_t pcilib_register_size_t;		/**< Type holding the size in bits of the register */
typedef uint32_t pcilib_register_value_t;	/**< Type holding the register value */
typedef uint8_t pcilib_dma_engine_addr_t;
typedef uint8_t pcilib_dma_engine_t;
typedef uint64_t pcilib_event_id_t;
typedef uint32_t pcilib_event_t;
typedef uint64_t pcilib_timeout_t;		/**< In microseconds */
typedef unsigned int pcilib_irq_hw_source_t;
typedef uint32_t pcilib_irq_source_t;
typedef struct _xmlNode pcilib_xml_node_t;

typedef enum {
    PCILIB_LOG_DEBUG = 0,			/**< Debug messages will be always printed as they should be filtered based on setting of corresponding environmental variable */
    PCILIB_LOG_INFO,				/**< Informational message are suppresed by default */
    PCILIB_LOG_WARNING,				/**< Warnings messages indicate that something unexpected happen, but application can continue */
    PCILIB_LOG_ERROR				/**< The error which is impossible to handle on this level of library */
} pcilib_log_priority_t;

typedef enum {
    PCILIB_HOST_ENDIAN = 0,                     /**< The same byte ordering as on the host system running the driver */
    PCILIB_LITTLE_ENDIAN,                       /**< x86 is Little-endian, least significant bytes are at the lower addresses */
    PCILIB_BIG_ENDIAN                           /**< Old mainframes and network byte order, most significant bytes are at the lower addresses */
} pcilib_endianess_t;

typedef enum {
    PCILIB_ACCESS_R = 1,			/**< getting property is allowed */
    PCILIB_ACCESS_W = 2,			/**< setting property is allowed */
    PCILIB_ACCESS_RW = 3
} pcilib_access_mode_t;

typedef enum {
    PCILIB_REGISTER_R = 1,			/**< reading from register is allowed */
    PCILIB_REGISTER_W = 2,			/**< normal writting to register is allowed */
    PCILIB_REGISTER_RW = 3,
    PCILIB_REGISTER_W1C = 4,			/**< writting 1 resets the bit, writting 0 keeps the value */
    PCILIB_REGISTER_RW1C = 5,
    PCILIB_REGISTER_W1I = 8,			/**< writting 1 inversts the bit, writting 0 keeps the value */
    PCILIB_REGISTER_RW1I = 9,
} pcilib_register_mode_t;

typedef enum {
    PCILIB_TYPE_INVALID = 0,                    /**< uninitialized */
    PCILIB_TYPE_DEFAULT = 0,			/**< default type */
    PCILIB_TYPE_STRING = 1,			/**< char* */
    PCILIB_TYPE_DOUBLE = 2,			/**< double */
    PCILIB_TYPE_LONG = 3
} pcilib_value_type_t;

typedef enum {
    PCILIB_DMA_IRQ = 1,
    PCILIB_EVENT_IRQ = 2
} pcilib_irq_type_t;

typedef enum {					/**< 0x8000 and up are reserved for driver-specific types */
    PCILIB_EVENT_DATA = 0,			/**< default data format */
    PCILIB_EVENT_RAW_DATA = 1			/**< raw data */
} pcilib_event_data_type_t;

typedef enum {
    PCILIB_DMA_TO_DEVICE = 1,
    PCILIB_DMA_FROM_DEVICE = 2,
    PCILIB_DMA_BIDIRECTIONAL = 3
} pcilib_dma_direction_t;

typedef enum {
    PCILIB_DMA_FLAGS_DEFAULT = 0,
    PCILIB_DMA_FLAG_EOP = 1,			/**< last buffer of the packet */
    PCILIB_DMA_FLAG_WAIT = 2,			/**< wait completion of write operation / wait for data during read operation */
    PCILIB_DMA_FLAG_MULTIPACKET = 4,		/**< read multiple packets */
    PCILIB_DMA_FLAG_PERSISTENT = 8,		/**< do not stop DMA engine on application termination / permanently close DMA engine on dma_stop */
    PCILIB_DMA_FLAG_IGNORE_ERRORS = 16		/**< do not crash on errors, but return appropriate error codes */
} pcilib_dma_flags_t;

typedef enum {
    PCILIB_STREAMING_STOP = 0,                  /**< stop streaming */
    PCILIB_STREAMING_CONTINUE = 1,              /**< wait the default DMA timeout for a new data */
    PCILIB_STREAMING_WAIT = 2,                  /**< wait the specified timeout for a new data */
    PCILIB_STREAMING_CHECK = 3,                 /**< do not wait for the data, bail out imideatly if no data ready */
    PCILIB_STREAMING_FAIL = 4,                  /**< fail if data is not available on timeout */
    PCILIB_STREAMING_REQ_FRAGMENT = 5,          /**< only fragment of a packet is read, wait for next fragment and fail if no data during DMA timeout */
    PCILIB_STREAMING_REQ_PACKET = 6,            /**< wait for next packet and fail if no data during the specified timeout */
    PCILIB_STREAMING_TIMEOUT_MASK = 3           /**< mask specifying all timeout modes */
} pcilib_streaming_action_t;

typedef enum {
    PCILIB_EVENT_FLAGS_DEFAULT = 0,
    PCILIB_EVENT_FLAG_RAW_DATA_ONLY = 1,	/**< Do not parse data, just read raw and pass it to rawdata callback. If passed to rawdata callback, idicates the data is not identified as event (most probably just padding) */
    PCILIB_EVENT_FLAG_STOP_ONLY = 1,		/**< Do not cleanup, just stop acquiring new frames, the cleanup should be requested afterwards */
    PCILIB_EVENT_FLAG_EOF = 2,			/**< Indicates that it is the last part of the frame (not required) */
    PCILIB_EVENT_FLAG_PREPROCESS = 4		/**< Enables preprocessing of the raw data (decoding frames, etc.) */
} pcilib_event_flags_t;

typedef enum {
    PCILIB_EVENT_INFO_FLAG_BROKEN = 1		/**< Indicates broken frames (if this flag is fales, the frame still can be broken) */
} pcilib_event_info_flags_t;

typedef enum {
    PCILIB_LIST_FLAGS_DEFAULT = 0,
    PCILIB_LIST_FLAG_CHILDS = 1                 /**< Request all sub-elements or indicated that sub-elements are available */
} pcilib_list_flags_t;

typedef struct {
    pcilib_value_type_t type;                   /**< Current data type */
    const char *unit;                           /**< Units (if known) */
    const char *format;                         /**< requested printf format (may enforce using output in hex form) */

    union {
	long ival;                              /**< The value if type = PCILIB_TYPE_LONG */
	double fval;                            /**< The value if type = PCILIB_TYPE_DOUBLE */
	const char *sval;                       /**< The value if type = PCILIB_TYPE_STRING, the pointer may point to static location or reference actual string in str or data */
    };

        // This is a private part
    size_t size;                                /**< Size of the data */
    void *data;                                 /**< Arbitrary data, for instance actual string referenced by the sval */
    char str[16];                               /**< Used for shorter strings converted from integer/float types */
} pcilib_value_t;

typedef struct {
    pcilib_register_value_t min, max;           /**< Minimum and maximum allowed values */
} pcilib_register_value_range_t;

typedef struct {
    pcilib_register_value_t value;              /**< This value will get assigned instead of the name */
    pcilib_register_value_t min, max;	        /**< the values in the specified range are aliased by name */
    const char *name; 				/**< corresponding string to value */
    const char *description;                    /**< longer description */
} pcilib_register_value_name_t;

typedef struct {
    pcilib_register_t id;                       /**< Direct register ID which can be used in API calls */
    const char *name;                           /**< The access name of the register */
    const char *description;                    /**< Brief description of the register */
    const char *bank;                           /**< The name of the bank register belongs to */
    pcilib_register_mode_t mode;                /**< Register access (ro/wo/rw) and how writting to register works (clearing/inverting set bits) */
    pcilib_register_value_t defvalue;           /**< Default register value */
    const pcilib_register_value_range_t *range; /**< Specifies default, minimum, and maximum values */
    const pcilib_register_value_name_t *values; /**< The list of enum names for the register value */
} pcilib_register_info_t;

typedef struct {
    const char *name;                           /**< Name of the property view */
    const char *path;                           /**< Full path to the property */
    const char *description;                    /**< Short description */
    pcilib_value_type_t type;                   /**< The default data type or PCILIB_TYPE_INVALID if directory */
    pcilib_access_mode_t mode;                  /**< Specifies if the view is read/write-only */
    pcilib_list_flags_t flags;                  /**< Indicates if have sub-folders, etc. */
    const char *unit;                           /**< Returned unit (if any) */
} pcilib_property_info_t;

typedef struct {
    pcilib_event_t type;
    uint64_t seqnum;				/**< we will add seqnum_overflow if required */
    uint64_t offset;				/**< nanoseconds */
    struct timeval timestamp;			/**< most accurate timestamp */
    pcilib_event_info_flags_t flags;		/**< flags */
} pcilib_event_info_t;


#define PCILIB_BAR_DETECT 		((pcilib_bar_t)-1)
#define PCILIB_BAR_INVALID		((pcilib_bar_t)-1)
#define PCILIB_BAR_NOBAR		((pcilib_bar_t)-2)
#define PCILIB_BAR0			0
#define PCILIB_BAR1			1
#define PCILIB_DMA_ENGINE_INVALID	((pcilib_dma_engine_t)-1)
#define PCILIB_DMA_ENGINE_ALL		((pcilib_dma_engine_t)-1)
#define PCILIB_DMA_FLAGS_DEFAULT	((pcilib_dma_flags_t)0)
#define PCILIB_DMA_ENGINE_ADDR_INVALID	((pcilib_dma_engine_addr_t)-1)
#define PCILIB_REGISTER_INVALID		((pcilib_register_t)-1)
#define PCILIB_ADDRESS_INVALID		((uintptr_t)-1)
#define PCILIB_EVENT0			1
#define PCILIB_EVENT1			2
#define PCILIB_EVENT2			4
#define PCILIB_EVENT3			8
#define PCILIB_EVENTS_ALL		((pcilib_event_t)-1)
#define PCILIB_EVENT_INVALID		((pcilib_event_t)-1)
#define PCILIB_EVENT_DATA_TYPE_INVALID	((pcilib_event_data_type_t)-1)
#define PCILIB_TIMEOUT_INFINITE		((pcilib_timeout_t)-1)
#define PCILIB_TIMEOUT_IMMEDIATE	0
#define PCILIB_IRQ_TYPE_ALL 		0
#define PCILIB_IRQ_SOURCE_DEFAULT	0
#define PCILIB_MODEL_DETECT		NULL


typedef void (*pcilib_logger_t)(void *arg, const char *file, int line, pcilib_log_priority_t prio, const char *msg, va_list va);

/**<
 * Callback function called when new data is read by DMA streaming function
 * @ctx - DMA Engine context
 * @flags - DMA Flags
 * @bufsize - size of data in bytes
 * @buf - data
 * @returns 
 * <0 - error, stop streaming (the value is negative error code)
 *  0 - stop streaming (PCILIB_STREAMING_STOP)
 *  1 - wait DMA timeout and return gracefuly if no data (PCILIB_STREAMING_CONTINUE)
 *  2 - wait the specified timeout and return gracefuly if no data (PCILIB_STREAMING_WAIT)
 *  3 - check if more data is available without waiting, return gracefuly if not (PCILIB_STREAMING_CHECK)
 *  5 - wait DMA timeout and fail if no data (PCILIB_STREAMING_REQ_FRAGMENT)
 *  6 - wait the specified timeout and fail if no data (PCILIB_STREAMING_REQ_PACKET)
 */
typedef int (*pcilib_dma_callback_t)(void *ctx, pcilib_dma_flags_t flags, size_t bufsize, void *buf); 
typedef int (*pcilib_event_callback_t)(pcilib_event_id_t event_id, pcilib_event_info_t *info, void *user);
typedef int (*pcilib_event_rawdata_callback_t)(pcilib_event_id_t event_id, pcilib_event_info_t *info, pcilib_event_flags_t flags, size_t size, void *data, void *user);

#ifdef __cplusplus
extern "C" {
#endif



int pcilib_set_logger(pcilib_log_priority_t min_prio, pcilib_logger_t logger, void *arg);

pcilib_t *pcilib_open(const char *device, const char *model);
void pcilib_close(pcilib_t *ctx);

pcilib_property_info_t *pcilib_get_property_list(pcilib_t *ctx, const char *branch, pcilib_list_flags_t flags);
void pcilib_free_property_info(pcilib_t *ctx, pcilib_property_info_t *info);
pcilib_register_info_t *pcilib_get_register_list(pcilib_t *ctx, const char *bank, pcilib_list_flags_t flags);
pcilib_register_info_t *pcilib_get_register_info(pcilib_t *ctx, const char *req_bank_name, const char *req_reg_name, pcilib_list_flags_t flags);
void pcilib_free_register_info(pcilib_t *ctx, pcilib_register_info_t *info);


int pcilib_start_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);
int pcilib_stop_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);

    // Interrupt API is preliminary and can be significantly changed in future
int pcilib_enable_irq(pcilib_t *ctx, pcilib_irq_type_t irq_type, pcilib_dma_flags_t flags);
int pcilib_acknowledge_irq(pcilib_t *ctx, pcilib_irq_type_t irq_type, pcilib_irq_source_t irq_source);
int pcilib_disable_irq(pcilib_t *ctx, pcilib_dma_flags_t flags);

int pcilib_wait_irq(pcilib_t *ctx, pcilib_irq_hw_source_t source, pcilib_timeout_t timeout, size_t *count);
int pcilib_clear_irq(pcilib_t *ctx, pcilib_irq_hw_source_t source);

void *pcilib_map_bar(pcilib_t *ctx, pcilib_bar_t bar);
void pcilib_unmap_bar(pcilib_t *ctx, pcilib_bar_t bar, void *data);
char *pcilib_resolve_register_address(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr);	// addr is offset if bar is specified
char *pcilib_resolve_data_space(pcilib_t *ctx, uintptr_t addr, size_t *size);

pcilib_register_t pcilib_find_register(pcilib_t *ctx, const char *bank, const char *reg);
pcilib_event_t pcilib_find_event(pcilib_t *ctx, const char *event);
pcilib_event_data_type_t pcilib_find_event_data_type(pcilib_t *ctx, pcilib_event_t event, const char *data_type);
pcilib_dma_engine_t pcilib_find_dma_by_addr(pcilib_t *ctx, pcilib_dma_direction_t direction, pcilib_dma_engine_addr_t dma);

int pcilib_read(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf);
int pcilib_write(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf);
int pcilib_write_fifo(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t fifo_size, size_t n, void *buf);
int pcilib_read_fifo(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t fifo_size, size_t n, void *buf);

int pcilib_skip_dma(pcilib_t *ctx, pcilib_dma_engine_t dma);
int pcilib_stream_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr);
int pcilib_push_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *written_bytes);
int pcilib_read_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf, size_t *read_bytes);
int pcilib_read_dma_custom(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *read_bytes);
int pcilib_write_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf, size_t *written_bytes);
double pcilib_benchmark_dma(pcilib_t *ctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);

int pcilib_read_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf);
int pcilib_write_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf);
int pcilib_read_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t *value);
int pcilib_write_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t value);
int pcilib_read_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t *value);
int pcilib_write_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t value);
int pcilib_read_register_view(pcilib_t *ctx, const char *bank, const char *regname, const char *unit, pcilib_value_t *value);
int pcilib_write_register_view(pcilib_t *ctx, const char *bank, const char *regname, const char *unit, const pcilib_value_t *value);
int pcilib_read_register_view_by_id(pcilib_t *ctx, pcilib_register_t reg, const char *view, pcilib_value_t *val);
int pcilib_write_register_view_by_id(pcilib_t *ctx, pcilib_register_t reg, const char *view, const pcilib_value_t *valarg);
int pcilib_get_property(pcilib_t *ctx, const char *prop, pcilib_value_t *val);
int pcilib_set_property(pcilib_t *ctx, const char *prop, const pcilib_value_t *val);

void pcilib_clean_value(pcilib_t *ctx, pcilib_value_t *val);
int pcilib_copy_value(pcilib_t *ctx, pcilib_value_t *dst, const pcilib_value_t *src);
int pcilib_set_value_from_float(pcilib_t *ctx, pcilib_value_t *val, double fval);
int pcilib_set_value_from_int(pcilib_t *ctx, pcilib_value_t *val, long ival);
int pcilib_set_value_from_register_value(pcilib_t *ctx, pcilib_value_t *value, pcilib_register_value_t regval);
int pcilib_set_value_from_static_string(pcilib_t *ctx, pcilib_value_t *value, const char *str);
double pcilib_get_value_as_float(pcilib_t *ctx, const pcilib_value_t *val, int *err);
long pcilib_get_value_as_int(pcilib_t *ctx, const pcilib_value_t *val, int *err);
pcilib_register_value_t pcilib_get_value_as_register_value(pcilib_t *ctx, const pcilib_value_t *val, int *err);
int pcilib_convert_value_unit(pcilib_t *ctx, pcilib_value_t *val, const char *unit_name);
int pcilib_convert_value_type(pcilib_t *ctx, pcilib_value_t *val, pcilib_value_type_t type);

int pcilib_get_property_attr(pcilib_t *ctx, const char *prop, const char *attr, pcilib_value_t *val);
int pcilib_get_register_attr_by_id(pcilib_t *ctx, pcilib_register_t reg, const char *attr, pcilib_value_t *val);
int pcilib_get_register_attr(pcilib_t *ctx, const char *bank, const char *regname, const char *attr, pcilib_value_t *val);
int pcilib_get_register_bank_attr(pcilib_t *ctx, const char *bankname, const char *attr, pcilib_value_t *val);

int pcilib_reset(pcilib_t *ctx);
int pcilib_trigger(pcilib_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);

/*
 * The recording of new events will be stopped after reaching max_events records
 * or when the specified amount of time is elapsed. However, the @pcilib_stop
 * function should be called still. 
 * NOTE: This options may not be respected if the PCILIB_EVENT_FLAG_RAW_DATA_ONLY
 * is specified.
 */
int pcilib_configure_autostop(pcilib_t *ctx, size_t max_events, pcilib_timeout_t duration);

/*
 * Request auto-triggering while grabbing
 */
int pcilib_configure_autotrigger(pcilib_t *ctx, pcilib_timeout_t interval, pcilib_event_t event, size_t trigger_size, void *trigger_data);
/*
 * Request streaming the rawdata from the event engine. It is fastest way to acuqire data.
 * No memory copies will be performed and DMA buffers will be directly passed to the user
 * callback. However, to prevent data loss, no processing should be done on the data. The
 * user callback is only expected to copy data into the appropriate place and return control
 * to the event engine.
 * The performance can be boosted further by disabling any data processing within the event
 * engine. Just pass PCILIB_EVENT_FLAG_RAW_DATA_ONLY flag to the @pcilib_start function.
 */
int pcilib_configure_rawdata_callback(pcilib_t *ctx, pcilib_event_rawdata_callback_t callback, void *user);

/*
 * Configures maximal number of preprocessing threads. Actual amount of threads 
 * may be bigger. For instance, additionaly a real-time reader thread will be 
 * executed for most of hardware
 */
int pcilib_configure_preprocessing_threads(pcilib_t *ctx, size_t max_threads);

pcilib_context_t *pcilib_get_event_engine(pcilib_t *ctx);

int pcilib_start(pcilib_t *ctx, pcilib_event_t event_mask, pcilib_event_flags_t flags);
int pcilib_stop(pcilib_t *ctx, pcilib_event_flags_t flags);

int pcilib_stream(pcilib_t *ctx, pcilib_event_callback_t callback, void *user);
int pcilib_get_next_event(pcilib_t *ctx, pcilib_timeout_t timeout, pcilib_event_id_t *evid, size_t info_size, pcilib_event_info_t *info);

int pcilib_copy_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t size, void *buf, size_t *retsize);
int pcilib_copy_data_with_argument(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t size, void *buf, size_t *retsize);
void *pcilib_get_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t *size_or_err);
void *pcilib_get_data_with_argument(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size_or_err);

/*
 * This function is provided to find potentially corrupted data. If the data is overwritten by 
 * the time return_data is called it will return error. 
 */
int pcilib_return_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, void *data);

/*
 * @param data - will be allocated and shuld be freed if NULL, otherwise used and size should contain correct size.
 *   In case of failure the content of data is undefined.
 * @param timeout - will be autotriggered if NULL
 */
int pcilib_grab(pcilib_t *ctx, pcilib_event_t event_mask, size_t *size, void **data, pcilib_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* _PCITOOL_PCILIB_H */
