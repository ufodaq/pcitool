#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <alloca.h>
#include <arpa/inet.h>

#include <getopt.h>

#include "pci.h"
#include "ipecamera.h"
#include "tools.h"
#include "kernel.h"

/* defines */
#define MAX_KBUF 14
//#define BIGBUFSIZE (512*1024*1024)
#define BIGBUFSIZE (1024*1024)


#define DEFAULT_FPGA_DEVICE "/dev/fpga0"

#define LINE_WIDTH 80
#define SEPARATOR_WIDTH 2
#define BLOCK_SEPARATOR_WIDTH 2
#define BLOCK_SIZE 8
#define BENCHMARK_ITERATIONS 128

#define isnumber pcilib_isnumber
#define isxnumber pcilib_isxnumber


typedef uint8_t access_t;

typedef enum {
    MODE_INVALID,
    MODE_INFO,
    MODE_LIST,
    MODE_BENCHMARK,
    MODE_READ,
    MODE_READ_REGISTER,
    MODE_WRITE,
    MODE_WRITE_REGISTER
} MODE;

typedef enum {
    OPT_DEVICE = 'd',
    OPT_MODEL = 'm',
    OPT_BAR = 'b',
    OPT_ACCESS = 'a',
    OPT_ENDIANESS = 'e',
    OPT_SIZE = 's',
    OPT_INFO = 'i',
    OPT_BENCHMARK = 'p',
    OPT_LIST = 'l',
    OPT_READ = 'r',
    OPT_WRITE = 'w',
    OPT_HELP = 'h',
} OPTIONS;

static struct option long_options[] = {
    {"device",			required_argument, 0, OPT_DEVICE },
    {"model",			required_argument, 0, OPT_MODEL },
    {"bar",			required_argument, 0, OPT_BAR },
    {"access",			required_argument, 0, OPT_ACCESS },
    {"endianess",		required_argument, 0, OPT_ENDIANESS },
    {"size",			required_argument, 0, OPT_SIZE },
    {"info",			no_argument, 0, OPT_INFO },
    {"list",			no_argument, 0, OPT_LIST },
    {"benchmark",		no_argument, 0, OPT_BENCHMARK },
    {"read",			optional_argument, 0, OPT_READ },
    {"write",			optional_argument, 0, OPT_WRITE },
    {"help",			no_argument, 0, OPT_HELP },
    { 0, 0, 0, 0 }
};


void Usage(int argc, char *argv[], const char *format, ...) {
    if (format) {
	va_list ap;
    
	va_start(ap, format);
	printf("Error %i: ", errno);
	vprintf(format, ap);
	printf("\n");
	va_end(ap);
    
        printf("\n");
    }


    printf(
"Usage:\n"
" %s <mode> [options] [hex data]\n"
"  Modes:\n"
"	-i		- Device Info\n"
"	-l		- List Data Banks & Registers\n"
"	-p		- Performance Evaluation\n"
"	-r <addr|reg>	- Read Data/Register\n"
"	-w <addr|reg>	- Write Data/Register\n"
"	--help		- Help message\n"
"\n"
"  Addressing:\n"
"	-d <device>	- FPGA device (/dev/fpga0)\n"
"	-m <model>	- Memory model (autodetected)\n"
"	   pci		- Plain\n"
"	   ipecamera	- IPE Camera\n"
"	-b <bank>	- Data/Register bank (autodetected)\n"
"\n"
"  Options:\n"
"	-s <size>	- Number of words (default: 1)\n"
"	-a <bitness>	- Bits per word (default: 32)\n"
"	-e <l|b>	- Endianess Little/Big (default: host)\n"
"\n\n",
argv[0]);

    exit(0);
}

void Error(const char *format, ...) {
    va_list ap;
    
    va_start(ap, format);
    printf("Error %i: ", errno);
    vprintf(format, ap);
    if (errno) printf("\n errno: %s", strerror(errno));
    printf("\n\n");
    va_end(ap);
    
    exit(-1);
}


