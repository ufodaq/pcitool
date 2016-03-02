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
typedef uint64_t pcilib_register_value_t;	/**< Type holding the register value */
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
    PCILIB_ACCESS_RW = 3,
    PCILIB_ACCESS_INCONSISTENT = 0x10000	/**< inconsistent access, one will not read that one has written */
} pcilib_access_mode_t;

typedef enum {
    PCILIB_REGISTER_R = 1,			/**< reading from register is allowed */
    PCILIB_REGISTER_W = 2,			/**< normal writting to register is allowed */
    PCILIB_REGISTER_RW = 3,
    PCILIB_REGISTER_W1C = 4,			/**< writting 1 resets the bit, writting 0 keeps the value */
    PCILIB_REGISTER_RW1C = 5,
    PCILIB_REGISTER_W1I = 8,			/**< writting 1 inversts the bit, writting 0 keeps the value */
    PCILIB_REGISTER_RW1I = 9,
    PCILIB_REGISTER_INCONSISTENT = 0x10000	/**< inconsistent register, writting and reading does not match */
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
    PCILIB_DMA_FLAG_IGNORE_ERRORS = 16,		/**< do not crash on errors, but return appropriate error codes */
    PCILIB_DMA_FLAG_STOP = 32			/**< indicates that we actually calling pcilib_dma_start to stop persistent DMA engine */
} pcilib_dma_flags_t;

