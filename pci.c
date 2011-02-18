#define _PCILIB_PCI_C

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#include "driver/pciDriver.h"

#include "kernel.h"
#include "tools.h"

#include "pci.h"
#include "ipecamera.h"

static pci_board_info board_info;
static page_mask = -1;


static void pcilib_print_error(const char *msg, ...) {
    va_list va;
    
    va_start(va, msg);
    vprintf(msg, va);
    va_end(va);
}

static void (*Error)(const char *msg, ...) = pcilib_print_error;

int pcilib_open(const char *device) {
    int handle = open(device, O_RDWR);
    return handle;
}

void pcilib_close(int handle) {
    close(handle);
}

int pcilib_set_error_handler(void (*err)(const char *msg, ...)) {
    Error = err;
}

const pci_board_info *pcilib_get_board_info(int handle) {
    int ret;
    
    if (page_mask < 0) {
	ret = ioctl( handle, PCIDRIVER_IOC_PCI_INFO, &board_info );
	if (ret) Error("PCIDRIVER_IOC_PCI_INFO ioctl have failed");
	
	page_mask = get_page_mask();
    }
    
    return &board_info;
}


pcilib_model_t pcilib_detect_model(int handle) {
	unsigned short vendor_id;
	unsigned short device_id;

    pcilib_get_board_info(handle);

    if ((board_info.vendor_id == PCIE_XILINX_VENDOR_ID)&&(board_info.device_id ==  PCIE_IPECAMERA_DEVICE_ID)) return PCILIB_MODEL_IPECAMERA;
    return PCILIB_MODEL_PCI;
}

static int pcilib_detect_bar(int handle, unsigned long addr, int size) {
    int ret,i;
	
    pcilib_get_board_info(handle);
		
    for (i = 0; i < PCILIB_MAX_BANKS; i++) {
	if ((addr >= board_info.bar_start[i])&&((board_info.bar_start[i] + board_info.bar_length[i]) >= (addr + size))) return i;
    }
	
    return -1;
}

static void *pcilib_detect_address(int handle, int *bar, unsigned long *addr, int size) {
    if (*bar < 0) {
	*bar = pcilib_detect_bar(handle, *addr, size);
	if (*bar < 0) Error("The requested data block at address 0x%x with size 0x%x does not belongs to any available memory bank", *addr, size);
    } else {
	pcilib_get_board_info(handle);
	
	if ((*addr < board_info.bar_start[*bar])||((board_info.bar_start[*bar] + board_info.bar_length[*bar]) < (((uintptr_t)*addr) + size))) {
	    if ((board_info.bar_length[*bar]) >= (((uintptr_t)*addr) + size)) 
		*addr += board_info.bar_start[*bar];
	    else
		Error("The requested data block at address 0x%x with size 0x%x does not belong the specified memory bank (Bar %i: starting at 0x%x with size 0x%x)", *addr, size, *bar, board_info.bar_start[*bar], board_info.bar_length[*bar]);
	}
    }
    
    *addr -= board_info.bar_start[*bar];
    *addr += board_info.bar_start[*bar] & page_mask;
}


#ifdef FILE_IO
int file_io_handle;
#endif /* FILE_IO */

void *pcilib_map_bar(int handle, int bar) {
    void *res;
    int ret; 

    pcilib_get_board_info(handle);
    
    ret = ioctl( handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_PCI );
    if (ret) Error("PCIDRIVER_IOC_MMAP_MODE ioctl have failed", bar);

    ret = ioctl( handle, PCIDRIVER_IOC_MMAP_AREA, PCIDRIVER_BAR0 + bar );
    if (ret) Error("PCIDRIVER_IOC_MMAP_AREA ioctl have failed for bank %i", bar);

#ifdef FILE_IO
    file_io_handle = open("/root/drivers/pciDriver/data", O_RDWR);
    res = mmap( 0, board_info.bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, file_io_handle, 0 );
#else
    res = mmap( 0, board_info.bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, handle, 0 );
#endif
    if ((!res)||(res == MAP_FAILED)) Error("Failed to mmap data bank %i", bar);

    
    return res;
}

void pcilib_unmap_bar(int handle, int bar, void *data) {
    munmap(data, board_info.bar_length[bar]);
#ifdef FILE_IO
    close(file_io_handle);
#endif
}

int pcilib_read(void *buf, int handle, int bar, unsigned long addr, int size) {
    int i;
    void *data;
    unsigned int offset;
    char local_buf[size];
    


    pcilib_detect_address(handle, &bar, &addr, size);
    data = pcilib_map_bar(handle, bar);

/*
    for (i = 0; i < size/4; i++)  {
	((uint32_t*)((char*)data+addr))[i] = 0x100 * i + 1;
    }
*/

    pcilib_memcpy(buf, data + addr, size);
    
    pcilib_unmap_bar(handle, bar, data);    
}

int pcilib_write(void *buf, int handle, int bar, unsigned long addr, int size) {
    int i;
    void *data;
    unsigned int offset;
    char local_buf[size];
    

    pcilib_detect_address(handle, &bar, &addr, size);
    data = pcilib_map_bar(handle, bar);

    pcilib_memcpy(data + addr, buf, size);
    
    pcilib_unmap_bar(handle, bar, data);    
}


int pcilib_find_register(pcilib_model_t model, const char *reg) {
    int i;

    pcilib_register_t *registers =  pcilib_model_description[model].registers;
    
    for (i = 0; registers[i].size; i++) {
	if (!strcasecmp(registers[i].name, reg)) return i;
    }
    
    return -1;
};