void List(pcilib_t *handle, pcilib_model_t model, const char *bank) {
    int i;
    pcilib_register_bank_description_t *banks;
    pcilib_register_description_t *registers;

    const pci_board_info *board_info = pcilib_get_board_info(handle);

    for (i = 0; i < PCILIB_MAX_BANKS; i++) {
	if (board_info->bar_length[i] > 0) {
	    printf(" BAR %d - ", i);

	    switch ( board_info->bar_flags[i]&IORESOURCE_TYPE_BITS) {
		case IORESOURCE_IO:  printf(" IO"); break;
		case IORESOURCE_MEM: printf("MEM"); break;
		case IORESOURCE_IRQ: printf("IRQ"); break;
		case IORESOURCE_DMA: printf("DMA"); break;
	    }
	    
	    if (board_info->bar_flags[i]&IORESOURCE_MEM_64) printf("64");
	    else printf("32");
	    
	    printf(", Start: 0x%08lx, Length: 0x%8lx, Flags: 0x%08lx\n", board_info->bar_start[i], board_info->bar_length[i], board_info->bar_flags[i] );
	}
    }
    printf("\n");

    if ((bank)&&(bank != (char*)-1)) banks = NULL;
    else banks = pcilib_model[model].banks;
    
    if (banks) {
	printf("Banks: \n");
	for (i = 0; banks[i].access; i++) {
	    printf(" 0x%02x %s", banks[i].addr, banks[i].name);
	    if ((banks[i].description)&&(banks[i].description[0])) {
		printf(": %s", banks[i].description);
	    }
	    printf("\n");
	}
	printf("\n");
    }
    
    if (bank == (char*)-1) registers = NULL;
    else registers = pcilib_model[model].registers;
    
    if (registers) {
        pcilib_register_bank_addr_t bank_addr;
	if (bank) {
	    pcilib_register_bank_t bank_id = pcilib_find_bank(handle, bank);
	    pcilib_register_bank_description_t *b = pcilib_model[model].banks + bank_id;

	    bank_addr = b->addr;
	    if (b->description) printf("%s:\n", b->description);
	    else if (b->name) printf("Registers of bank %s:\n", b->name);
	    else printf("Registers of bank 0x%x:\n", b->addr);
	} else {
	    printf("Registers: \n");
	}
	for (i = 0; registers[i].bits; i++) {
	    const char *mode;
	    
	    if ((bank)&&(registers[i].bank != bank_addr)) continue;

	    if (registers[i].mode == PCILIB_REGISTER_RW) mode = "RW";
	    else if (registers[i].mode == PCILIB_REGISTER_R) mode = "R ";
	    else if (registers[i].mode == PCILIB_REGISTER_W) mode = " W";
	    else mode = "  ";
	    
	    printf(" 0x%02x (%2i %s) %s", registers[i].addr, registers[i].bits, mode, registers[i].name);
	    if ((registers[i].description)&&(registers[i].description[0])) {
		printf(": %s", registers[i].description);
	    }
	    printf("\n");

	}
	printf("\n");
    }
}

void Info(pcilib_t *handle, pcilib_model_t model) {
    const pci_board_info *board_info = pcilib_get_board_info(handle);

    printf("Vendor: %x, Device: %x, Interrupt Pin: %i, Interrupt Line: %i\n", board_info->vendor_id, board_info->device_id, board_info->interrupt_pin, board_info->interrupt_line);
    List(handle, model, (char*)-1);
}