typedef enum {
    PCILIB_STREAMING_STOP = 0,                  /**< stop streaming */
    PCILIB_STREAMING_CONTINUE = 1,              /**< wait DMA timeout and return gracefuly if no new data appeared */
    PCILIB_STREAMING_WAIT = 2,                  /**< wait the specified timeout and return gracefuly if no new data appeared */
    PCILIB_STREAMING_CHECK = 3,                 /**< check if more data is available without waiting, return gracefuly if no data is ready */
    PCILIB_STREAMING_TIMEOUT_MASK = 3,          /**< mask specifying all timeout modes */
    PCILIB_STREAMING_FAIL = 4,                  /**< a flag indicating that the error should be generated if no data is available upon the timeout (whatever timeout mode is used) */
    PCILIB_STREAMING_REQ_FRAGMENT = 5,          /**< only fragment of a packet is read, wait for next fragment and fail if no data during DMA timeout */
    PCILIB_STREAMING_REQ_PACKET = 6             /**< wait for next packet and fail if no data during the specified timeout */
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
#define PCILIB_REGISTER_ADDRESS_INVALID	((pcilib_register_addr_t)-1)
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

/**
 * Callback function called when pcilib wants to log a new message
 * @param[in,out] arg 	- logging context provided with pcilib_set_logger() call
 * @param[in] file	- source file generating the message
 * @param[in] line	- source line generating the message
 * @param[in] prio	- the message priority (messages with priority bellow the currently set loglevel are already filtered)
 * @param[in] msg 	- message or printf-style formating string
 * @param[in] va	- vairable parameters defined by formating string
 */
typedef void (*pcilib_logger_t)(void *arg, const char *file, int line, pcilib_log_priority_t prio, const char *msg, va_list va);

/**
 * Callback function called when new data is read by DMA streaming function
 * @param[in,out] ctx 	- DMA Engine context
 * @param[in] flags 	- DMA Flags
 * @param[in] bufsize 	- size of data in bytes
 * @param[in] buf 	- data
 * @return 		- #pcilib_streaming_action_t instructing how the streaming should continue or a negative error code in the case of error
 */
typedef int (*pcilib_dma_callback_t)(void *ctx, pcilib_dma_flags_t flags, size_t bufsize, void *buf); 

/**
 * Callback function called when new event is generated by event engine
 * @param[in] event_id	- event id
 * @param[in] info	- event description (depending on event engine may provide additional fields on top of #pcilib_event_info_t)
 * @param[in,out] user	- User-specific data provided in pcilib_stream() call
 * @return 		- #pcilib_streaming_action_t instructing how the streaming should continue or a negative error code in the case of error
 */
typedef int (*pcilib_event_callback_t)(pcilib_event_id_t event_id, const pcilib_event_info_t *info, void *user);

/**
 * Callback function called when new portion of raw data is read by event engine
 * @param[in] event_id	- id of the event this data will be associated with if processing is successful
 * @param[in] info	- event description (depending on event engine may provide additional fields on top of #pcilib_event_info_t)
 * @param[in] flags	- may indicate if it is the last portion of data for current event 
 * @param[in] size	- size of data in bytes
 * @param[in,out] data	- data (the callback may modify the data, but the size should be kept of course)
 * @param[in,out] user	- User-specific data provided in pcilib_stream() call
 * @return 		- #pcilib_streaming_action_t instructing how the streaming should continue or a negative error code in the case of error
 */
typedef int (*pcilib_event_rawdata_callback_t)(pcilib_event_id_t event_id, const pcilib_event_info_t *info, pcilib_event_flags_t flags, size_t size, void *data, void *user);

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************//**
 * \defgroup public_api_global Global Public API (logging, etc.)
 * Global functions which does not require existing pcilib context
 * @{
 */

/**
 * Replaces default logging function.
 * @param min_prio 	- messages with priority below \a min_prio will be ignored
 * @param logger	- configures a new logging function or restores the default one if NULL is passed
 * @param arg		- logging context, this parameter will be passed through to the \a logger
 * @return 		- error code or 0 on success
 */
int pcilib_set_logger(pcilib_log_priority_t min_prio, pcilib_logger_t logger, void *arg);

/** public_api_global
 * @}
 */


/***********************************************************************************************************//**
 * \defgroup public_api Public API
 * Base pcilib functions which does not belong to the specific APIs
 * @{
 */

/**
 * Initializes pcilib context, detects model configuration, and populates model-specific registers.
 * Event and DMA engines will not be started automatically, but calls to pcilib_start() / pcilib_start_dma()
 * are provided for this purpose. In the end, the context should be cleaned using pcilib_stop().
 * @param[in] device	- path to the device file [/dev/fpga0]
 * @param[in] model	- specifies the model of hardware, autodetected if NULL is passed
 * @return 		- initialized context or NULL in the case of error
 */
pcilib_t *pcilib_open(const char *device, const char *model);

/**
 * Destroy pcilib context and all memory associated with it. The function will stop event engine if necessary,
 * but DMA engine stay running if it was already running before pcilib_open() was called
 * @param[in,out] ctx 	- pcilib context
 */
void pcilib_close(pcilib_t *ctx);

/**
 * Should reset the hardware and software in default state. It is implementation-specific, but is not actively
 * used in existing implementations because the hardware is initialized from bash scripts which are often 
 * changed by Michele and his band. On the software side, it probably should reset all software registers
 * to default value and may be additionaly re-start and clear DMA. However, this is not implemented yet.
 * @param[in,out] ctx	- pcilib context
 * @return		- error code or 0 on success
 */ 
 
int pcilib_reset(pcilib_t *ctx);

/** public_api
 * @}
 */


/***********************************************************************************************************//**
 * \defgroup public_api_pci Public PCI API (MMIO)
 * API for manipulation with memory-mapped PCI BAR space
 * @{
 */

/**
 * Resolves the phisycal PCI address to a virtual address in the process address space.
 * The required BAR will be mapped if necessary.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bar	- specifies the BAR address belong to, use PCILIB_BAR_DETECT for autodetection
 * @param[in] addr	- specifies the physical address on the PCI bus or offset in the BAR if \a bar is specified
 * @return		- the virtual address in the process address space or NULL in case of error
 */
char *pcilib_resolve_bar_address(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr);	

/**
 * Performs PIO read from the PCI BAR. The BAR will be automatically mapped and unmapped if necessary.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bar	- the BAR to read, use PCILIB_BAR_DETECT to detect bar by the specified physical address
 * @param[in] addr	- absolute physical address to read or the offset in the specified bar
 * @param[in] access	- word size (access width in bytes)
 * @param[in] n		- number of words to read
 * @param[out] buf	- the read data will be placed in this buffer
 * @return 		- error code or 0 on success
 */ 
int pcilib_read(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t access, size_t n, void *buf);

/**
 * Performs PIO write to the PCI BAR. The BAR will be automatically mapped and unmapped if necessary.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bar	- the BAR to write, use PCILIB_BAR_DETECT to detect bar by the specified physical address
 * @param[in] addr	- absolute physical address to write or the offset in the specified bar
 * @param[in] access	- word size (access width in bytes)
 * @param[in] n		- number of words to write
 * @param[out] buf	- the pointer to the data to be written
 * @return 		- error code or 0 on success
 */ 
int pcilib_write(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t access, size_t n, void *buf);

/**
 * Performs PIO read from the PCI BAR. The specified address is treated as FIFO and will be read
 * \a n times. The BAR will be automatically mapped and unmapped if necessary.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bar	- the BAR to read, use PCILIB_BAR_DETECT to detect bar by the specified physical address
 * @param[in] addr	- absolute physical address to read or the offset in the specified bar
 * @param[in] access	- the size of FIFO register in bytes (i.e. if `access = 4`, the 32-bit reads from FIFO will be performed)
 * @param[in] n		- specifies how many times the data should be read from FIFO
 * @param[out] buf	- the data will be placed in this buffer which should be at least `n * access` bytes long
 * @return 		- error code or 0 on success
 */ 
int pcilib_read_fifo(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t access, size_t n, void *buf);

/**
 * Performs PIO write to the PCI BAR. The specified address is treated as FIFO and will be written
 * \a n times. The BAR will be automatically mapped and unmapped if necessary.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bar	- the BAR to write, use PCILIB_BAR_DETECT to detect bar by the specified physical address
 * @param[in] addr	- absolute physical address to write or the offset in the specified bar
 * @param[in] access	- the size of FIFO register in bytes (i.e. if `access = 4`, the 32-bit writes to FIFO will be performed)
 * @param[in] n		- specifies how many times the data should be written to FIFO
 * @param[out] buf	- buffer to write which should be at least `n * access` bytes long
 * @return 		- error code or 0 on success
 */ 
int pcilib_write_fifo(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t access, size_t n, void *buf);

/** public_api_pci
 * @}
 */


/***********************************************************************************************************//**
 * \defgroup public_api_register Public Register API 
 * API for register manipulations
 * @{
 */

/**
 * Returns the list of registers provided by the hardware model.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- if set, only register within the specified bank will be returned
 * @param[in] flags	- currently ignored
 * @return 		- the list of the register which should be cleaned with pcilib_free_register_info() or NULL in the case of error
 */
pcilib_register_info_t *pcilib_get_register_list(pcilib_t *ctx, const char *bank, pcilib_list_flags_t flags);

/**
 * Returns the information about the specified register
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- indicates the bank where to look for register, autodetected if NULL is passed
 * @param[in] reg	- the name of the register
 * @param[in] flags	- currently ignored
 * @return 		- information about the specified register which should be cleaned with pcilib_free_register_info() or NULL in the case of error
 */
pcilib_register_info_t *pcilib_get_register_info(pcilib_t *ctx, const char *bank, const char *reg, pcilib_list_flags_t flags);

/**
 * Cleans up the memory occupied by register list returned from the pcilib_get_register_list() and pcilib_get_register_info() calls
 * @param[in,out] ctx	- pcilib context
 * @param[in,out] info	- buffer to clean
 */
void pcilib_free_register_info(pcilib_t *ctx, pcilib_register_info_t *info);

/**
 * Finds register id corresponding to the specified bank and register names. It is faster 
 * to access registers by id instead of names. Therefore, in long running applications it 
 * is preferred to resolve names of all required registers during initialization and access 
 * them using ids only. 
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * @param[in] reg	- the name of the register
 * @return 		- register id or PCILIB_REGISTER_INVALID if register is not found
 */
pcilib_register_t pcilib_find_register(pcilib_t *ctx, const char *bank, const char *reg);

/**
 * Extracts additional information about the specified register. The additional information
 * is model-specific and are provided as extra XML attributes in XML-described registers.
 * The available attributes are only restricted by used XSD schema.
 * @param[in,out] ctx	- pcilib context
 * @param[in] reg	- register id
 * @param[in] attr	- requested attribute name
 * @param[in,out] val	- the value of attribute is returned here (see \ref public_api_value),
 *			pcilib_clean_value() will be executed if \a val contains data. Therefore it should be always initialized to 0 before first use
 * @return		- error code or 0 on success
 */ 
int pcilib_get_register_attr_by_id(pcilib_t *ctx, pcilib_register_t reg, const char *attr, pcilib_value_t *val);

/**
 * Extracts additional information about the specified register. 
 * Equivalent to the pcilib_get_register_attr_by_id(), but first resolves register id using the specified bank and name.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * @param[in] regname	- register name
 * @param[in] attr	- requested attribute name
 * @param[in,out] val	- the value of attribute is returned here (see \ref public_api_value),
 *			pcilib_clean_value() will be executed if \a val contains data. Therefore it should be always initialized to 0 before first use
 * @return		- error code or 0 on success
 */ 
int pcilib_get_register_attr(pcilib_t *ctx, const char *bank, const char *regname, const char *attr, pcilib_value_t *val);

/**
 * Reads one or multiple sequential registers from the specified register bank. This function may provide access 
 * to the undefined registers in the bank. In other cases, the pcilib_read_register() / pcilib_read_register_by_id() 
 * calls are preferred.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- the bank to read (should be specified, no autodetection)
 * @param[in] addr	- the register address within the bank, addresses are counted in bytes not registers (i.e. it is offset in bytes from the start of register bank)
 * @param[in] n		- number of registers to read; i.e. `n * register_size`  bytes will be read, where \a register_size is defined in the bank configuration (see #pcilib_register_bank_description_t)
 * @param[out] buf	- the buffer of `n * register_size` bytes long where the data will be stored
 * @return		- error code or 0 on success
 */
int pcilib_read_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf);

/**
 * Writes one or multiple sequential registers from the specified register bank. This function may provide access 
 * to the undefined registers in the bank. In other cases, the pcilib_write_register() / pcilib_write_register_by_id() 
 * calls are preferred.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- the bank to write (should be specified, no autodetection)
 * @param[in] addr	- the register address within the bank, addresses are counted in bytes not registers (i.e. it is offset in bytes from the start of register bank)
 * @param[in] n		- number of registers to write; i.e. `n * register_size`  bytes will be written, where \a register_size is defined in the bank configuration (see #pcilib_register_bank_description_t)
 * @param[in] buf	- the buffer of `n * register_size` bytes long with the data
 * @return		- error code or 0 on success
 */
int pcilib_write_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, const pcilib_register_value_t *buf);

