#ifndef _PCITOOL_PCILIB_H
#define _PCITOOL_PCILIB_H

#define PCILIB_MAX_BANKS 6

#include <time.h>
#include <stdint.h>

#ifndef __timespec_defined
struct timespec {
    time_t tv_sec;
    long tv_nsec;
};
#endif /* __timespec_defined */

#define pcilib_memcpy pcilib_memcpy32
#define pcilib_datacpy pcilib_datacpy32

typedef struct pcilib_s pcilib_t;
typedef void pcilib_context_t;

typedef uint8_t pcilib_bar_t;			/**< Type holding the PCI Bar number */
typedef uint8_t pcilib_register_t;		/**< Type holding the register ID within the Bank */
typedef uint8_t pcilib_register_addr_t;		/**< Type holding the register ID within the Bank */
typedef uint8_t pcilib_register_bank_t;		/**< Type holding the register bank number */
typedef uint8_t pcilib_register_bank_addr_t;	/**< Type holding the register bank number */
typedef uint8_t pcilib_register_size_t;		/**< Type holding the size in bits of the register */
typedef uint32_t pcilib_register_value_t;	/**< Type holding the register value */
typedef uint64_t pcilib_event_id_t;
typedef uint8_t pcilib_dma_addr_t;
typedef uint8_t pcilib_dma_t;


typedef uint32_t pcilib_event_t;

typedef enum {
    PCILIB_HOST_ENDIAN = 0,
    PCILIB_LITTLE_ENDIAN,
    PCILIB_BIG_ENDIAN
} pcilib_endianess_t;

typedef enum {
    PCILIB_MODEL_DETECT,
    PCILIB_MODEL_PCI,
    PCILIB_MODEL_IPECAMERA
} pcilib_model_t;

typedef enum {
    PCILIB_REGISTER_R = 1,
    PCILIB_REGISTER_W = 2,
    PCILIB_REGISTER_RW = 3
} pcilib_register_mode_t;

typedef enum {
    PCILIB_DEFAULT_PROTOCOL,
    IPECAMERA_REGISTER_PROTOCOL
} pcilib_register_protocol_t;

typedef enum {
    PCILIB_EVENT_DATA
} pcilib_event_data_type_t;

#define PCILIB_BAR_DETECT 		((pcilib_bar_t)-1)
#define PCILIB_BAR_INVALID		((pcilib_bar_t)-1)
#define PCILIB_BAR0			0
#define PCILIB_BAR1			1
#define PCILIB_REGISTER_INVALID		((pcilib_register_t)-1)
#define PCILIB_ADDRESS_INVALID		((uintptr_t)-1)
#define PCILIB_REGISTER_BANK_INVALID	((pcilib_register_bank_t)-1)
#define PCILIB_REGISTER_BANK0 		0
#define PCILIB_REGISTER_BANK1 		1
#define PCILIB_REGISTER_BANK2 		2
#define PCILIB_REGISTER_BANK3 		3
#define PCILIB_EVENT0			1
#define PCILIB_EVENT1			2
#define PCILIB_EVENT2			4
#define PCILIB_EVENT3			8
#define PCILIB_EVENTS_ALL		((pcilib_event_t)-1)
#define PCILIB_EVENT_INVALID		((pcilib_event_t)-1)
#define PCILIB_EVENT_ID_INVALID		0

typedef struct {
    pcilib_register_bank_addr_t addr;

    pcilib_bar_t bar;			// optional
    size_t size;
    
    pcilib_register_protocol_t protocol;

    uintptr_t read_addr;		// or offset if bar specified
    uintptr_t write_addr;		// or offset if bar specified
    uint8_t raw_endianess;

    uint8_t access;
    uint8_t endianess;
    
    const char *format;
    const char *name;
    const char *description;
} pcilib_register_bank_description_t;

typedef struct {
    pcilib_register_addr_t addr;
    pcilib_register_size_t offset;
    pcilib_register_size_t bits;
    pcilib_register_value_t defvalue;
    pcilib_register_mode_t mode;

    pcilib_register_bank_t bank;
    
    const char *name;
    const char *description;
} pcilib_register_description_t;

/**
  * Default mappings
  */
typedef struct {
    uintptr_t start;
    uintptr_t end;
    pcilib_register_bank_t bank;
} pcilib_register_range_t;

typedef struct {
    const char *name;
    const char *description;
} pcilib_event_description_t;

typedef struct {
    int (*read)(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t *value);
    int (*write)(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t value);
} pcilib_protocol_description_t;

typedef enum {
    PCILIB_DMA_FROM_DEVICE = 1,
    PCILIB_DMA_TO_DEVICE = 2,
    PCILIB_DMA_BIDIRECTIONAL = 3
} pcilib_dma_direction_t;