int Benchmark(pcilib_t *handle, pcilib_bar_t bar) {
    int err;
    int i, errors;
    void *data, *buf, *check;
    struct timeval start, end;
    unsigned long time;
    unsigned int size, max_size;
    
    const pci_board_info *board_info = pcilib_get_board_info(handle);
		
    if (bar < 0) {
	unsigned long maxlength = 0;
	for (i = 0; i < PCILIB_MAX_BANKS; i++) {
	    if (board_info->bar_length[i] > maxlength) {
		maxlength = board_info->bar_length[i];
		bar = i;
	    }
	}
	
	if (bar < 0) Error("Data banks are not available");
    }
    

    max_size = board_info->bar_length[bar];
    
    err = posix_memalign( (void**)&buf, 256, max_size );
    if (!err) err = posix_memalign( (void**)&check, 256, max_size );
    if ((err)||(!buf)||(!check)) Error("Allocation of %i bytes of memory have failed", max_size);

    printf("Transfer time (Bank: %i):\n", bar);    
    data = pcilib_map_bar(handle, bar);
    
    for (size = 4 ; size < max_size; size *= 8) {
	gettimeofday(&start,NULL);
	for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
	    pcilib_memcpy(buf, data, size);
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf("%8i bytes - read: %8.2lf MB/s", size, 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));
	
	fflush(0);

	gettimeofday(&start,NULL);
	for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
	    pcilib_memcpy(data, buf, size);
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf(", write: %8.2lf MB/s\n", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));
    }
    
    pcilib_unmap_bar(handle, bar, data);

    printf("\n\nOpen-Transfer-Close time: \n");
    
    for (size = 4 ; size < max_size; size *= 8) {
	gettimeofday(&start,NULL);
	for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
	    pcilib_read(handle, bar, 0, size, buf);
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf("%8i bytes - read: %8.2lf MB/s", size, 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));
	
	fflush(0);

	gettimeofday(&start,NULL);
	for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
	    pcilib_write(handle, bar, 0, size, buf);
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf(", write: %8.2lf MB/s", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));

	gettimeofday(&start,NULL);
	for (i = 0, errors = 0; i < BENCHMARK_ITERATIONS; i++) {
	    pcilib_write(handle, bar, 0, size, buf);
	    pcilib_read(handle, bar, 0, size, check);
	    if (memcmp(buf, check, size)) ++errors;
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf(", write-verify: %8.2lf MB/s", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));
	if (errors) printf(", errors: %u of %u", errors, BENCHMARK_ITERATIONS);
	printf("\n");
    }
    
    printf("\n\n");

    free(check);
    free(buf);
}

#define pci2host16(endianess, value) endianess?


int ReadData(pcilib_t *handle, pcilib_bar_t bar, uintptr_t addr, size_t n, access_t access, int endianess) {
    void *buf;
    int i, err;
    int size = n * abs(access);
    int block_width, blocks_per_line;
    int numbers_per_block, numbers_per_line; 
    
    numbers_per_block = BLOCK_SIZE / access;

    block_width = numbers_per_block * ((access * 2) +  SEPARATOR_WIDTH);
    blocks_per_line = (LINE_WIDTH - 10) / (block_width +  BLOCK_SEPARATOR_WIDTH);
    if ((blocks_per_line > 1)&&(blocks_per_line % 2)) --blocks_per_line;
    numbers_per_line = blocks_per_line * numbers_per_block;

//    buf = alloca(size);
    err = posix_memalign( (void**)&buf, 256, size );
    if ((err)||(!buf)) Error("Allocation of %i bytes of memory have failed", size);
    
    pcilib_read(handle, bar, addr, size, buf);
    if (endianess) pcilib_swap(buf, buf, abs(access), n);

    for (i = 0; i < n; i++) {
	if (i) {
	    if (i%numbers_per_line == 0) printf("\n");
	    else {
		printf("%*s", SEPARATOR_WIDTH, "");
		if (i%numbers_per_block == 0) printf("%*s", BLOCK_SEPARATOR_WIDTH, "");
	    }
	}
	    
	if (i%numbers_per_line == 0) printf("%8lx:  ", addr + i * abs(access));

	switch (access) {
	    case 1: printf("%0*hhx", access * 2, ((uint8_t*)buf)[i]); break;
	    case 2: printf("%0*hx", access * 2, ((uint16_t*)buf)[i]); break;
	    case 4: printf("%0*x", access * 2, ((uint32_t*)buf)[i]); break;
	    case 8: printf("%0*lx", access * 2, ((uint64_t*)buf)[i]); break;
	}
    }
    printf("\n\n");

    
    free(buf);
}

int ReadRegister(pcilib_t *handle, pcilib_model_t model, const char *bank, const char *reg) {
    int err;
    int i;

    pcilib_register_value_t value;
    
    if (reg) {
        err = pcilib_read_register(handle, bank, reg, &value);
        if (err) printf("Error reading register %s\n", reg);
        else printf("%s = %i\n", reg, value);
    } else {
	pcilib_register_bank_t bank_id;
	pcilib_register_bank_addr_t bank_addr;
        
	pcilib_register_description_t *registers = pcilib_model[model].registers;
	
	if (registers) {
	    if (bank) {
		bank_id = pcilib_find_bank(handle, bank);
		bank_addr = pcilib_model[model].banks[bank_id].addr;
	    }
	    
	    printf("Registers:\n");
	    for (i = 0; registers[i].bits; i++) {
		if ((registers[i].mode & PCILIB_REGISTER_R)&&((!bank)||(registers[i].bank == bank_addr))) {
		    err = pcilib_read_register_by_id(handle, i, &value);
		    if (err) printf(" %s = error reading value [%i]", registers[i].name, registers[i].defvalue);
	    	    else printf(" %s = %i [%i]", registers[i].name, value, registers[i].defvalue);
		}
		printf("\n");
	    }
	} else {
	    printf("No registers");
	}
	printf("\n");
    }
}