/**
 * Reads the specified register.
 * @param[in,out] ctx	- pcilib context
 * @param[in] reg	- register id
 * @param[out] value	- the register value is returned here
 * @return		- error code or 0 on success
 */ 
int pcilib_read_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t *value);

/**
 * Writes to the specified register.
 * @param[in,out] ctx	- pcilib context
 * @param[in] reg	- register id
 * @param[in] value	- the register value to write
 * @return		- error code or 0 on success
 */ 
int pcilib_write_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t value);

/**
 * Reads the specified register.
 * Equivalent to the pcilib_read_register_by_id(), but first resolves register id using the specified bank and name.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * @param[in] regname	- the name of the register
 * @param[out] value	- the register value is returned here
 * @return		- error code or 0 on success
 */ 
int pcilib_read_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t *value);

/**
 * Writes to the specified register.
 * Equivalent to the pcilib_write_register_by_id(), but first resolves register id using the specified bank and name.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * @param[in] regname	- the name of the register
 * @param[in] value	- the register value to write
 * @return		- error code or 0 on success
 */ 
int pcilib_write_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t value);


/**
 * Reads a view of the specified register. The views allow to convert values to standard units
 * or get self-explanatory names associated with the values (enums).
 * @param[in,out] ctx	- pcilib context
 * @param[in] reg	- register id
 * @param[in] view	- specifies the name of the view associated with register or desired units
 * @param[out] value	- the register value is returned here (see \ref public_api_value),
 *			pcilib_clean_value() will be executed if \a val contains data. Therefore it should be always initialized to 0 before first use.
 * @return		- error code or 0 on success
 */ 
int pcilib_read_register_view_by_id(pcilib_t *ctx, pcilib_register_t reg, const char *view, pcilib_value_t *value);

/**
 * Writes the specified register using value represented in the specified view. The views allow to convert values from standard units
 * or self-explanatory names associated with the values (enums).
 * @param[in,out] ctx	- pcilib context
 * @param[in] reg	- register id
 * @param[in] view	- specifies the name of the view associated with register or the used units
 * @param[in] value	- the register value in the specified view (see \ref public_api_value)
 * @return		- error code or 0 on success
 */ 
int pcilib_write_register_view_by_id(pcilib_t *ctx, pcilib_register_t reg, const char *view, const pcilib_value_t *value);

/**
 * Reads a view of the specified register. The views allow to convert values to standard units
 * or get self-explanatory names associated with the values (enums).
 * Equivalent to the pcilib_read_register_view_by_id(), but first resolves register id using the specified bank and name.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * @param[in] regname	- the name of the register
 * @param[in] view	- specifies the name of the view associated with register or desired units
 * @param[out] value	- the register value is returned here (see \ref public_api_value),
 *			pcilib_clean_value() will be executed if \a val contains data. Therefore it should be always initialized to 0 before first use
 * @return		- error code or 0 on success
 */ 
int pcilib_read_register_view(pcilib_t *ctx, const char *bank, const char *regname, const char *view, pcilib_value_t *value);


/**
 * Writes the specified register using value represented in the specified view. The views allow to convert values from standard units
 * or self-explanatory names associated with the values (enums).
 * Equivalent to the pcilib_write_register_view_by_id(), but first resolves register id using the specified bank and name.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * @param[in] regname	- the name of the register
 * @param[in] view	- specifies the name of the view associated with register or the used units
 * @param[in] value	- the register value in the specified view (see \ref public_api_value)
 * @return		- error code or 0 on success
 */ 
int pcilib_write_register_view(pcilib_t *ctx, const char *bank, const char *regname, const char *view, const pcilib_value_t *value);

/** public_api_register
 * @}
 */

/***********************************************************************************************************//**
 * \defgroup public_api_property Public Property API 
 * Properties is another abstraction on top of registers allowing arbitrary data types and computed registers
 * @{
 */

/**
 * Returns the list of properties available under the specified path
 * @param[in,out] ctx	- pcilib context
 * @param[in] branch	- path or NULL to return the top-level properties
 * @param[in] flags	- not used at the moment
 * @return 		- the list of the properties which should be cleaned with pcilib_free_property_info() or NULL in the case of error
 */
pcilib_property_info_t *pcilib_get_property_list(pcilib_t *ctx, const char *branch, pcilib_list_flags_t flags);

/**
 * Cleans up the memory occupied by property list returned from the pcilib_get_property_list() call
 * @param[in,out] ctx	- pcilib context
 * @param[in,out] info	- buffer to clean
 */
void pcilib_free_property_info(pcilib_t *ctx, pcilib_property_info_t *info);

/**
 * Extracts additional information about the specified register. 
 * Equivalent to the pcilib_get_register_attr_by_id(), but first resolves register id using the specified bank and name.
 * @param[in,out] ctx	- pcilib context
 * @param[in] prop	- property name (full name including path)
 * @param[in] attr	- requested attribute name
 * @param[in,out] val	- the value of attribute is returned here (see \ref public_api_value),
 *			pcilib_clean_value() will be executed if \a val contains data. Therefore it should be always initialized to 0 before first use
 * @return		- error code or 0 on success
 */ 
int pcilib_get_property_attr(pcilib_t *ctx, const char *prop, const char *attr, pcilib_value_t *val);

/**
 * Reads / computes the property value.
 * @param[in,out] ctx	- pcilib context
 * @param[in] prop	- property name (full name including path)
 * @param[out] val	- the register value is returned here (see \ref public_api_value),
 *			pcilib_clean_value() will be executed if \a val contains data. Therefore it should be always initialized to 0 before first use
 * @return		- error code or 0 on success
 */ 
int pcilib_get_property(pcilib_t *ctx, const char *prop, pcilib_value_t *val);

/**
 * Writes the property value or executes the code associated with property
 * @param[in,out] ctx	- pcilib context
 * @param[in] prop	- property name (full name including path)
 * @param[out] val	- the property value (see \ref public_api_value),
 * @return		- error code or 0 on success
 */ 
int pcilib_set_property(pcilib_t *ctx, const char *prop, const pcilib_value_t *val);

/** public_api_property
 * @}
 */

/***********************************************************************************************************//**
 * \defgroup public_api_dma Public DMA API
 * High speed interface for reading and writting unstructured data
 * @{
 */

/**
 * This function resolves the ID of DMA engine from its address and direction.
 * It is a bit confusing, but addresses should be able to represent bi-directional
 * DMA engines. Unfortunatelly, implementation often is limited to uni-directional
 * engines. In this case, two DMA engines with different IDs can be virtually 
 * combined in a DMA engine with the uniq address. This will allow user to specify
 * the same engine address in all types of accesses and we will resolve the appropriate 
 * engine ID depending on the requested direction.
 * @param[in,out] ctx	- pcilib context
 * @param[in] direction	- DMA direction (to/from device)
 * @param[in] dma	- address of DMA engine
 * @return		- ID of DMA engine or PCILIB_DMA_INVALID if the specified engine does not exist
 */
pcilib_dma_engine_t pcilib_find_dma_by_addr(pcilib_t *ctx, pcilib_dma_direction_t direction, pcilib_dma_engine_addr_t dma);

