#define _IPECAMERA_MODEL_C
#include <sys/time.h>
#include <assert.h>

#include "../tools.h"
#include "../error.h"
#include "model.h"

#define ADDR_MASK 0x7F00
#define WRITE_BIT 0x8000
#define RETRIES 10

#define READ_READY_BIT 0x20000
#define READ_ERROR_BIT 0x40000

#define ipecamera_datacpy(dst, src, bank)   pcilib_datacpy(dst, src, 4, 1, bank->raw_endianess)

static pcilib_register_value_t ipecamera_bit_mask[9] = { 0, 1, 3, 7, 15, 31, 63, 127, 255 };

int ipecamera_read(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t *value) {
    uint32_t val;
    char *wr, *rd;
    struct timeval start, cur;
    int retries = RETRIES;
    
    assert(addr < 128);
    
    wr =  pcilib_resolve_register_address(ctx, bank->write_addr);
    rd =  pcilib_resolve_register_address(ctx, bank->read_addr);
    if ((!rd)||(!wr)) {
	pcilib_error("Error resolving addresses of read & write registers");
	return PCILIB_ERROR_INVALID_ADDRESS;
    }

retry:
    val = (addr << 8);
    
    //printf("%i %x %p %p\n", addr,  val, wr, rd);
    
    ipecamera_datacpy(wr, &val, bank);
    
    gettimeofday(&start, NULL);

    ipecamera_datacpy(&val, rd, bank);
    while ((val & READ_READY_BIT) == 0) {
        gettimeofday(&cur, NULL);
	if (((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) > PCILIB_REGISTER_TIMEOUT) break;
	
	ipecamera_datacpy(&val, rd, bank);
    }

    if ((val & READ_READY_BIT) == 0) {
	pcilib_error("Timeout reading register value");
	return PCILIB_ERROR_TIMEOUT;
    }
    
    if (val & READ_ERROR_BIT) {
	pcilib_error("Error reading register value");
	return PCILIB_ERROR_FAILED;
    }

    if (((val&ADDR_MASK) >> 8) != addr) {
	if (--retries > 0) {
	    pcilib_warning("Address verification failed during register read, retrying (try %i of %i)...", RETRIES - retries, RETRIES);
	    goto retry;
	}
	pcilib_error("Address verification failed during register read");
	return PCILIB_ERROR_VERIFY;
    }

    
//    printf("%i\n", val&ipecamera_bit_mask[bits]);

    *value = val&ipecamera_bit_mask[bits];

    return 0;
}

int ipecamera_write(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t value) {
    uint32_t val;
    char *wr, *rd;
    struct timeval start, cur;
    int retries = RETRIES;
    
    assert(addr < 128);
    assert(value < 256);
    
    wr =  pcilib_resolve_register_address(ctx, bank->write_addr);
    rd =  pcilib_resolve_register_address(ctx, bank->read_addr);
    if ((!rd)||(!wr)) {
	pcilib_error("Error resolving addresses of read & write registers");
	return PCILIB_ERROR_INVALID_ADDRESS;
    }

retry:
    val = WRITE_BIT|(addr << 8)|(value&0xFF);
    
    //printf("%i %x %p %p\n", addr,  val, wr, rd);
    
    ipecamera_datacpy(wr, &val, bank);
    
    gettimeofday(&start, NULL);

    ipecamera_datacpy(&val, rd, bank);
    while ((val & READ_READY_BIT) == 0) {
        gettimeofday(&cur, NULL);
	if (((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) > PCILIB_REGISTER_TIMEOUT) break;
	
	ipecamera_datacpy(&val, rd, bank);
    }

    if ((val & READ_READY_BIT) == 0) {
	if (--retries > 0) {
	    pcilib_warning("Timeout occured during register write, retrying (try %i of %i)...", RETRIES - retries, RETRIES);
	    goto retry;
	}

	pcilib_error("Timeout writting register value");
	return PCILIB_ERROR_TIMEOUT;
    }
    
    if (val & READ_ERROR_BIT) {
	pcilib_error("Error writting register value");
	return PCILIB_ERROR_FAILED;
    }

    if (((val&ADDR_MASK) >> 8) != addr) {
	if (--retries > 0) {
	    pcilib_warning("Address verification failed during register write, retrying (try %i of %i)...", RETRIES - retries, RETRIES);
	    goto retry;
	}
	pcilib_error("Address verification failed during register write");
	return PCILIB_ERROR_VERIFY;
    }
    
    if ((val&ipecamera_bit_mask[bits]) != value) {
	pcilib_error("Value verification failed during register read (%lu != %lu)", val&ipecamera_bit_mask[bits], value);
	return PCILIB_ERROR_VERIFY;
    }

    //printf("%i\n", val&ipecamera_bit_mask[bits]);

    return 0;
}



