//#define PCILIB_FILE_IO
#define _BSD_SOURCE
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
		pcilib_add_register_banks(ctx, 0, dma->banks);

	    if (dma->registers)
		pcilib_add_registers(ctx, 0, dma->registers);

	    if (dma->engines) {
		for (j = 0; dma->engines[j].addr_bits; j++);
		memcpy(ctx->engines, dma->engines, j * sizeof(pcilib_dma_engine_description_t));
		ctx->num_engines = j;
	    } else
		ctx->dma.engines = ctx->engines;
	}

	if (model_info->protocols)
	    pcilib_add_register_protocols(ctx, 0, model_info->protocols);
		
	if (model_info->banks)
	    pcilib_add_register_banks(ctx, 0, model_info->banks);

	if (model_info->registers)
	    pcilib_add_registers(ctx, 0, model_info->registers);
		
	if (model_info->ranges)
	    pcilib_add_register_ranges(ctx, 0, model_info->ranges);
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
    int err;
    size_t i;
    pcilib_t *ctx = malloc(sizeof(pcilib_t));
	
	pcilib_register_description_t *registers=NULL;
	pcilib_register_bank_description_t *banks=NULL;
	int number_registers, number_banks;
		
	char *xmlfile;
	pcilib_xml_read_config(&xmlfile,3);
	xmlDocPtr doc;
	doc=pcilib_xml_getdoc(xmlfile);
	
	xmlXPathContextPtr context;
	context=pcilib_xml_getcontext(doc);

	number_registers=pcilib_xml_getnumberregisters(context);
	number_banks=pcilib_xml_getnumberbanks(context);

	if(number_registers)registers=calloc((number_registers),sizeof(pcilib_register_description_t));
	else pcilib_error("no registers in the xml file");
	if(number_banks)banks=calloc((number_banks),sizeof(pcilib_register_bank_description_t));
	else pcilib_error("no banks in the xml file");

    if (ctx) {
	memset(ctx, 0, sizeof(pcilib_t));
	ctx->pci_cfg_space_fd = -1;
	
	ctx->handle = open(device, O_RDWR);
	if (ctx->handle < 0) {
	    pcilib_error("Error opening device (%s)", device);
	    free(ctx);
	    return NULL;
	}
	
	ctx->page_mask = (uintptr_t)-1;

	ctx->alloc_reg = PCILIB_DEFAULT_REGISTER_SPACE;
	ctx->registers = (pcilib_register_description_t *)malloc(PCILIB_DEFAULT_REGISTER_SPACE * sizeof(pcilib_register_description_t));
	ctx->register_ctx = (pcilib_register_context_t *)malloc(PCILIB_DEFAULT_REGISTER_SPACE * sizeof(pcilib_register_context_t));
	
	if ((!ctx->registers)||(!ctx->register_ctx)) {
	    pcilib_error("Error allocating memory for register model");
	    pcilib_close(ctx);
	    return NULL;
	}
	
	memset(ctx->registers, 0, sizeof(pcilib_register_description_t));
	memset(ctx->banks, 0, sizeof(pcilib_register_bank_description_t));
	memset(ctx->ranges, 0, sizeof(pcilib_register_range_t));

	memset(ctx->register_ctx, 0, PCILIB_DEFAULT_REGISTER_SPACE * sizeof(pcilib_register_context_t));

	
	for (i = 0; pcilib_protocols[i].api; i++);
	memcpy(ctx->protocols, pcilib_protocols, i * sizeof(pcilib_register_protocol_description_t));
	ctx->num_protocols = i;

	err = pcilib_detect_model(ctx, model);
	if (err) {
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

	if(banks){
		pcilib_xml_initialize_banks(doc,banks);
		pcilib_add_register_banks(ctx,number_banks,banks);
	}else pcilib_error("no memory for banks");

	if(registers){
		pcilib_xml_initialize_registers(doc,registers);
		pcilib_xml_arrange_registers(registers,number_registers);
	    pcilib_add_registers(ctx,number_registers,registers);
	}else pcilib_error("no memory for registers");
	
	ctx->model_info.registers = ctx->registers;
	ctx->model_info.banks = ctx->banks;
	ctx->model_info.protocols = ctx->protocols;
	ctx->model_info.ranges = ctx->ranges;


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


int pcilib_map_data_space(pcilib_t *ctx, uintptr_t addr) {
    int err;
    pcilib_bar_t i;
    
    if (!ctx->data_bar_mapped) {
        const pcilib_board_info_t *board_info = pcilib_get_board_info(ctx);
	if (!board_info) return PCILIB_ERROR_FAILED;

	err = pcilib_map_register_space(ctx);
	if (err) {
	    pcilib_error("Error mapping register space");
	    return err;
	}
	
	int data_bar = -1;
	
	for (i = 0; i < PCILIB_MAX_BARS; i++) {
	    if ((ctx->bar_space[i])||(!board_info->bar_length[i])) continue;
	    
	    if (addr) {
	        if (board_info->bar_start[i] == addr) {
		    data_bar = i;
		    break;
		}
	    } else {
		if (data_bar >= 0) {
		    data_bar = -1;
		    break;
		}
		
		data_bar = i;
	    }
	}
	    

	if (data_bar < 0) {
	    if (addr) pcilib_error("Unable to find the specified data space (%lx)", addr);
	    else pcilib_error("Unable to find the data space");
	    return PCILIB_ERROR_NOTFOUND;
	}
	
	ctx->data_bar = data_bar;
	
	if (!ctx->bar_space[data_bar]) {
	    char *data_space = pcilib_map_bar(ctx, data_bar);
	    if (data_space) ctx->bar_space[data_bar] = data_space;
	    else {
	        pcilib_error("Unable to map the data space");
		return PCILIB_ERROR_FAILED;
	    }
	}
	
	ctx->data_bar_mapped = 0;
    }
    
    return 0;
}
	
char *pcilib_resolve_register_address(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr) {
    if (bar == PCILIB_BAR_DETECT) {
      printf("bar = PCILIB_BAR_DETECT\n");
	    // First checking the default register bar
	size_t offset = addr - ctx->board_info.bar_start[ctx->reg_bar];
	if ((addr > ctx->board_info.bar_start[ctx->reg_bar])&&(offset < ctx->board_info.bar_length[ctx->reg_bar])) {
	    if (!ctx->bar_space[ctx->reg_bar]) {
		pcilib_error("The register bar is not mapped");
		return NULL;
	    }

	    return ctx->bar_space[ctx->reg_bar] + offset + (ctx->board_info.bar_start[ctx->reg_bar] & ctx->page_mask);
	}
	    
	    // Otherwise trying to detect
	bar = pcilib_detect_bar(ctx, addr, 1);
	if (bar != PCILIB_BAR_INVALID) {
	  printf("bar pas ainvalid\n");
	    size_t offset = addr - ctx->board_info.bar_start[bar];
	    if ((offset < ctx->board_info.bar_length[bar])&&(ctx->bar_space[bar])) {
		if (!ctx->bar_space[bar]) {
		    pcilib_error("The requested bar (%i) is not mapped", bar);
		    return NULL;
		}
		return ctx->bar_space[bar] + offset + (ctx->board_info.bar_start[bar] & ctx->page_mask);
	    }
	}
    } else {
	printf("bar internal :%i\n",bar);
	//      printf("bar invalid\n");
	if (!ctx->bar_space[bar]) {
	    pcilib_error("The requested bar (%i) is not mapped", bar);
	    return NULL;
	}
	
	if (addr < ctx->board_info.bar_length[bar]) {
	  printf("path1\n");
	  // printf("apres: %s\n",ctx->bar_space[bar] + addr);
	    return ctx->bar_space[bar] + addr + (ctx->board_info.bar_start[bar] & ctx->page_mask);
	}
	
	if ((addr >= ctx->board_info.bar_start[bar])&&(addr < (ctx->board_info.bar_start[bar] + ctx->board_info.bar_length[ctx->reg_bar]))) {
	  printf("path2\n");
	    return ctx->bar_space[bar] + (addr - ctx->board_info.bar_start[bar]) + (ctx->board_info.bar_start[bar] & ctx->page_mask);
	}
    }

    return NULL;
}

char *pcilib_resolve_data_space(pcilib_t *ctx, uintptr_t addr, size_t *size) {
    int err;
    
    err = pcilib_map_data_space(ctx, addr);
    if (err) {
	pcilib_error("Failed to map the specified address space (%lx)", addr);
	return NULL;
    }
    
    if (size) *size = ctx->board_info.bar_length[ctx->data_bar];
    
    return ctx->bar_space[ctx->data_bar] + (ctx->board_info.bar_start[ctx->data_bar] & ctx->page_mask);
}


void pcilib_close(pcilib_t *ctx) {
    pcilib_bar_t i;

    if (ctx) {
	const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
	const pcilib_event_api_description_t *eapi = model_info->api;
	const pcilib_dma_api_description_t *dapi = ctx->dma.api;
	
        if ((eapi)&&(eapi->free)) eapi->free(ctx->event_ctx);
        if ((dapi)&&(dapi->free)) dapi->free(ctx->dma_ctx);

	pcilib_free_register_banks(ctx);
	
	if (ctx->register_ctx)
	    free(ctx->register_ctx);

	if (ctx->event_plugin)
	    pcilib_plugin_close(ctx->event_plugin);
	
	if (ctx->kmem_list) {
	    pcilib_warning("Not all kernel buffers are properly cleaned");
	
	    while (ctx->kmem_list) {
		pcilib_free_kernel_memory(ctx, ctx->kmem_list, 0);
	    }
	}
	
	for (i = 0; i < PCILIB_MAX_BARS; i++) {
	    if (ctx->bar_space[i]) {
		char *ptr = ctx->bar_space[i];
		ctx->bar_space[i] = NULL;
		pcilib_unmap_bar(ctx, i, ptr);
	    }
	}
	
	if (ctx->pci_cfg_space_fd >= 0)
	    close(ctx->pci_cfg_space_fd);

	if (ctx->registers)
	    free(ctx->registers);
	
	if (ctx->model)
	    free(ctx->model);
	
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
    if (size != 256) {
	pcilib_error("Failed to read PCI configuration from sysfs");
	return PCILIB_ERROR_FAILED;
    }

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

    while ((cap_offset)&&(cap_offset < 256)) {
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