/**
 * Starts DMA engine. This call will allocate DMA buffers and pass their bus addresses to the hardware.
 *  - During the call, the C2S engine may start writting data. The written buffers are marked as
 * ready and can be read-out using pcilib_stream_dma() and pcilib_read_dma() calls. If no empty
 * buffers left, the C2S DMA engine will stall until some buffers are read out. 
 *  - The S2C engine waits until the data is pushed with pcilib_push_data() call
 *
 * After pcilib_start_dma() call, the pcilib_stop_dma() function should be necessarily called. However,
 * it will clean up the process memory, but only in some cases actually stop the DMA engine.
 * This depends on \a flags passed to both pcilib_start_dma() and pcilib_stop_dma() calls.
 * if PCILIB_DMA_FLAG_PERSISTENT flag is passed to the pcilib_start_dma(), the DMA engine
 * will remain running unless the same flag is also passed to the pcilib_stop_dma() call.
 * The allocated DMA buffers will stay intact and the hardware may continue reading / writting
 * data while there is space/data left. However, the other functions of DMA API should not
 * be called after pcilib_stop_dma() until new pcilib_start_dma() call is issued.
 *
 * The process- and thread-safety is implementation depedent. However, currently the process-
 * safety is ensured while accessing the kernel memory (todo: this may get complicated if we 
 * provide a way to perform DMA directly to the user-supplied pages). 
 * The thread-safety is not provided by currently implemented DMA engines. The DMA functions
 * may be called from multiple threads, but it is user responsibility to ensure that only a 
 * single DMA-related call is running. On other hand, the DMA and register APIs may be used
 * in parallel.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] dma	- ID of DMA engine, the ID should first be resolved using pcilib_find_dma_by_addr()
 * @param[in] flags	- PCILIB_DMA_FLAG_PERSISTENT indicates that engine should be kept running after pcilib_stop_dma() call
 * @return 		- error code or 0 on success
 */
int pcilib_start_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);

/**
 * Stops DMA engine or just cleans up the process-specific memory buffers (see 
 * pcilib_start_dma() for details). No DMA API calls allowed after this point
 * until pcilib_start_dma() is called anew.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] dma	- ID of DMA engine, the ID should first be resolved using pcilib_find_dma_by_addr()
 * @param[in] flags	- PCILIB_DMA_FLAG_PERSISTENT indicates that engine should be actually stopped independent of the flags passed to pcilib_start_dma() call
 * @return 		- error code or 0 on success
 */ 
int pcilib_stop_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);

/**
 * Tries to drop all pending data in the DMA channel. It will drop not only data currently in the DMA buffers,
 * but will allow hardware to write more and will drop the newly written data as well. The standard DMA timeout 
 * is allowed to receive new data. If hardware continuously writes data, after #PCILIB_DMA_SKIP_TIMEOUT 
 * microseconds the function will exit with #PCILIB_ERROR_TIMEOUT error. 
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] dma	- ID of DMA engine, the ID should first be resolved using pcilib_find_dma_by_addr()
 * @return 		- error code or 0 on success
 */
int pcilib_skip_dma(pcilib_t *ctx, pcilib_dma_engine_t dma);

/**
 * Reads data from DMA buffers and pass it to the specified callback function. The return code of the callback
 * function determines if streaming should continue and how much to wait for next data to arrive before 
 * triggering timeout. 
 * The function is process- and thread-safe. The PCILIB_ERROR_BUSY will be returned immediately if DMA is used 
 * by another thread or process.
 *
 * The function waits the specified \a timeout microseconds for the first data. Afterwards, the waiting time
 * for next portion of data depends on the last return code \a (ret) from callback function. 
 * If `ret & PCILIB_STREAMING_TIMEOUT_MASK` is
 *   - PCILIB_STREAMING_STOP		- the streaming stops
 *   - PCILIB_STREAMING_CONTINUE	- the standard DMA timeout will be used to wait for a new data
 *   - PCILIB_STREAMING_WAIT		- the timeout specified in the function arguments will be used to wait for a new data
 *   - PCILIB_STREAMING_CHECK		- the function will return if new data is not available immediately
 * The function return code depends on the return code from the callback as well. If no data received within the specified 
 * timeout and no callback is called, the PCILIB_ERROR_TIMEOUT is returned. Otherwise, success is returned unless 
 * PCILIB_STREAMING_FAIL flag has been set in the callback return code before the timeout was triggered.
 * 
 * @param[in,out] ctx	- pcilib context
 * @param[in] dma	- ID of DMA engine, the ID should first be resolved using pcilib_find_dma_by_addr()
 * @param[in] addr	- instructs DMA to start reading at the specified address (not supported by existing DMA engines)
 * @param[in] size	- instructs DMA to read only \a size bytes (not supported by existing DMA engines)
 * @param[in] flags	- not used by existing DMA engines
 * @param[in] timeout	- specifies number of microseconds to wait before reporting timeout, special values #PCILIB_TIMEOUT_IMMEDIATE and #PCILIB_TIMEOUT_INFINITE are supported.
 * @param[in] cb	- callback function to call
 * @param[in,out] cbattr - passed through as the first parameter of callback function
 * @return 		- error code or 0 on success
 */
int pcilib_stream_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr);

/**
 * Reads data from DMA until timeout is hit, a full DMA packet is read,  or the specified number of bytes are read.
 * The function is process- and thread-safe. The #PCILIB_ERROR_BUSY will be returned immediately if DMA is used 
 * by another thread or process.
 *
 * We can't read arbitrary number of bytes from DMA. A full DMA packet is always read. The DMA packet is not equal 
 * to DMA buffer, but may consist of multiple buffers and the size may vary during run time. This may cause problems
 * if not treated properly. While the number of actually read bytes is bellow the specified size, the function may
 * continue to read the data. But as the new packet is started, it should fit completely in the provided buffer space
 * or PCILIB_ERROR_TOOBIG error will be returned. Therefore, it is a good practice to read only a single packet at 
 * once and provide buffer capable to store the larges possible packet.
 *
 * Unless #PCILIB_DMA_FLAG_MULTIPACKET flag is specified, the function will stop after the first full packet is read.
 * Otherwise, the reading will continue until all `size` bytes are read or timeout is hit. The stanard DMA timeout
 * should be met while reading DMA buffers belonging to the same packet. Otherwise, PCILIB_ERROR_TIMEOUT is returned
 * and number of bytes read so far is returned in the `rdsize`.
 * If #PCILIB_DMA_FLAG_WAIT flag is specified, the number of microseconds specified in the `timeout` parameter are 
 * allowed for a new packet to come. If no new data arrived in the specified timeout, the function returns successfuly 
 * and number of read bytes is returned in the `rdsize`.
 *
 * We can't put the read data back into the DMA. Therefore, even in the case of error some data may be returned. The 
 * number of actually read bytes is always reported in `rdsize` and the specified amount of data is always written 
 * to the provided buffer.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] dma	- ID of DMA engine, the ID should first be resolved using pcilib_find_dma_by_addr()
 * @param[in] addr	- instructs DMA to start reading at the specified address (not supported by existing DMA engines)
 * @param[in] size	- specifies how many bytes should be read
 * @param[in] flags	- Various flags controlling the function behavior
 *			  - #PCILIB_DMA_FLAG_MULTIPACKET indicates that multiple DMA packets will be read (not recommended, use pcilib_stream_dma() in this case)
 *			  - #PCILIB_DMA_FLAG_WAIT indicates that we need to wait the specified timeout between consequitive DMA packets (default DMA timeout is used otherwise)
 * @param[in] timeout	- specifies number of microseconds to wait before reporting timeout, special values #PCILIB_TIMEOUT_IMMEDIATE and #PCILIB_TIMEOUT_INFINITE are supported.
 *			This parameter specifies timeout between consequtive DMA packets, the standard DMA timeout is expected between buffers belonging to the same DMA packet.
 * @param[out] buf	- the buffer of \a size bytes long to store the data
 * @param[out] rdsize	- number of bytes which were actually read. The correct value will be reported in both case if function has finished successfully or if error has happened.
 * @return 		- error code or 0 on success. In both cases some data may be returned in the buffer, check `rdsize`.
 */