int ReadRegisterRange(pcilib_t *handle, pcilib_model_t model, const char *bank, uintptr_t addr, size_t n) {
    int err;
    int i;

    pcilib_register_bank_description_t *banks = pcilib_model[model].banks;
    pcilib_register_bank_t bank_id = pcilib_find_bank(handle, bank);

    if (bank_id == PCILIB_REGISTER_BANK_INVALID) {
	if (bank) Error("Invalid register bank is specified (%s)", bank);
	else Error("Register bank should be specified");
    }

    int access = banks[bank_id].access / 8;
    int size = n * abs(access);
    int block_width, blocks_per_line;
    int numbers_per_block, numbers_per_line; 
    
    numbers_per_block = BLOCK_SIZE / access;

    block_width = numbers_per_block * ((access * 2) +  SEPARATOR_WIDTH);
    blocks_per_line = (LINE_WIDTH - 6) / (block_width +  BLOCK_SEPARATOR_WIDTH);
    if ((blocks_per_line > 1)&&(blocks_per_line % 2)) --blocks_per_line;
    numbers_per_line = blocks_per_line * numbers_per_block;


    pcilib_register_value_t buf[n];
    err = pcilib_read_register_space(handle, bank, addr, n, buf);
    if (err) Error("Error reading register space for bank \"%s\" at address %lx, size %lu", bank?bank:"default", addr, n);


    for (i = 0; i < n; i++) {
	if (i) {
	    if (i%numbers_per_line == 0) printf("\n");
	    else {
		printf("%*s", SEPARATOR_WIDTH, "");
		if (i%numbers_per_block == 0) printf("%*s", BLOCK_SEPARATOR_WIDTH, "");
	    }
	}
	    
	if (i%numbers_per_line == 0) printf("%4lx:  ", addr + i);
	printf("%0*lx", access * 2, buf[i]);
    }
    printf("\n\n");
}

int WriteData(pcilib_t *handle, pcilib_bar_t bar, uintptr_t addr, size_t n, access_t access, int endianess, char ** data) {
    void *buf, *check;
    int res, i, err;
    int size = n * abs(access);

    err = posix_memalign( (void**)&buf, 256, size );
    if (!err) err = posix_memalign( (void**)&check, 256, size );
    if ((err)||(!buf)||(!check)) Error("Allocation of %i bytes of memory have failed", size);

    for (i = 0; i < n; i++) {
	switch (access) {
	    case 1: res = sscanf(data[i], "%hhx", ((uint8_t*)buf)+i); break;
	    case 2: res = sscanf(data[i], "%hx", ((uint16_t*)buf)+i); break;
	    case 4: res = sscanf(data[i], "%x", ((uint32_t*)buf)+i); break;
	    case 8: res = sscanf(data[i], "%lx", ((uint64_t*)buf)+i); break;
	}
	if ((res != 1)||(!isxnumber(data[i]))) Error("Can't parse data value at poition %i, (%s) is not valid hex number", i, data[i]);
    }

    if (endianess) pcilib_swap(buf, buf, abs(access), n);
    pcilib_write(handle, bar, addr, size, buf);
    pcilib_read(handle, bar, addr, size, check);
    
    if (memcmp(buf, check, size)) {
	printf("Write failed: the data written and read differ, the foolowing is read back:\n");
        if (endianess) pcilib_swap(check, check, abs(access), n);
	ReadData(handle, bar, addr, n, access, endianess);
	exit(-1);
    }

    free(check);
    free(buf);
}

