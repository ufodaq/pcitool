//#define PCILIB_FILE_IO
#define _XOPEN_SOURCE 700
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include "pcilib.h"
#include "pci.h"
#include "tools.h"
#include "error.h"
#include "model.h"
#include "plugin.h"
#include "bar.h"
#include "xml.h"
#include "locking.h"

static int pcilib_detect_model(pcilib_t *ctx, const char *model) {
    int i, j;

    const pcilib_model_description_t *model_info = NULL;
    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);

    model_info = pcilib_find_plugin_model(ctx, board_info->vendor_id, board_info->device_id, model);
    if (model_info) {
	memcpy(&ctx->model_info, model_info, sizeof(pcilib_model_description_t));
	memcpy(&ctx->dma, model_info->dma, sizeof(pcilib_dma_description_t));
	ctx->model = strdup(model_info->name);
    } else if (model) {
	    // If not found, check for DMA models
	for (i = 0; pcilib_dma[i].name; i++) {
	    if (!strcasecmp(model, pcilib_dma[i].name))
		break;
	}

	if (pcilib_dma[i].api) {
	    model_info = &ctx->model_info;
	    memcpy(&ctx->dma, &pcilib_dma[i], sizeof(pcilib_dma_description_t));
	    ctx->model_info.dma = &ctx->dma;
	}
    }

	    // Precedens of register configuration: DMA/Event Initialization (top), XML, Event Description, DMA Description (least)
    if (model_info) {
	const pcilib_dma_description_t *dma = model_info->dma;

	if (dma) {
	    if (dma->banks)
		pcilib_add_register_banks(ctx, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, 0, dma->banks, NULL);

	    if (dma->registers)
		pcilib_add_registers(ctx, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, 0, dma->registers, NULL);

	    if (dma->engines) {
		for (j = 0; dma->engines[j].addr_bits; j++);
		memcpy(ctx->engines, dma->engines, j * sizeof(pcilib_dma_engine_description_t));
		ctx->num_engines = j;
	    } else
		ctx->dma.engines = ctx->engines;
	}

	if (model_info->protocols)
	    pcilib_add_register_protocols(ctx, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, 0, model_info->protocols, NULL);
		
	if (model_info->banks)
	    pcilib_add_register_banks(ctx, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, 0, model_info->banks, NULL);

	if (model_info->registers)
	    pcilib_add_registers(ctx, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, 0, model_info->registers, NULL);
		
	if (model_info->ranges)
	    pcilib_add_register_ranges(ctx, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, 0, model_info->ranges);
    }

    // Load XML registers

    // Check for all installed models
    // memcpy(&ctx->model_info, model, sizeof(pcilib_model_description_t));
    // how we reconcile the banks from event model and dma description? The banks specified in the DMA description should override corresponding banks of events...


    if (!model_info) {
	if ((model)&&(strcasecmp(model, "pci"))/*&&(no xml)*/)
	    return PCILIB_ERROR_NOTFOUND;
	
	ctx->model = strdup("pci");
    }

    return 0;
}