int pcilib_read_dma_custom(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *rdsize);

/**
 * Reads data from DMA until timeout is hit, a full DMA packet is read, or the specified number of bytes are read.
 * Please, check detailed explanation when reading is stopped in the description of pcilib_read_dma_custom().
 * The function is process- and thread-safe. The #PCILIB_ERROR_BUSY will be returned immediately if DMA is used 
 * by another thread or process. 
 *
 * The function actually executes the pcilib_read_dma_custom() without special flags and with default DMA timeout
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] dma	- ID of DMA engine, the ID should first be resolved using pcilib_find_dma_by_addr()
 * @param[in] addr	- instructs DMA to start reading at the specified address (not supported by existing DMA engines)
 * @param[in] size	- specifies how many bytes should be read
 * @param[out] buf	- the buffer of \a size bytes long to store the data
 * @param[out] rdsize	- number of bytes which were actually read. The correct value will be reported in both case if function has finished successfully or if error has happened.
 * @return 		- error code or 0 on success. In both cases some data may be returned in the buffer, check `rdsize`.
 */
int pcilib_read_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf, size_t *rdsize);

/**
 * Pushes new data to the DMA engine. The actual behavior is implementation dependent. The successful exit does not mean
 * what all data have reached hardware, but only guarantees that it is stored in DMA buffers and the hardware is instructed
 * to start reading buffers. The function may return #PCILIB_ERROR_TIMEOUT is all DMA buffers are occupied and no buffers is
 * read by the hardware within the specified timeout. Even if an error is returned, part of the data may be already send to
 * DMA and can't be revoked back. Number of actually written bytes is always returned in the `wrsize`.
 *
 * The function is process- and thread-safe. The #PCILIB_ERROR_BUSY will be returned immediately if DMA is used 
 * by another thread or process.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] dma	- ID of DMA engine, the ID should first be resolved using pcilib_find_dma_by_addr()
 * @param[in] addr	- instructs DMA to start writting at the specified address (not supported by existing DMA engines)
 * @param[in] size	- specifies how many bytes should be written
 * @param[in] flags	- Various flags controlling the function behavior
 *			  - #PCILIB_DMA_FLAG_EOP indicates that this is the last data in the DMA packet
 *			  - #PCILIB_DMA_FLAG_WAIT requires function to block until the data actually reach hardware. #PCILIB_ERROR_TIMEOUT may be returned if it takes longer when the specified timeout.
  * @param[in] timeout	- specifies number of microseconds to wait before reporting timeout, special values #PCILIB_TIMEOUT_IMMEDIATE and #PCILIB_TIMEOUT_INFINITE are supported.
 * @param[out] buf	- the buffer with the data
 * @param[out] wrsize	- number of bytes which were actually written. The correct value will be reported in both case if function has finished successfully or if error has happened.
 * @return 		- error code or 0 on success. In both cases some data may be written to the DMA, check `wrsize`.
 */
int pcilib_push_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *wrsize);


/**
 * Pushes new data to the DMA engine and blocks until hardware gets it all.  Even if an error has occured, a 
 * part of the data may be already had send to DMA and can't be revoked back. Number of actually written bytes 
 * is always returned in the `wrsize`.
 *
 * The function is process- and thread-safe. The #PCILIB_ERROR_BUSY will be returned immediately if DMA is used 
 * by another thread or process.

 * The function actually executes the pcilib_push_dma() with #PCILIB_DMA_FLAG_EOP and #PCILIB_DMA_FLAG_WAIT flags set and
 * the default DMA timeout.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] dma	- ID of DMA engine, the ID should first be resolved using pcilib_find_dma_by_addr()
 * @param[in] addr	- instructs DMA to start writting at the specified address (not supported by existing DMA engines)
 * @param[in] size	- specifies how many bytes should be written
 * @param[out] buf	- the buffer with the data
 * @param[out] wrsize	- number of bytes which were actually written. The correct value will be reported in both case if function has finished successfully or if error has happened.
 * @return 		- error code or 0 on success. In both cases some data may be written to the DMA, check `wrsize`.
 */
int pcilib_write_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf, size_t *wrsize);

/**
 * Benchmarks the DMA implementation. The reported performance may be significantly affected by several environmental variables.
 *  - PCILIB_BENCHMARK_HARDWARE	 - if set will not copy the data out, but immediately drop as it lended in DMA buffers. This allows to remove influence of memcpy performance.
 *  - PCILIB_BENCHMARK_STREAMING - if set will not re-initialized the DMA on each iteration. If DMA is properly implemented this should have only marginal influence on performance
 *
 * The function is process- and thread-safe. The #PCILIB_ERROR_BUSY will be returned immediately if DMA is used 
 * by another thread or process.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] dma	- Address of DMA engine
 * @param[in] addr	- Benchmark will read and write data at the specified address (ignored by existing DMA engines)
 * @param[in] size	- specifies how many bytes should be read and written at each iteration
 * @param[in] iterations - number of iterations to execute
 * @param[in] direction - Specifies if DMA reading or writting should be benchmarked, bi-directional benchmark is possible as well
 * @return		- bandwidth in MiB/s (Mebibytes per second)
 */
double pcilib_benchmark_dma(pcilib_t *ctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);

/** public_api_dma
 * @}
 */

/***********************************************************************************************************//**
 * \defgroup public_api_irq Public IRQ API
 * This is actually part of DMA API. IRQ handling is currently provided by DMA engine. 
 * However, the IRQs are barely used so far. Therefore, the API can be significantly changed when confronted with harsh reallity.
 * @{
 */

int pcilib_enable_irq(pcilib_t *ctx, pcilib_irq_type_t irq_type, pcilib_dma_flags_t flags);
int pcilib_disable_irq(pcilib_t *ctx, pcilib_dma_flags_t flags);
int pcilib_wait_irq(pcilib_t *ctx, pcilib_irq_hw_source_t source, pcilib_timeout_t timeout, size_t *count);
int pcilib_acknowledge_irq(pcilib_t *ctx, pcilib_irq_type_t irq_type, pcilib_irq_source_t irq_source);
int pcilib_clear_irq(pcilib_t *ctx, pcilib_irq_hw_source_t source);

/** public_api_irq
 * @}
 */

/***********************************************************************************************************//**
 * \defgroup public_api_event Public Event API
 * High level API for reading the structured data from hardware
 * @{
 */

/**
 * Provides access to the context of event engine. This may be required to call implementation-specific
 * functions of event engine.
 * @param [in,out] ctx	- pcilib context
 * @return 		- context of event engine or NULL in the case of error. Depending on the implementation can be extension of pcilib_contex_t, it should be safe to type cast if you are running the correct engine.
 */
pcilib_context_t *pcilib_get_event_engine(pcilib_t *ctx);