typedef enum {
    PCILIB_DMA_TYPE_BLOCK,
    PCILIB_DMA_TYPE_PACKET
} pcilib_dma_type_t;

typedef struct {
    pcilib_dma_addr_t addr;
    pcilib_dma_type_t type;
    pcilib_dma_direction_t direction;
    size_t max_bytes;
} pcilib_dma_engine_description_t;

typedef struct {
    pcilib_dma_engine_description_t **engines;
} pcilib_dma_info_t;

typedef int (*pcilib_callback_t)(pcilib_event_t event, pcilib_event_id_t event_id, void *user);

typedef struct {
    pcilib_dma_context_t *(*init)(pcilib_t *ctx);
    void (*free)(pcilib_dma_context_t *ctx);

//    int (*read)(pcilib_dma_context_t *ctx, pcilib_event_t event_mask, pcilib_callback_t callback, void *user);
//    int (*write)(pcilib_dma_context_t *ctx);
} pcilib_dma_api_description_t;

typedef struct {
    pcilib_context_t *(*init)(pcilib_t *ctx);
    void (*free)(pcilib_context_t *ctx);

    int (*reset)(pcilib_context_t *ctx);

    int (*start)(pcilib_context_t *ctx, pcilib_event_t event_mask, pcilib_callback_t callback, void *user);
    int (*stop)(pcilib_context_t *ctx);
    int (*trigger)(pcilib_context_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);
    
    pcilib_event_id_t (*next_event)(pcilib_context_t *ctx, pcilib_event_t event_mask, const struct timespec *timeout);
    void* (*get_data)(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size);
    int (*return_data)(pcilib_context_t *ctx, pcilib_event_id_t event_id);
} pcilib_event_api_description_t;

typedef struct {
    uint8_t access;
    uint8_t endianess;
    
    pcilib_register_description_t *registers;
    pcilib_register_bank_description_t *banks;
    pcilib_register_range_t *ranges;
    pcilib_event_description_t *events;

    pcilib_dma_api_description_t *dma_api;    
    pcilib_event_api_description_t *event_api;
} pcilib_model_description_t;

#ifndef _PCILIB_PCI_C
extern pcilib_model_description_t pcilib_model[];
#endif /* ! _PCILIB_PCI_C */

int pcilib_set_error_handler(void (*err)(const char *msg, ...), void (*warn)(const char *msg, ...));

pcilib_model_t pcilib_get_model(pcilib_t *ctx);
pcilib_context_t *pcilib_get_implementation_context(pcilib_t *ctx);

pcilib_t *pcilib_open(const char *device, pcilib_model_t model);
void pcilib_close(pcilib_t *ctx);

void *pcilib_map_bar(pcilib_t *ctx, pcilib_bar_t bar);
void pcilib_unmap_bar(pcilib_t *ctx, pcilib_bar_t bar, void *data);
char *pcilib_resolve_register_address(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr);
char *pcilib_resolve_data_space(pcilib_t *ctx, uintptr_t addr, size_t *size);

pcilib_register_bank_t pcilib_find_bank_by_addr(pcilib_t *ctx, pcilib_register_bank_addr_t bank);
pcilib_register_bank_t pcilib_find_bank_by_name(pcilib_t *ctx, const char *bankname);
pcilib_register_bank_t pcilib_find_bank(pcilib_t *ctx, const char *bank);
pcilib_register_t pcilib_find_register(pcilib_t *ctx, const char *bank, const char *reg);
pcilib_event_t pcilib_find_event(pcilib_t *ctx, const char *event);

int pcilib_read(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf);
int pcilib_write(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf);

int pcilib_read_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf);
int pcilib_write_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf);
int pcilib_read_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t *value);
int pcilib_write_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t value);
int pcilib_read_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t *value);
int pcilib_write_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t value);

int pcilib_reset(pcilib_t *ctx);
int pcilib_start(pcilib_t *ctx, pcilib_event_t event_mask, void *callback, void *user);
int pcilib_stop(pcilib_t *ctx);

int pcilib_trigger(pcilib_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);

pcilib_event_id_t pcilib_get_next_event(pcilib_t *ctx, pcilib_event_t event_mask, const struct timespec *timeout);
void *pcilib_get_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t *size);
void *pcilib_get_data_with_argument(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size);
/*
 * This function is provided to find potentially corrupted data. If the data is overwritten by 
 * the time return_data is called it will return error.
 */
int pcilib_return_data(pcilib_t *ctx, pcilib_event_id_t event_id);

/*
 * @param data - will be allocated and shuld be freed if NULL, otherwise used and size should contain correct size.
 *   In case of failure the content of data is undefined.
 * @param timeout - will be autotriggered if NULL
 */
int pcilib_grab(pcilib_t *ctx, pcilib_event_t event_mask, size_t *size, void **data, const struct timespec *timeout);

#endif /* _PCITOOL_PCILIB_H */