pcilib_t *pcilib_open(const char *device, const char *model) {
    int err, xmlerr;
    pcilib_t *ctx = malloc(sizeof(pcilib_t));
    const pcilib_driver_version_t *drv_version;
	
    if (!model)
	model = getenv("PCILIB_MODEL");

    if (ctx) {
	memset(ctx, 0, sizeof(pcilib_t));
	ctx->pci_cfg_space_fd = -1;
	
	ctx->handle = open(device, O_RDWR);
	if (ctx->handle < 0) {
	    pcilib_error("Error opening device (%s)", device);
	    free(ctx);
	    return NULL;
	}
	
	drv_version = pcilib_get_driver_version(ctx);
	if (!drv_version) {
	    pcilib_error("Driver verification has failed (%s)", device);
	    free(ctx);
	    return NULL;
	}
	
	ctx->page_mask = (uintptr_t)-1;
	
	if ((model)&&(!strcasecmp(model, "maintenance"))) {
	    ctx->model = strdup("maintenance");
	    return ctx;
	}

	err = pcilib_init_locking(ctx);
	if (err) {
	    pcilib_error("Error (%i) initializing locking subsystem", err);
	    pcilib_close(ctx);
	    return NULL;
	}
	
	err = pcilib_init_py(ctx);
	if (err) {
	    pcilib_error("Error (%i) initializing python subsystem", err);
	    pcilib_close(ctx);
	    return NULL;
	}

	ctx->alloc_reg = PCILIB_DEFAULT_REGISTER_SPACE;
	ctx->alloc_views = PCILIB_DEFAULT_VIEW_SPACE;
	ctx->alloc_units = PCILIB_DEFAULT_UNIT_SPACE;
	ctx->registers = (pcilib_register_description_t *)malloc(PCILIB_DEFAULT_REGISTER_SPACE * sizeof(pcilib_register_description_t));
	ctx->register_ctx = (pcilib_register_context_t *)malloc(PCILIB_DEFAULT_REGISTER_SPACE * sizeof(pcilib_register_context_t));
	ctx->views = (pcilib_view_description_t**)malloc(PCILIB_DEFAULT_VIEW_SPACE * sizeof(pcilib_view_description_t*));
	ctx->units = (pcilib_unit_description_t*)malloc(PCILIB_DEFAULT_UNIT_SPACE * sizeof(pcilib_unit_description_t));
	
	if ((!ctx->registers)||(!ctx->register_ctx)||(!ctx->views)||(!ctx->units)) {
	    pcilib_error("Error allocating memory for register model");
	    pcilib_close(ctx);
	    return NULL;
	}
	
	memset(ctx->registers, 0, sizeof(pcilib_register_description_t));
	memset(ctx->units, 0, sizeof(pcilib_unit_t));
	memset(ctx->views, 0, sizeof(pcilib_view_t*));
	memset(ctx->banks, 0, sizeof(pcilib_register_bank_description_t));
	memset(ctx->ranges, 0, sizeof(pcilib_register_range_t));

	memset(ctx->register_ctx, 0, PCILIB_DEFAULT_REGISTER_SPACE * sizeof(pcilib_register_context_t));

	pcilib_add_register_protocols(ctx, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, 0, pcilib_standard_register_protocols, NULL);
	pcilib_add_register_banks(ctx, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, 0, pcilib_standard_register_banks, NULL);
	pcilib_add_registers(ctx, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, 0, pcilib_standard_registers, NULL);

	err = pcilib_detect_model(ctx, model);
	if ((err)&&(err != PCILIB_ERROR_NOTFOUND)) {
	    const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
	    if (board_info)
	        pcilib_error("Error (%i) configuring model %s (%x:%x)", err, (model?model:""), board_info->vendor_id, board_info->device_id);
	    else
	        pcilib_error("Error (%i) configuring model %s", err, (model?model:""));
	    pcilib_close(ctx);
	    return NULL;
	}

	if (!ctx->model)
	    ctx->model = strdup(model?model:"pci");
	    
	err = pcilib_py_add_script_dir(ctx, NULL);
	if (err) {
	    pcilib_error("Error (%i) add script path to python path", err);
	    pcilib_close(ctx);
	    return NULL;
	}
	
	
	xmlerr = pcilib_init_xml(ctx, ctx->model);
	if ((xmlerr)&&(xmlerr != PCILIB_ERROR_NOTFOUND)) {
	    pcilib_error("Error (%i) initializing XML subsystem for model %s", xmlerr, ctx->model);
	    pcilib_close(ctx);
	    return NULL;
	}
	

	    // We have found neither standard model nor XML
	if ((err)&&(xmlerr)) {
	    pcilib_error("The specified model (%s) is not available", model);
	    pcilib_close(ctx);
	    return NULL;
	}
	
	ctx->model_info.registers = ctx->registers;
	ctx->model_info.banks = ctx->banks;
	ctx->model_info.protocols = ctx->protocols;
	ctx->model_info.ranges = ctx->ranges;
	ctx->model_info.views = (const pcilib_view_description_t**)ctx->views;
	ctx->model_info.units = ctx->units;

	err = pcilib_init_register_banks(ctx);
	if (err) {
	    pcilib_error("Error (%i) initializing regiser banks\n", err);
	    pcilib_close(ctx);
	    return NULL;
	}
	err = pcilib_init_event_engine(ctx);
	if (err) {
	    pcilib_error("Error (%i) initializing event engine\n", err);
	    pcilib_close(ctx);
	    return NULL;
	}
    }

    return ctx;
}