/** 
 * Resolves the event ID based on its name.
 * @param [in,out] ctx	- pcilib context
 * @param [in] event	- the event name
 * @return		- the event ID or PCILIB_EVENT_INVALID if event is not found
 */
pcilib_event_t pcilib_find_event(pcilib_t *ctx, const char *event);


/**
 * Analyzes current configuration, allocates necessary buffers and spawns required data processing threads. 
 * Depending on the implementation and the current configuration, the actual event grabbing may start already
 * here. In this case, the preprocessed events will be storred in the temporary buffers and may be lost if
 * pcilib_stop() is called before reading them out. Alternatively, the actual grabbing may only commend when
 * the pcilib_stream() or pcilib_get_next_event() functions are executed. 
 * The function is non-blocking and will return immediately after allocating required memory and spawning 
 * of the preprocessing threads. It is important to call pcilib_stop() in the end.
 *
 * The grabbing will stop automatically if conditions defined using pcilib_configure_autostop() function
 * are met. It also possible to stop grabbing using pcilib_stop() call at any moment.
 *
 * The process- and thread-safety is implementation depedent. However, normally the event engine will depend
 * on DMA and if DMA engine is process-safe it will ensure the process-safety for event engine as well.
 * The thread-safety is not directly ensured by currently implemented Event engines. The functions of Event
 * engine may be called from multiple threads, but it is the user responsibility to ensure that only a single
 * function of Event engine is running at each moment. On other hand, the Event and register APIs may be used
 * in parallel.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] ev_mask	- specifies events to listen for, use #PCILIB_EVENTS_ALL to grab all events
 * @param[in] flags	- various implementation-specific flags controlling operation of event engine
 *			   - #PCILIB_EVENT_FLAG_PREPROCESS - requires event preprocessing (i.e. event data is decoded before it is requested, this is often required for multi-threaded processing)
 *			   - #PCILIB_EVENT_FLAG_RAW_DATA_ONLY - disables data processing at all, only raw data will be provided in this case
 * @return 		- error code or 0 on success
 */
int pcilib_start(pcilib_t *ctx, pcilib_event_t ev_mask, pcilib_event_flags_t flags);
 

/**
 * Stops event grabbing and optionally cleans up the used memory.
 * This function operates in two modes. If #PCILIB_EVENT_FLAG_STOP_ONLY flag is specified, the 
 * event grabbing is stopped, but all memory structures are kept intact. This also forces the
 * blocked pcilib_stream() and pcilib_get_next_event() to return after a short while. 

 * Unlike DMA engine, the event engine is not persistent and is always completely stopped when 
 * application is finished. Therefore, later the pcilib_stop() should be necessarily called 
 * again without #PCILIB_EVENT_FLAG_STOP_ONLY flag to commend full clean up and release all
 * used memory. Such call may only be issued when no threads are using Event engine any more. 
 * There should be no functions waiting for next event to appear and all of the obtained event 
 * data should be already returned back to the system.
 *
 * pcilib_stop executed with #PCILIB_EVENT_FLAG_STOP_ONLY is thread safe. The full version of
 * the function is not and should be never called in parallel with any other action of the
 * event engine. Actually, it is thread safe in the case of ipecamera, but this is not 
 * guaranteed for other event engines.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] flags	- flags specifying operation mode
 *			   - #PCILIB_EVENT_FLAG_STOP_ONLY - instructs library to keep all the data structures intact and to onlystop the data grabbing
 * @return		- error code or 0 on success
 */
int pcilib_stop(pcilib_t *ctx, pcilib_event_flags_t flags);

/**
 * Streams the incomming events to the provided callback function. If Event engine is not started
 * yet, it will be started and terminated upon the completion of the call. The streaming will 
 * continue while Event engine is started and the callback function does not return an error (negative)
 * or #PCILIB_STREAMING_STOP. 
 *
 * The callback is called only when all the data associated with the event is received from hardware. 
 * So, the raw data is necessarily present, but availability of alternative data formats is
 * optional. Depending on the implementation and current configuration, the data decoding can be 
 * performed beforehand, in parallel with callback execution, or only them the data is 
 * requested with pcilib_get_data() or pcilib_copy_data() calls.
 *
 * The function is thread-safe. The multiple requests to pcilib_stream() and pcilib_get_next_event() 
 * will be automatically serialized by the event engine. The pcilib_stream() is running in the 
 * single thread and no new calls to callback are issued until currently executed callback 
 * returns. The client application may get hold on the data from multiple events simultaneously. 
 * However, the data could be overwritten meanwhile by the hardware. The pcilib_return_data() 
 * will inform if it has happened by returning #PCILIB_ERROR_OVERWRITTEN. 
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] callback	- callback function to call for each event, the streaming is stopped if it return #PCILIB_STREAMING_STOP or negative value indicating the error
 * @param[in,out] user	- used data is passed through as the last parameter of callback function
 * @return		- error code or 0 on success
 */
int pcilib_stream(pcilib_t *ctx, pcilib_event_callback_t callback, void *user);

/**
 * Waits until the next event is available. 
 * The event is only returned when all the data associated with the event is received from hardware.
 * So, the raw data is necessarily present, but availability of alternative data formats is
 * optional. Depending on the implementation and current configuration, the data decoding can be 
 * performed beforehand, in parallel, or only them the data is requested with pcilib_get_data() 
 * or pcilib_copy_data() calls.
 *
 * The function is thread-safe. The multiple requests to pcilib_stream() and pcilib_get_next_event() 
 * will be automatically serialized by the event engine. The client application may get hold on 
 * the data from multiple events simultaneously. However, the data could be overwritten meanwhile 
 * by the hardware. The pcilib_return_data() will inform if it has happened by returning 
 * #PCILIB_ERROR_OVERWRITTEN. 
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] timeout	- specifies number of microseconds to wait for next event before reporting timeout, special values #PCILIB_TIMEOUT_IMMEDIATE and #PCILIB_TIMEOUT_INFINITE are supported.
 * @param[out] evid	- the event ID is returned in this parameter
 * @param[in] info_size	- the size of the passed event info structure (the implementation of event engine may extend the standad #pcilib_event_info_t definition and provide extra information about the event.
			If big enough info buffer is provided, this additional information will be copied as well. Otherwise only standard information is provided.
 * @param[out] info	- The information about the recorded event is written to `info`
 * @return		- error code or 0 on success
 */
int pcilib_get_next_event(pcilib_t *ctx, pcilib_timeout_t timeout, pcilib_event_id_t *evid, size_t info_size, pcilib_event_info_t *info);

/**
 * Requests the streaming the rawdata from the event engine. The callback will be called 
 * each time new DMA packet is received. It is the fastest way to acuqire data. 
 * No memory copies performed and DMA buffers are directly passed to the specified callback. 
 * However, to prevent data loss, no long processing is allowed is only expected to copy data 
 * into the appropriate place and return control to the event engine.
 *
 * This function should be exectued before the grabbing is started with pcilib_start().
 * The performance can be boosted further by disabling any data processing within the event
 * engine. This is achieved by passing the #PCILIB_EVENT_FLAG_RAW_DATA_ONLY flag to pcilib_start() 
 * function while starting the grabbing.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] callback	- callback function to call for each event, the streaming is stopped if it return #PCILIB_STREAMING_STOP or negative value indicating the error
 * @param[in,out] user	- used data is passed through as the last parameter of callback function
 * @return		- error code or 0 on success
 */