int WriteRegisterRange(pcilib_t *handle, pcilib_model_t model, const char *bank, uintptr_t addr, size_t n, char ** data) {
    pcilib_register_value_t *buf, *check;
    int res, i, err;
    unsigned long value;
    int size = n * sizeof(pcilib_register_value_t);

    err = posix_memalign( (void**)&buf, 256, size );
    if (!err) err = posix_memalign( (void**)&check, 256, size );
    if ((err)||(!buf)||(!check)) Error("Allocation of %i bytes of memory have failed", size);

    for (i = 0; i < n; i++) {
	res = sscanf(data[i], "%lx", &value);
	if ((res != 1)||(!isxnumber(data[i]))) Error("Can't parse data value at poition %i, (%s) is not valid hex number", i, data[i]);
	buf[i] = value;
    }

    err = pcilib_write_register_space(handle, bank, addr, n, buf);
    if (err) Error("Error writting register space for bank \"%s\" at address %lx, size %lu", bank?bank:"default", addr, n);
    
    err = pcilib_read_register_space(handle, bank, addr, n, check);
    if (err) Error("Error reading register space for bank \"%s\" at address %lx, size %lu", bank?bank:"default", addr, n);

    if (memcmp(buf, check, size)) {
	printf("Write failed: the data written and read differ, the foolowing is read back:\n");
	ReadRegisterRange(handle, model, bank, addr, n);
	exit(-1);
    }

    free(check);
    free(buf);

}

int WriteRegister(pcilib_t *handle, pcilib_model_t model, const char *bank, const char *reg, char ** data) {
    int err;
    int i;

    unsigned long val;
    pcilib_register_value_t value;
    
    if ((!isnumber(*data))||(sscanf(*data, "%li", &val) != 1)) {
	Error("Can't parse data value (%s) is not valid decimal number", data[i]);
    }

    value = val;
    
    err = pcilib_write_register(handle, bank, reg, value);
    if (err) Error("Error writting register %s\n", reg);

    err = pcilib_read_register(handle, bank, reg, &value);
    if (err) Error("Error reading back register %s for verification\n", reg);

    if (val != value) {
	Error("Failed to write register %s: %lu is written and %lu is read back", reg, val, value);
    } else {
	printf("%s = %i\n", reg, value);
    }
    
    return 0;
}