const pcilib_driver_version_t *pcilib_get_driver_version(pcilib_t *ctx) {
    int ret;
    
    if (!ctx->driver_version.version) {
	ret = ioctl( ctx->handle, PCIDRIVER_IOC_VERSION, &ctx->driver_version );
	if (ret) {
	    pcilib_error("PCIDRIVER_IOC_DRIVER_VERSION ioctl have failed");
	    return NULL;
	}
	
	if (ctx->driver_version.interface != PCIDRIVER_INTERFACE_VERSION) {
	    pcilib_error("Using pcilib (version: %u.%u.%u, driver interface: 0x%lx) with incompatible driver (version: %u.%u.%u, interface: 0x%lx)", 
		PCILIB_VERSION_GET_MAJOR(PCILIB_VERSION),
		PCILIB_VERSION_GET_MINOR(PCILIB_VERSION),
		PCILIB_VERSION_GET_MICRO(PCILIB_VERSION),
		PCIDRIVER_INTERFACE_VERSION,
		PCILIB_VERSION_GET_MAJOR(ctx->driver_version.version),
		PCILIB_VERSION_GET_MINOR(ctx->driver_version.version),
		PCILIB_VERSION_GET_MICRO(ctx->driver_version.version),
		ctx->driver_version.interface
	    );
	    return NULL;
	}
    }

    return &ctx->driver_version;
}

const pcilib_board_info_t *pcilib_get_board_info(pcilib_t *ctx) {
    int ret;
    
    if (ctx->page_mask ==  (uintptr_t)-1) {
	ret = ioctl( ctx->handle, PCIDRIVER_IOC_PCI_INFO, &ctx->board_info );
	if (ret) {
	    pcilib_error("PCIDRIVER_IOC_PCI_INFO ioctl have failed");
	    return NULL;
	}
	
	ctx->page_mask = pcilib_get_page_mask();
    }

    return &ctx->board_info;
}


pcilib_context_t *pcilib_get_implementation_context(pcilib_t *ctx) {
    return ctx->event_ctx;
}


void pcilib_close(pcilib_t *ctx) {
    pcilib_bar_t bar;

    if (ctx) {
	pcilib_dma_engine_t dma;
	const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
	const pcilib_event_api_description_t *eapi = model_info->api;
	const pcilib_dma_api_description_t *dapi = ctx->dma.api;
	
        if ((eapi)&&(eapi->free)) eapi->free(ctx->event_ctx);
        if ((dapi)&&(dapi->free)) dapi->free(ctx->dma_ctx);

	for  (dma = 0; dma < PCILIB_MAX_DMA_ENGINES; dma++) {
	    if (ctx->dma_rlock[dma])
		pcilib_return_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, ctx->dma_rlock[dma]);
	    if (ctx->dma_wlock[dma])
		pcilib_return_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, ctx->dma_wlock[dma]);
	}

	pcilib_free_register_banks(ctx, 0);

	if (ctx->event_plugin)
	    pcilib_plugin_close(ctx->event_plugin);

	if (ctx->locks.kmem)
	    pcilib_free_locking(ctx);

	if (ctx->kmem_list) {
	    pcilib_warning("Not all kernel buffers are properly cleaned");
	
	    while (ctx->kmem_list) {
		pcilib_free_kernel_memory(ctx, ctx->kmem_list, 0);
	    }
	}
	
	for (bar = 0; bar < PCILIB_MAX_BARS; bar++) {
	    if (ctx->bar_space[bar]) {
		char *ptr = ctx->bar_space[bar];
		ctx->bar_space[bar] = NULL;
		pcilib_unmap_bar(ctx, bar, ptr);
	    }
	}
	
	if (ctx->pci_cfg_space_fd >= 0)
	    close(ctx->pci_cfg_space_fd);


        if (ctx->units) {
            pcilib_clean_units(ctx, 0);
            free(ctx->units);
        }

	if (ctx->views) {
	    pcilib_clean_views(ctx, 0);
	    free(ctx->views);
	}

        pcilib_clean_registers(ctx, 0);

	if (ctx->register_ctx)
            free(ctx->register_ctx);

	if (ctx->registers)
	    free(ctx->registers);
	    
	if (ctx->model)
	    free(ctx->model);

	pcilib_free_xml(ctx);
	pcilib_free_py(ctx);

	if (ctx->handle >= 0)
	    close(ctx->handle);
	
	free(ctx);
    }
}