int pcilib_configure_rawdata_callback(pcilib_t *ctx, pcilib_event_rawdata_callback_t callback, void *user);

/**
 * Configures conditions when the grabbing will be stopped automatically. The recording of new events may be 
 * stopped after reaching max_events records or when the specified amount of time is elapsed whatever happens
 * first. However, the pcilib_stop() function still must be called afterwards. 
 *
 * This function should be exectued before the grabbing is started with pcilib_start(). 
 * NOTE that this options might not be respected if grabbing is started with the 
 * #PCILIB_EVENT_FLAG_RAW_DATA_ONLY flag specified.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] max_events - specifies number of events after which the grabbing is stopped
 * @param[in] duration	- specifies number of microseconds to run the grabbing, special values #PCILIB_TIMEOUT_IMMEDIATE and #PCILIB_TIMEOUT_INFINITE are supported.
 * @return		- error code or 0 on success
 */
int pcilib_configure_autostop(pcilib_t *ctx, size_t max_events, pcilib_timeout_t duration);

/**
 * Request the auto-triggering while grabbing. The software triggering is currently not supported (but planned).
 * Therefore it is fully relied on hardware support. If no hardware support is available, #PCILIB_ERROR_NOTSUPPORTED
 * will be returned. 
 *
 * This function should be exectued before the grabbing is started with pcilib_start().
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] interval	- instructs hardware that each `interval` microseconds a new trigger should be issued
 * @param[in] event	- specifies ID of the event which will be triggered
 * @param[in] trigger_size - specifies the size of `trigger` buffer
 * @param[in] trigger_data - this implementation-specific buffer which will be passed through to the Event engine
 * @return		- error code or 0 on success
 */
int pcilib_configure_autotrigger(pcilib_t *ctx, pcilib_timeout_t interval, pcilib_event_t event, size_t trigger_size, void *trigger_data);

/** 
 * Issues a single software trigger for the specified event. No hardware support is required. The function 
 * is fully thread safe and can be called while other thread is blocked in the pcilib_stream() or pcilib_get_next_event()
 * calls.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] event	- specifies ID of the event to trigger
 * @param[in] trigger_size - specifies the size of `trigger` buffer
 * @param[in] trigger_data - this implementation-specific buffer which will be passed through to the Event engine
 * @return		- error code or 0 on success
 */  
int pcilib_trigger(pcilib_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);

/** public_api_event
 * @}
 */

/***********************************************************************************************************//**
 * \defgroup public_api_event_data Public Event Data API
 * A part of Event API actually providing access to the data
 * @{
 */

/**
 * Resolves the data type based on its name.
 * @param[in,out] ctx	- pcilib context
 * @param[in] event	- the ID of the event producing the data
 * @param[in] data_type	- the name of data type
 * @return		- the data type or PCILIB_EVENT_DATA_TYPE_INVALID if the specified type is not found 
 */
pcilib_event_data_type_t pcilib_find_event_data_type(pcilib_t *ctx, pcilib_event_t event, const char *data_type);

/**
 * This is a simples way to grab the data from the event engine. The event engine is started, a software trigger
 * for the specified event is issued if `timeout` is equal to PCILIB_TIMEOUT_IMMEDIATE, event is grabbed and the 
 * default data is copied into the user buffer. Then, the grabbing is stopped.
 * @param[in,out] ctx	- pcilib context
 * @param[in] event	- the event to trigger and/or event mask to grab
 * @param[in,out] size	- specifies the size of the provided buffer (if user supplies the buffer), the amount of data actually written is returned in this paramter
 * @param[in,out] data  - Either contains a pointer to user-supplied buffer where the data will be written or pointer to NULL otherwise. 
			In the later case, the pointer to newly allocated buffer will be returned in case of success. It is responsibility of the user to free the memory in this case.
			In case of failure, the content of data is undefined.
 * @param[in] timeout	- either is equal to #PCILIB_TIMEOUT_IMMEDIATE for immediate software trigger or specifies number of microseconds to wait for event triggered by hardware
 * @return 		- error code or 0 on success
 */
int pcilib_grab(pcilib_t *ctx, pcilib_event_t event, size_t *size, void **data, pcilib_timeout_t timeout);

/**
 * Copies the data of the specified type associated with the specified event into the provided buffer. May return #PCILIB_ERROR_OVERWRITTEN
 * if the data was overwritten during the call.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] event_id	- specifies the ID of event to get data from
 * @param[in] data_type	- specifies the desired type of data
			  - PCILIB_EVENT_DATA will request the default data type
			  - PCILIB_EVENT_RAW_DATA will request the raw data
			  - The other types of data may be defined by event engine
 * @param[in] size	- specifies the size of provided buffer in bytes
 * @param[out] buf	- the data will be copied in this buffer if it is big enough, otherwise #PCILIB_ERROR_TOOBIG will be returned
 * @param[out] retsize	- the number of actually written bytes will be returned in this parameter
 * @return 		- error code or 0 on success
 */ 
int pcilib_copy_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t size, void *buf, size_t *retsize);

/**
 * Copies the data of the specified type associated with the specified event into the provided buffer. May return #PCILIB_ERROR_OVERWRITTEN
 * if the data was overwritten during the call. This is very similar to pcilib_copy_data(), but allows to specify implementation specific 
 * argument explaining the requested data format.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] event_id	- specifies the ID of event to get data from
 * @param[in] data_type	- specifies the desired type of data
			  - PCILIB_EVENT_DATA will request the default data type
			  - PCILIB_EVENT_RAW_DATA will request the raw data
			  - The other types of data may be defined by event engine
 * @param[in] arg_size	- specifies the size of `arg` in bytes
 * @param[in] arg	- implementation-specific argument expalining the requested data format
 * @param[in] size	- specifies the size of provided buffer in bytes
 * @param[out] buf	- the data will be copied in this buffer if it is big enough, otherwise #PCILIB_ERROR_TOOBIG will be returned
 * @param[out] retsize	- the number of actually written bytes will be returned in this parameter
 * @return 		- error code or 0 on success
 */ 
int pcilib_copy_data_with_argument(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t size, void *buf, size_t *retsize);


/**
 * Returns pointer to the data of the specified type associated with the specified event. The data should be returned back to the 
 * Event engine using pcilib_return_data() call. WARNING: Current implementation may overwrite the data before the pcilib_return_data()
 * call is executed. In this case the #PCILIB_ERROR_OVERWRITTEN error will be returned by pcilib_return_data() call. I guess this is 
 * a bad idea and have to be changed. Meanwhile, for the ipecamera implementation the image data will be never overwritten. However, 
 * the raw data may get overwritten and the error code of pcilib_return_data() has to be consulted.
 * 
 * @param[in,out] ctx	- pcilib context
 * @param[in] event_id	- specifies the ID of event to get data from
 * @param[in] data_type	- specifies the desired type of data
			  - PCILIB_EVENT_DATA will request the default data type
			  - PCILIB_EVENT_RAW_DATA will request the raw data
			  - The other types of data may be defined by event engine
 * @param[out] size_or_err - contain error code if function returns NULL or number of actually written bytes otherwise
 * @return 		- the pointer to the requested data or NULL otherwise
 */ 
void *pcilib_get_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t *size_or_err);