int main(int argc, char **argv) {
    int i;
    long itmp;
    unsigned char c;

    pcilib_model_t model = PCILIB_MODEL_DETECT;
    MODE mode = MODE_INVALID;
    const char *fpga_device = DEFAULT_FPGA_DEVICE;
    pcilib_bar_t bar = PCILIB_BAR_DETECT;
    const char *addr = NULL;
    const char *reg = NULL;
    const char *bank = NULL;
    uintptr_t start = -1;
    size_t size = 1;
    access_t access = 4;
    int skip = 0;
    int endianess = 0;

    pcilib_t *handle;
    
    while ((c = getopt_long(argc, argv, "hilpr::w::d:m:b:a:s:e:", long_options, NULL)) != (unsigned char)-1) {
	extern int optind;
	switch (c) {
	    case OPT_HELP:
		Usage(argc, argv, NULL);
	    break;
	    case OPT_INFO:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_INFO;
	    break;
	    case OPT_LIST:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_LIST;
	    break;
	    case OPT_BENCHMARK:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_BENCHMARK;
	    break;
	    case OPT_READ:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_READ;
		if (optarg) addr = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) addr = argv[optind++];
	    break;
	    case OPT_WRITE:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_WRITE;
		if (optarg) addr = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) addr = argv[optind++];
	    break;
	    case OPT_DEVICE:
		fpga_device = optarg;
	    break;
	    case OPT_MODEL:
		if (!strcasecmp(optarg, "pci")) model = PCILIB_MODEL_PCI;
		else if (!strcasecmp(optarg, "ipecamera")) model = PCILIB_MODEL_IPECAMERA;
		else Usage(argc, argv, "Invalid memory model (%s) is specified", optarg);\
	    break;
	    case OPT_BAR:
		bank = optarg;
//		if ((sscanf(optarg,"%li", &itmp) != 1)||(itmp < 0)||(itmp >= PCILIB_MAX_BANKS)) Usage(argc, argv, "Invalid data bank (%s) is specified", optarg);
//		else bar = itmp;
	    break;
	    case OPT_ACCESS:
		if ((!isnumber(optarg))||(sscanf(optarg, "%li", &itmp) != 1)) access = 0;
		switch (itmp) {
		    case 8: access = 1; break;
		    case 16: access = 2; break;
		    case 32: access = 4; break;
		    case 64: access = 8; break;
		    default: Usage(argc, argv, "Invalid data width (%s) is specified", optarg);
		}	
	    break;
	    case OPT_SIZE:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &size) != 1))
		    Usage(argc, argv, "Invalid size is specified (%s)", optarg);
	    break;
	    case OPT_ENDIANESS:
		if ((*optarg == 'b')||(*optarg == 'B')) {
		    if (ntohs(1) == 1) endianess = 0;
		    else endianess = 1;
		} else if ((*optarg == 'l')||(*optarg == 'L')) {
		    if (ntohs(1) == 1) endianess = 1;
		    else endianess = 0;
		} else Usage(argc, argv, "Invalid endianess is specified (%s)", optarg);
		
	    break;
	    default:
		Usage(argc, argv, "Unknown option (%s)", argv[optind]);
	}
    }

    if (mode == MODE_INVALID) {
	if (argc > 1) Usage(argc, argv, "Operation is not specified");
	else Usage(argc, argv, NULL);
    }

    pcilib_set_error_handler(&Error);

    handle = pcilib_open(fpga_device, model);
    if (handle < 0) Error("Failed to open FPGA device: %s", fpga_device);

    model = pcilib_get_model(handle);

    switch (mode) {
     case MODE_WRITE:
        if (!addr) Usage(argc, argv, "The address is not specified");
        if ((argc - optind) != size) Usage(argc, argv, "The %i data values is specified, but %i required", argc - optind, size);
     break;
     case MODE_READ:
        if (!addr) {
	    if (model == PCILIB_MODEL_PCI) {
		Usage(argc, argv, "The address is not specified");
	    } else ++mode;
	}
     break;
     default:
        if (argc > optind) Usage(argc, argv, "Invalid non-option parameters are supplied");
    }

    if (addr) {
	if ((isxnumber(addr))&&(sscanf(addr, "%lx", &start) == 1)) {
		// check if the address in the register range
	    pcilib_register_range_t *ranges =  pcilib_model[model].ranges;

	    for (i = 0; ranges[i].start != ranges[i].end; i++) 
		if ((start >= ranges[i].start)&&(start <= ranges[i].end)) break;
		
		// register access in plain mode
	    if (ranges[i].start != ranges[i].end) ++mode;
	} else {
	    if (pcilib_find_register(handle, bank, addr) == PCILIB_REGISTER_INVALID) {
	        Usage(argc, argv, "Invalid address (%s) is specified", addr);
	    } else {
	        reg = addr;
		++mode;
	    }
	} 
    }
    
    if (bank) {
	switch (mode) {
	    case MODE_BENCHMARK:
	    case MODE_READ:
	    case MODE_WRITE:
		if ((!isnumber(bank))||(sscanf(bank,"%li", &itmp) != 1)||(itmp < 0)||(itmp >= PCILIB_MAX_BANKS)) 
		    Usage(argc, argv, "Invalid data bank (%s) is specified", bank);
		else bar = itmp;
	    break;
	    default:
		if (pcilib_find_bank(handle, bank) == PCILIB_REGISTER_BANK_INVALID)
		    Usage(argc, argv, "Invalid data bank (%s) is specified", bank);
	}
    }

    switch (mode) {
     case MODE_INFO:
        Info(handle, model);
     break;
     case MODE_LIST:
        List(handle, model, bank);
     break;
     case MODE_BENCHMARK:
        Benchmark(handle, bar);
     break;
     case MODE_READ:
        if (addr) {
	    ReadData(handle, bar, start, size, access, endianess);
	} else {
	    Error("Address to read is not specified");
	}
     break;
     case MODE_READ_REGISTER:
        if ((reg)||(!addr)) ReadRegister(handle, model, bank, reg);
	else ReadRegisterRange(handle, model, bank, start, size);
     break;
     case MODE_WRITE:
	WriteData(handle, bar, start, size, access, endianess, argv + optind);
     break;
     case MODE_WRITE_REGISTER:
        if (reg) WriteRegister(handle, model, bank, reg, argv + optind);
	else WriteRegisterRange(handle, model, bank, start, size, argv + optind);
     break;
    }

    pcilib_close(handle);
}
