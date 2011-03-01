/*******************************************************************
 * This is a test program for the IOctl interface of the 
 * pciDriver.
 * 
 * $Revision: 1.3 $
 * $Date: 2006-11-17 18:49:01 $
 * 
 *******************************************************************/

/*******************************************************************
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2006/10/16 16:56:09  marcus
 * Added nice comment at the start.
 *
 *******************************************************************/

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
#include <alloca.h>

#include <getopt.h>

#include "pci.h"
#include "ipecamera.h"


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

//#define FILE_IO

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
"	-m <model>	- Memory model\n"
"	   pci		- Plain (default)\n"
"	   ipecamera	- IPE Camera\n"
"	-b <bank>	- Data bank (autodetected)\n"
"\n"
"  Options:\n"
"	-s <size>	- Number of words (default: 1)\n"
"	-a <bitness>	- Bits per word (default: 32)\n"
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


void List(int handle, pcilib_model_t model) {
    int i;
    pcilib_register_t *registers;

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
    
    registers = pcilib_model_description[model].registers;
    
    if (registers) {
	printf("Registers: \n");
	for (i = 0; registers[i].size; i++) {
	    const char *mode;
	    if (registers[i].mode == PCILIB_REGISTER_RW) mode = "RW";
	    else if (registers[i].mode == PCILIB_REGISTER_R) mode = "R ";
	    else if (registers[i].mode == PCILIB_REGISTER_W) mode = " W";
	    else mode = "  ";
	    
	    printf(" 0x%02x (%2i %s) %s", registers[i].id, registers[i].size, mode, registers[i].name);
	    if ((registers[i].description)&&(registers[i].description[0])) {
		printf(": %s", registers[i].description);
	    }
	    printf("\n");

	}
	printf("\n");
    }
}

void Info(int handle, pcilib_model_t model) {
    const pci_board_info *board_info = pcilib_get_board_info(handle);

    printf("Vendor: %x, Device: %x, Interrupt Pin: %i, Interrupt Line: %i\n", board_info->vendor_id, board_info->device_id, board_info->interrupt_pin, board_info->interrupt_line);
    List(handle, model);
}