static int pcilib_update_pci_configuration_space(pcilib_t *ctx) {
    int err;
    int size;

    if (ctx->pci_cfg_space_fd < 0) {
	char fname[128];

	const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
	if (!board_info) {
	    pcilib_error("Failed to acquire board info");
	    return PCILIB_ERROR_FAILED;
	}

	sprintf(fname, "/sys/bus/pci/devices/0000:%02x:%02x.%1x/config", board_info->bus, board_info->slot, board_info->func);

	ctx->pci_cfg_space_fd = open(fname, O_RDONLY);
	if (ctx->pci_cfg_space_fd < 0) {
	    pcilib_error("Failed to open configuration space in %s", fname);
	    return PCILIB_ERROR_FAILED;
	}
    } else {
	err = lseek(ctx->pci_cfg_space_fd, SEEK_SET, 0);
	if (err) {
	    close(ctx->pci_cfg_space_fd);
	    ctx->pci_cfg_space_fd = -1;
	    return pcilib_update_pci_configuration_space(ctx);
	}
    }

    size = read(ctx->pci_cfg_space_fd, ctx->pci_cfg_space_cache, 256);
    if (size < 64) {
	if (size <= 0)
	    pcilib_error("Failed to read PCI configuration from sysfs, errno: %i", errno);
	else
	    pcilib_error("Failed to read PCI configuration from sysfs, only %zu bytes read (expected at least 64)", size);
	return PCILIB_ERROR_FAILED;
    }

    ctx->pci_cfg_space_size = size;

    return 0;
}


static uint32_t *pcilib_get_pci_capabilities(pcilib_t *ctx, int cap_id) {
    int err;

    uint32_t cap;
    uint8_t cap_offset;		/**< Offset of capability in the configuration space */

    if (!ctx->pci_cfg_space_fd) {
	err = pcilib_update_pci_configuration_space(ctx);
	if (err) {
	    pcilib_error("Error (%i) reading PCI configuration space", err);
	    return NULL;
	}
    }

	// This is just a pointer to the first cap
    cap = ctx->pci_cfg_space_cache[(0x34>>2)];
    cap_offset = cap&0xFC;

    while ((cap_offset)&&(cap_offset < ctx->pci_cfg_space_size)) {
	cap = ctx->pci_cfg_space_cache[cap_offset>>2];
	if ((cap&0xFF) == cap_id)
	    return &ctx->pci_cfg_space_cache[cap_offset>>2];
	cap_offset = (cap>>8)&0xFC;
    }

    return NULL;
};


static const uint32_t *pcilib_get_pcie_capabilities(pcilib_t *ctx) {
    if (ctx->pcie_capabilities)
	return ctx->pcie_capabilities;

    ctx->pcie_capabilities = pcilib_get_pci_capabilities(ctx, 0x10);
    return ctx->pcie_capabilities;
}


const pcilib_pcie_link_info_t *pcilib_get_pcie_link_info(pcilib_t *ctx) {
    int err;
    const uint32_t *cap;

    err = pcilib_update_pci_configuration_space(ctx);
    if (err) {
	pcilib_error("Error (%i) updating PCI configuration space", err);
	return NULL;
    }

    cap = pcilib_get_pcie_capabilities(ctx);
    if (!cap) return NULL;

	// Generally speaking this can be updated during the application life time
    
    ctx->link_info.max_payload = (cap[1] & 0x07) + 7;
    ctx->link_info.payload = ((cap[2] >> 5) & 0x07) + 7;
    ctx->link_info.link_speed = (cap[3]&0xF);
    ctx->link_info.link_width = (cap[3]&0x3F0) >> 4;
    ctx->link_info.max_link_speed = (cap[4]&0xF0000) >> 16;
    ctx->link_info.max_link_width = (cap[4]&0x3F00000) >> 20;

    return &ctx->link_info;
}

int pcilib_get_device_state(pcilib_t *ctx, pcilib_device_state_t *state) {
    int ret = ioctl( ctx->handle, PCIDRIVER_IOC_DEVICE_STATE, state);
    if (ret < 0) {
	pcilib_error("PCIDRIVER_IOC_DEVICE_STATE ioctl have failed");
	return PCILIB_ERROR_FAILED;
    }
    return 0;
}

int pcilib_set_dma_mask(pcilib_t *ctx, int mask) {
    if (ioctl(ctx->handle, PCIDRIVER_IOC_DMA_MASK, mask) < 0)
	return PCILIB_ERROR_FAILED;

    return 0;
}

int pcilib_set_mps(pcilib_t *ctx, int mps) {
    if (ioctl(ctx->handle, PCIDRIVER_IOC_MPS, mps) < 0)
	return PCILIB_ERROR_FAILED;

    return 0;
}