/**
 * Returns pointer to the data of the specified type associated with the specified event. The data should be returned back to the 
 * Event engine using pcilib_return_data() call. WARNING: Current implementation may overwrite the data before the pcilib_return_data()
 * call is executed, @see pcilib_get_data() for details. Overall this function is very similar to pcilib_get_data(), but allows to 
 * specify implementation specific argument explaining the requested data format.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] event_id	- specifies the ID of event to get data from
 * @param[in] data_type	- specifies the desired type of data
			  - PCILIB_EVENT_DATA will request the default data type
			  - PCILIB_EVENT_RAW_DATA will request the raw data
			  - The other types of data may be defined by event engine
 * @param[in] arg_size	- specifies the size of `arg` in bytes
 * @param[in] arg	- implementation-specific argument expalining the requested data format
 * @param[out] size_or_err - contain error code if function returns NULL or number of actually written bytes otherwise
 * @return 		- the pointer to the requested data or NULL otherwise
 */ 
void *pcilib_get_data_with_argument(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size_or_err);


/*
 * This function returns data obtained using pcilib_get_data() or pcilib_get_data_with_argument() calls. 
 * It occasionally may return #PCILIB_ERROR_OVERWRITTEN error indicating that the data was overwritten 
 * between pcilib_get_data() and pcilib_return_data() calls, @see pcilib_get_data() for details.
 * @param[in,out] ctx	- pcilib context
 * @param[in] event_id	- specifies the ID of event to get data from
 * @param[in] data_type	- specifies the data type request in the pcilib_get_data() call
 * @param[in,out] data	- specifies the buffer returned by pcilib_get_data() call
 * @return		- #PCILIB_ERROR_OVERWRITTEN or 0 if data is still valid
 */
int pcilib_return_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, void *data);

/** public_api_event_data
 * @}
 */


/***********************************************************************************************************//**
 * \defgroup public_api_value Polymorphic values
 * API for manipulation of data formats and units
 * @{
 */

/**
 * Destroys the polymorphic value and frees any extra used memory, but does not free #pcilib_value_t itself
 * @param[in] ctx	- pcilib context
 * @param[in,out] val	- initialized polymorphic value
 */
void pcilib_clean_value(pcilib_t *ctx, pcilib_value_t *val);

/**
 * Copies the polymorphic value. If `dst` already contains the value, cleans it first. 
 * Therefore, before first usage the value should be always initialized to 0.
 * @param[in] ctx	- pcilib context
 * @param[in,out] dst	- initialized polymorphic value
 * @param[in] src	- initialized polymorphic value
 * @return		- 0 on success or memory error
 */
int pcilib_copy_value(pcilib_t *ctx, pcilib_value_t *dst, const pcilib_value_t *src);

/**
 * Initializes the polymorphic  value from floating-point number. If `val` already contains the value, cleans it first. 
 * Therefore, before first usage the value should be always initialized to 0.
 * @param[in] ctx	- pcilib context
 * @param[in,out] val	- initialized polymorphic value
 * @param[in] fval	- initializer
 * @return		- 0 on success or memory error
 */
int pcilib_set_value_from_float(pcilib_t *ctx, pcilib_value_t *val, double fval);

/**
 * Initializes the polymorphic value from integer. If `val` already contains the value, cleans it first. 
 * Therefore, before first usage the value should be always initialized to 0.
 * @param[in] ctx	- pcilib context
 * @param[in,out] val	- initialized polymorphic value
 * @param[in] ival	- initializer
 * @return		- 0 on success or memory error
 */
int pcilib_set_value_from_int(pcilib_t *ctx, pcilib_value_t *val, long ival);

/**
 * Initializes the polymorphic value from the register value. If `val` already contains the value, cleans it first. 
 * Therefore, before first usage the value should be always initialized to 0.
 * @param[in] ctx	- pcilib context
 * @param[in,out] val	- initialized polymorphic value
 * @param[in] regval	- initializer
 * @return		- 0 on success or memory error
 */
int pcilib_set_value_from_register_value(pcilib_t *ctx, pcilib_value_t *val, pcilib_register_value_t regval);

/**
 * Initializes the polymorphic value from the static string. The string is not copied, but only referenced.
 * If `val` already contains the value, cleans it first. Therefore, before first usage the value should be always initialized to 0.
 * @param[in] ctx	- pcilib context
 * @param[in,out] val	- initialized polymorphic value
 * @param[in] str	- initializer
 * @return		- 0 on success or memory error
 */
int pcilib_set_value_from_static_string(pcilib_t *ctx, pcilib_value_t *val, const char *str);

/**
 * Initializes the polymorphic value from the string. The string is copied.
 * If `val` already contains the value, cleans it first. Therefore, before first usage the value should be always initialized to 0.
 * @param[in] ctx	- pcilib context
 * @param[in,out] val	- initialized polymorphic value
 * @param[in] str	- initializer
 * @return		- 0 on success or memory error
 */
int pcilib_set_value_from_string(pcilib_t *ctx, pcilib_value_t *val, const char *str);

/**
 * Get the floating point value from the polymorphic type. May inmply impliced type conversion,
 * for isntance parsing the number from the string. Will return 0. and report an error if
 * conversion failed.
 * @param[in] ctx	- pcilib context
 * @param[in] val	- initialized polymorphic value of arbitrary type
 * @param[out] err	- error code or 0 on sccuess
 * @return		- the value or 0 in the case of error
 */
double pcilib_get_value_as_float(pcilib_t *ctx, const pcilib_value_t *val, int *err);

/**
 * Get the integer value from the polymorphic type. May inmply impliced type conversion
 * resulting in precision loss if the `val` stores floating-point number. The warning
 * message will be printed in this case, but no error returned.
 * @param[in] ctx	- pcilib context
 * @param[in] val	- initialized polymorphic value of arbitrary type
 * @param[out] err	- error code or 0 on sccuess
 * @return		- the value or 0 in the case of error
 */
long pcilib_get_value_as_int(pcilib_t *ctx, const pcilib_value_t *val, int *err);

/**
 * Get the integer value from the polymorphic type. May inmply impliced type conversion
 * resulting in precision loss if the `val` stores floating-point number or complete 
 * data corruption if negative number is stored.  The warning message will be printed 
 * in this case, but no error returned.
 * @param[in] ctx	- pcilib context
 * @param[in] val	- initialized polymorphic value of arbitrary type
 * @param[out] err	- error code or 0 on sccuess
 * @return		- the value or 0 in the case of error
 */
pcilib_register_value_t pcilib_get_value_as_register_value(pcilib_t *ctx, const pcilib_value_t *val, int *err);


/**
 * Convert the units of the supplied polymorphic value. The error will be reported if currently used units of the 
 * value are unknown, the requested conversion is not supported, or the value is not numeric.
 * @param[in] ctx	- pcilib context
 * @param[in,out] val	- initialized polymorphic value of any numeric type
 * @param[in] unit_name	- the requested units
 * @return err		- error code or 0 on sccuess
 */
int pcilib_convert_value_unit(pcilib_t *ctx, pcilib_value_t *val, const char *unit_name);

/**
 * Convert the type of the supplied polymorphic value. The error will be reported if conversion
 * is not supported or failed due to non-conformant content.
 * @param[in] ctx	- pcilib context
 * @param[in,out] val	- initialized polymorphic value of any type
 * @param[in] type	- the requested type
 * @return err		- error code or 0 on sccuess
 */
int pcilib_convert_value_type(pcilib_t *ctx, pcilib_value_t *val, pcilib_value_type_t type);

/** public_api_value
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* _PCITOOL_PCILIB_H */