int Benchmark(int handle, int bar) {
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
	    pcilib_read(buf, handle, bar, 0, size);
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf("%8i bytes - read: %8.2lf MB/s", size, 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));
	
	fflush(0);

	gettimeofday(&start,NULL);
	for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
	    pcilib_write(buf, handle, bar, 0, size);
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf(", write: %8.2lf MB/s", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));

	gettimeofday(&start,NULL);
	for (i = 0, errors = 0; i < BENCHMARK_ITERATIONS; i++) {
	    pcilib_write(buf, handle, bar, 0, size);
	    pcilib_read(check, handle, bar, 0, size);
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


int ReadData(int handle, int bar, unsigned long addr, int n, int access) {
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
    
    pcilib_read(buf, handle, bar, addr, size);

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

int ReadRegister(int handle, pcilib_model_t model, const char *reg) {
    int i;
    
    if (!reg) {
        pcilib_register_t *registers = pcilib_model_description[model].registers;
	
	if (registers) {
	    printf("Registers:\n");
	    for (i = 0; registers[i].size; i++) {
		if (registers[i].mode & PCILIB_REGISTER_R) {
	    	    printf(" %s = %i [%i]", registers[i].name, 0, registers[i].defvalue);
		}
		printf("\n");
	    }
	} else {
	    printf("No registers");
	}
	printf("\n");
    }
}

int ReadRegisterRange(int handle, pcilib_model_t model, int bar, unsigned long addr, int n, int access) {
}

int WriteData(int handle, int bar, unsigned long addr, int n, int access, char ** data) {
    void *buf, *check;
    int res, i, err;
    int size = n * abs(access);
    
    err = posix_memalign( (void**)&buf, 256, size );
    if (!err) {
        err = posix_memalign( (void**)&check, 256, size );
    }
    if ((err)||(!buf)||(!check)) 
        Error("Allocation of %i bytes of memory have failed", size);

    for (i = 0; i < n; i++) {
	switch (access) {
	    case 1: res = sscanf(data[i], "%hhx", ((uint8_t*)buf)+i); break;
	    case 2: res = sscanf(data[i], "%hx", ((uint16_t*)buf)+i); break;
	    case 4: res = sscanf(data[i], "%x", ((uint32_t*)buf)+i); break;
	    case 8: res = sscanf(data[i], "%lx", ((uint64_t*)buf)+i); break;
	}
	
	if (res != 1) Error("Can't parse data value at poition %i, (%s) is not valid hex number", i, data[i]);
    }

    pcilib_write(buf, handle, bar, addr, size);
    pcilib_read(check, handle, bar, addr, size);
    
    if (memcmp(buf, check, size)) {
	printf("Write failed: the data written and read differ, the foolowing is read back:\n");
	ReadData(handle, bar, addr, n, access);
	exit(-1);
    }

    free(check);
    free(buf);
}

int WriteRegisterRange(int handle, pcilib_model_t model, int bar, unsigned long addr, int n, int access, char ** data) {
}

int WriteRegister(int handle, pcilib_model_t model, const char *reg, char ** data) {
}


int main(int argc, char **argv) {
    unsigned char c;

    pcilib_model_t model = (pcilib_model_t)-1;
    MODE mode = MODE_INVALID;
    const char *fpga_device = DEFAULT_FPGA_DEVICE;
    int bar = -1;
    const char *addr = NULL;
    unsigned long start = -1;
    int size = 1;
    int access = 4;
    int skip = 0;

    int i;
    int handle;
    
    const char *reg = NULL;
    
    while ((c = getopt_long(argc, argv, "hilpr::w::d:b:a:s:", long_options, NULL)) != (unsigned char)-1) {
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
		if ((sscanf(optarg,"%u", &bar) != 1)||(bar < 0)||(bar >= PCILIB_MAX_BANKS)) Usage(argc, argv, "Invalid data bank (%s) is specified", optarg);
	    break;
	    case OPT_ACCESS:
		if (sscanf(optarg, "%i", &access) != 1) access = 0;
		switch (access) {
		    case 8: access = 1; break;
		    case 16: access = 2; break;
		    case 32: access = 4; break;
		    case 64: access = 8; break;
		    default: Usage(argc, argv, "Invalid data width (%s) is specified", optarg);
		}	
	    break;
	    case OPT_SIZE:
		if (sscanf(optarg, "%u", &size) != 1)
		    Usage(argc, argv, "Invalid size is specified (%s)", optarg);
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

    handle = pcilib_open(fpga_device);
    if (handle < 0) Error("Failed to open FPGA device: %s", fpga_device);

    if (model == (pcilib_model_t)-1) {
	model = pcilib_detect_model(handle);
    }

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
	if (sscanf(addr, "%lx", &start) == 1) {
		// check if the address in the register range
	    pcilib_register_range_t *ranges =  pcilib_model_description[model].ranges;

	    for (i = 0; ranges[i].start != ranges[i].end; i++) 
		if ((start >= ranges[i].start)&&(start <= ranges[i].end)) break;
		
		// register access in plain mode
	    if (ranges[i].start != ranges[i].end) ++mode;
	} else {
	    if (pcilib_find_register(model, addr) >= 0) {
	        reg = addr;
		++mode;
	    } else {
	        Usage(argc, argv, "Invalid address (%s) is specified", addr);
	    }
	} 
    }

    
    switch (mode) {
     case MODE_INFO:
        Info(handle, model);
     break;
     case MODE_LIST:
        List(handle, model);
     break;
     case MODE_BENCHMARK:
        Benchmark(handle, bar);
     break;
     case MODE_READ:
        if (addr) {
	    ReadData(handle, bar, start, size, access);
	} else {
	    Error("Address to read is not specified");
	}
     break;
     case MODE_READ_REGISTER:
        if ((reg)||(!addr)) ReadRegister(handle, model, reg);
	else ReadRegisterRange(handle, model, bar, start, size, access);
     break;
     case MODE_WRITE:
	WriteData(handle, bar, start, size, access, argv + optind);
     break;
     case MODE_WRITE_REGISTER:
        if (reg) WriteRegister(handle, model, reg, argv + optind);
	else WriteRegisterRange(handle, model, bar, start, size, access, argv + optind);
     break;
    }

    pcilib_close(handle);
}
