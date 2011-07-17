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
#include <sys/types.h>
#include <dirent.h>

#include <getopt.h>

//#include "pci.h"
#include "tools.h"
#include "kernel.h"
#include "error.h"

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
#define isnumber_n pcilib_isnumber_n
#define isxnumber_n pcilib_isxnumber_n

typedef uint8_t access_t;

typedef enum {
    MODE_INVALID,
    MODE_INFO,
    MODE_LIST,
    MODE_BENCHMARK,
    MODE_READ,
    MODE_READ_REGISTER,
    MODE_WRITE,
    MODE_WRITE_REGISTER,
    MODE_RESET,
    MODE_GRAB,
    MODE_START_DMA,
    MODE_STOP_DMA,
    MODE_WAIT_IRQ,
    MODE_LIST_KMEM,
    MODE_FREE_KMEM
} MODE;

typedef enum {
    ACCESS_BAR,
    ACCESS_DMA,
    ACCESS_FIFO
} ACCESS_MODE;

typedef enum {
    OPT_DEVICE = 'd',
    OPT_MODEL = 'm',
    OPT_BAR = 'b',
    OPT_ACCESS = 'a',
    OPT_ENDIANESS = 'e',
    OPT_SIZE = 's',
    OPT_OUTPUT = 'o',
    OPT_TIMEOUT = 't',
    OPT_INFO = 'i',
    OPT_BENCHMARK = 'p',
    OPT_LIST = 'l',
    OPT_READ = 'r',
    OPT_WRITE = 'w',
    OPT_GRAB = 'g',
    OPT_QUIETE = 'q',
    OPT_RESET = 128,
    OPT_START_DMA,
    OPT_STOP_DMA,
    OPT_WAIT_IRQ,
    OPT_ITERATIONS,
    OPT_LIST_KMEM,
    OPT_FREE_KMEM,
    OPT_HELP = 'h',
} OPTIONS;

static struct option long_options[] = {
    {"device",			required_argument, 0, OPT_DEVICE },
    {"model",			required_argument, 0, OPT_MODEL },
    {"bar",			required_argument, 0, OPT_BAR },
    {"access",			required_argument, 0, OPT_ACCESS },
    {"endianess",		required_argument, 0, OPT_ENDIANESS },
    {"size",			required_argument, 0, OPT_SIZE },
    {"output",			required_argument, 0, OPT_OUTPUT },
    {"timeout",			required_argument, 0, OPT_TIMEOUT },
    {"iterations",		required_argument, 0, OPT_ITERATIONS },
    {"info",			no_argument, 0, OPT_INFO },
    {"list",			no_argument, 0, OPT_LIST },
    {"reset",			no_argument, 0, OPT_RESET },
    {"benchmark",		optional_argument, 0, OPT_BENCHMARK },
    {"read",			optional_argument, 0, OPT_READ },
    {"write",			optional_argument, 0, OPT_WRITE },
    {"grab",			optional_argument, 0, OPT_GRAB },
    {"start-dma",		required_argument, 0, OPT_START_DMA },
    {"stop-dma",		optional_argument, 0, OPT_STOP_DMA },
    {"wait-irq",		optional_argument, 0, OPT_WAIT_IRQ },
    {"list-kernel-memory",	no_argument, 0, OPT_LIST_KMEM },
    {"free-kernel-memory",	optional_argument, 0, OPT_FREE_KMEM },
    {"quiete",			no_argument, 0, OPT_QUIETE },
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
"   -i				- Device Info\n"
"   -l[l]			- List (detailed) Data Banks & Registers\n"
"   -p <barX|dmaX>		- Performance Evaluation\n"
"   -r <addr|reg|dmaX>		- Read Data/Register\n"
"   -w <addr|reg|dmaX>		- Write Data/Register\n"
"   -g [event]			- Grab Event\n"
"   --reset			- Reset board\n"
"   --help			- Help message\n"
"\n"
"  DMA Modes:\n"
"   --start-dma <num>[r|w]	- Start specified DMA engine\n"
"   --stop-dma [num][r|w]	- Stop specified engine or DMA subsystem\n"
"   --wait-irq <source>		- Wait for IRQ\n"
"\n"
"  Kernel Modes:\n"
"   --list-kernel-memory 	- List kernel buffers\n"
"   --free-kernel-memory [use]	- Cleans lost kernel space buffers (DANGEROUS)\n"
"\n"
"  Addressing:\n"
"   -d <device>			- FPGA device (/dev/fpga0)\n"
"   -m <model>			- Memory model (autodetected)\n"
"	pci			- Plain\n"
"	ipecamera		- IPE Camera\n"
"   -b <bank>			- PCI bar, Register bank, or DMA channel\n"
"\n"
"  Options:\n"
"   -s <size>			- Number of words (default: 1)\n"
"   -a [fifo|dma]<bits>		- Access type and bits per word (default: 32)\n"
"   -e <l|b>			- Endianess Little/Big (default: host)\n"
"   -o <file>			- Output to file (default: stdout)\n"
"   -t <timeout> 		- Timeout in microseconds\n"
"\n"
"  Information:\n"
"   -q				- Quiete mode (suppress warnings)\n"
"\n"
"  Data:\n"
"	Data can be specified as sequence of hexdecimal number or\n"
"	a single value prefixed with '*'. In this case it will be\n"
"	replicated the specified amount of times\n"
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

void Silence(const char *format, ...) {
}

void List(pcilib_t *handle, pcilib_model_description_t *model_info, const char *bank, int details) {
    int i;
    pcilib_register_bank_description_t *banks;
    pcilib_register_description_t *registers;
    pcilib_event_description_t *events;

    const pcilib_board_info_t *board_info = pcilib_get_board_info(handle);
    const pcilib_dma_info_t *dma_info = pcilib_get_dma_info(handle);
    
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
    
    if ((dma_info)&&(dma_info->engines)) {
	printf("DMA Engines: \n");
	for (i = 0; dma_info->engines[i]; i++) {
	    pcilib_dma_engine_description_t *engine = dma_info->engines[i];
	    printf(" DMA %2d ", engine->addr);
	    switch (engine->direction) {
		case PCILIB_DMA_FROM_DEVICE:
		    printf("C2S");
		break;
		case PCILIB_DMA_TO_DEVICE:
		    printf("S2C");
		break;
		case PCILIB_DMA_BIDIRECTIONAL:
		    printf("BI ");
		break;
	    }
	    printf(" - Type: ");
	    switch (engine->type) {
		case PCILIB_DMA_TYPE_BLOCK:
		    printf("Block");
		break;
		case PCILIB_DMA_TYPE_PACKET:
		    printf("Packet");
		break;
	    }
	    
	    printf(", Address Width: %02lu bits\n", engine->addr_bits);
	}
	printf("\n");
    }

    if ((bank)&&(bank != (char*)-1)) banks = NULL;
    else banks = model_info->banks;
    
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
    else registers = model_info->registers;
    
    if (registers) {
        pcilib_register_bank_addr_t bank_addr;
	if (bank) {
	    pcilib_register_bank_t bank_id = pcilib_find_bank(handle, bank);
	    pcilib_register_bank_description_t *b = model_info->banks + bank_id;

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
	    if (registers[i].type == PCILIB_REGISTER_BITS) {
		if (!details) continue;
		
		if (registers[i].bits > 1) {
		    printf("    [%2u:%2u] - %s\n", registers[i].offset, registers[i].offset + registers[i].bits, registers[i].name);
		} else {
		    printf("    [   %2u] - %s\n", registers[i].offset, registers[i].name);
		}
		
		continue;
	    }

	    if (registers[i].mode == PCILIB_REGISTER_RW) mode = "RW";
	    else if (registers[i].mode == PCILIB_REGISTER_R) mode = "R ";
	    else if (registers[i].mode == PCILIB_REGISTER_W) mode = " W";
	    else mode = "  ";
	    
	    printf(" 0x%02x (%2i %s) %s", registers[i].addr, registers[i].bits, mode, registers[i].name);
	    if ((details > 0)&&(registers[i].description)&&(registers[i].description[0])) {
		printf(": %s", registers[i].description);
	    }
	    printf("\n");

	}
	printf("\n");
    }

    if (bank == (char*)-1) events = NULL;
    else events = model_info->events;

    if (events) {
	printf("Events: \n");
	for (i = 0; events[i].name; i++) {
	    printf(" %s", events[i].name);
	    if ((events[i].description)&&(events[i].description[0])) {
		printf(": %s", events[i].description);
	    }
	}
	printf("\n");
    }
}

void Info(pcilib_t *handle, pcilib_model_description_t *model_info) {
    const pcilib_board_info_t *board_info = pcilib_get_board_info(handle);

    printf("Vendor: %x, Device: %x, Interrupt Pin: %i, Interrupt Line: %i\n", board_info->vendor_id, board_info->device_id, board_info->interrupt_pin, board_info->interrupt_line);
    List(handle, model_info, (char*)-1, 0);
}


#define BENCH_MAX_DMA_SIZE 4 * 1024 * 1024
#define BENCH_MAX_FIFO_SIZE 1024 * 1024

int Benchmark(pcilib_t *handle, ACCESS_MODE mode, pcilib_dma_engine_addr_t dma, pcilib_bar_t bar, uintptr_t addr, size_t n, access_t access, size_t iterations) {
    int err;
    int i, j, errors;
    void *data, *buf, *check;
    void *fifo;
    struct timeval start, end;
    unsigned long time;
    size_t size, min_size, max_size;
    double mbs_in, mbs_out, mbs;
    size_t irqs;
    
    const pcilib_board_info_t *board_info = pcilib_get_board_info(handle);

    if (mode == ACCESS_DMA) {
	if (n) {
	    min_size = n * access;
	    max_size = n * access;
	} else {
	    min_size = 1024;
	    max_size = BENCH_MAX_DMA_SIZE;
	}
	
        for (size = min_size; size <= max_size; size *= 4) {
	    mbs_in = pcilib_benchmark_dma(handle, dma, addr, size, BENCHMARK_ITERATIONS, PCILIB_DMA_FROM_DEVICE);
	    mbs_out = pcilib_benchmark_dma(handle, dma, addr, size, BENCHMARK_ITERATIONS, PCILIB_DMA_TO_DEVICE);
	    mbs = pcilib_benchmark_dma(handle, dma, addr, size, iterations, PCILIB_DMA_BIDIRECTIONAL);
	    err = pcilib_wait_irq(handle, 0, 0, &irqs);
	    if (err) irqs = 0;
	    
	    printf("%8i KB - ", size / 1024);
	    
	    printf("RW: ");
	    if (mbs < 0) printf("failed ...   ");
	    else printf("%8.2lf MB/s", mbs);

	    printf(", R: ");
	    if (mbs_in < 0) printf("failed ...   ");
	    else printf("%8.2lf MB/s", mbs_in);

	    printf(", W: ");
	    if (mbs_out < 0) printf("failed ...   ");
	    else printf("%8.2lf MB/s", mbs_out);
	    
	    if (irqs) {
	        printf(", IRQs: %lu", irqs);
	    }

	    printf("\n");
	}
	
	return 0;
    }

    if (bar == PCILIB_BAR_INVALID) {
	unsigned long maxlength = 0;
	

	for (i = 0; i < PCILIB_MAX_BANKS; i++) {
	    if ((addr >= board_info->bar_start[i])&&((board_info->bar_start[i] + board_info->bar_length[i]) >= (addr + access))) {
		bar = i;
		break;
	    }

	    if (board_info->bar_length[i] > maxlength) {
		maxlength = board_info->bar_length[i];
		bar = i;
	    }
	}
	
	if (bar < 0) Error("Data banks are not available");
    }
    
    if (n) {
	if ((mode == ACCESS_BAR)&&(n * access > board_info->bar_length[bar])) Error("The specified size (%i) exceeds the size of bar (%i)", n * access, board_info->bar_length[bar]);

	min_size = n * access;
	max_size = n * access;
    } else {
	min_size = access;
	if (mode == ACCESS_BAR) max_size = board_info->bar_length[bar];
	else max_size = BENCH_MAX_FIFO_SIZE;
    }
    
    err = posix_memalign( (void**)&buf, 256, max_size );
    if (!err) err = posix_memalign( (void**)&check, 256, max_size );
    if ((err)||(!buf)||(!check)) Error("Allocation of %i bytes of memory have failed", max_size);

    data = pcilib_map_bar(handle, bar);
    if (!data) Error("Can't map bar %i", bar);

    if (mode == ACCESS_FIFO) {
        fifo = data + (addr - board_info->bar_start[bar]) + (board_info->bar_start[bar] & pcilib_get_page_mask());
//	pcilib_resolve_register_address(handle, bar, addr);
	if (!fifo) Error("Can't resolve address (%lx) in bar (%u)", addr, bar);
    }

    if (mode == ACCESS_FIFO)
	printf("Transfer time (Bank: %i, Fifo: %lx):\n", bar, addr);
    else
	printf("Transfer time (Bank: %i):\n", bar);
	
    for (size = min_size ; size < max_size; size *= 8) {
	gettimeofday(&start,NULL);
	if (mode == ACCESS_BAR) {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_memcpy(buf, data, size);
	    }
	} else {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		for (j = 0; j < (size/access); j++) {
		    pcilib_memcpy(buf + j * access, fifo, access);
		}
	    }
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf("%8i bytes - read: %8.2lf MB/s", size, 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024. * 1024.));
	
	fflush(0);

	gettimeofday(&start,NULL);
	if (mode == ACCESS_BAR) {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_memcpy(data, buf, size);
	    }
	} else {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		for (j = 0; j < (size/access); j++) {
		    pcilib_memcpy(fifo, buf + j * access, access);
		}
	    }
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf(", write: %8.2lf MB/s\n", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024. * 1024.));
    }
    
    pcilib_unmap_bar(handle, bar, data);

    printf("\n\nOpen-Transfer-Close time: \n");
    
    for (size = 4 ; size < max_size; size *= 8) {
	gettimeofday(&start,NULL);
	if (mode == ACCESS_BAR) {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_read(handle, bar, 0, size, buf);
	    }
	} else {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_read_fifo(handle, bar, addr, access, size / access, buf);
	    }
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf("%8i bytes - read: %8.2lf MB/s", size, 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024. * 1024.));
	
	fflush(0);

	gettimeofday(&start,NULL);
	if (mode == ACCESS_BAR) {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_write(handle, bar, 0, size, buf);
	    }
	} else {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_write_fifo(handle, bar, addr, access, size / access, buf);
	    }
	}
	
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf(", write: %8.2lf MB/s", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024. * 1024.));

	if (mode == ACCESS_BAR) {
	    gettimeofday(&start,NULL);
	    for (i = 0, errors = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_write(handle, bar, 0, size, buf);
		pcilib_read(handle, bar, 0, size, check);
		if (memcmp(buf, check, size)) ++errors;
	    }
	    gettimeofday(&end,NULL);

	    time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	    printf(", write-verify: %8.2lf MB/s", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024. * 1024.));
	    if (errors) printf(", errors: %u of %u", errors, BENCHMARK_ITERATIONS);
	}
	printf("\n");
    }
    
    printf("\n\n");

    free(check);
    free(buf);
}

#define pci2host16(endianess, value) endianess?


int ReadData(pcilib_t *handle, ACCESS_MODE mode, pcilib_dma_engine_addr_t dma, pcilib_bar_t bar, uintptr_t addr, size_t n, access_t access, int endianess) {
    void *buf;
    int i, err;
    size_t ret;
    int size = n * abs(access);
    int block_width, blocks_per_line;
    int numbers_per_block, numbers_per_line; 
    pcilib_dma_engine_t dmaid;
    
    numbers_per_block = BLOCK_SIZE / access;

    block_width = numbers_per_block * ((access * 2) +  SEPARATOR_WIDTH);
    blocks_per_line = (LINE_WIDTH - 10) / (block_width +  BLOCK_SEPARATOR_WIDTH);
    if ((blocks_per_line > 1)&&(blocks_per_line % 2)) --blocks_per_line;
    numbers_per_line = blocks_per_line * numbers_per_block;

//    buf = alloca(size);
    err = posix_memalign( (void**)&buf, 256, size );
    if ((err)||(!buf)) Error("Allocation of %i bytes of memory have failed", size);
    
    switch (mode) {
      case ACCESS_DMA:
	dmaid = pcilib_find_dma_by_addr(handle, PCILIB_DMA_FROM_DEVICE, dma);
	if (dmaid == PCILIB_DMA_ENGINE_INVALID) Error("Invalid DMA engine (%lu) is specified", dma);
	err = pcilib_read_dma(handle, dmaid, addr, size, buf, &ret);
	if ((err)||(ret <= 0)) Error("No data is returned by DMA engine");
	size = ret;
	addr = 0;
      break;
      case ACCESS_FIFO:
	pcilib_read_fifo(handle, bar, addr, access, n, buf);
	addr = 0;
      break;
      default:
	pcilib_read(handle, bar, addr, size, buf);
    }
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




int ReadRegister(pcilib_t *handle, pcilib_model_description_t *model_info, const char *bank, const char *reg) {
    int i;
    int err;
    const char *format;

    pcilib_register_bank_t bank_id;
    pcilib_register_bank_addr_t bank_addr;

    pcilib_register_value_t value;
    
    if (reg) {
	pcilib_register_t regid = pcilib_find_register(handle, bank, reg);
        bank_id = pcilib_find_bank_by_addr(handle, model_info->registers[regid].bank);
        format = model_info->banks[bank_id].format;
        if (!format) format = "%lu";

        err = pcilib_read_register_by_id(handle, regid, &value);
    //    err = pcilib_read_register(handle, bank, reg, &value);
        if (err) printf("Error reading register %s\n", reg);
        else {
	    printf("%s = ", reg);
	    printf(format, value);
	    printf("\n");
	}
    } else {
	    // Adding DMA registers
	pcilib_get_dma_info(handle);	
    
	if (model_info->registers) {
	    if (bank) {
		bank_id = pcilib_find_bank(handle, bank);
		bank_addr = model_info->banks[bank_id].addr;
	    }
	    
	    printf("Registers:\n");
	    for (i = 0; model_info->registers[i].bits; i++) {
		if ((model_info->registers[i].mode & PCILIB_REGISTER_R)&&((!bank)||(model_info->registers[i].bank == bank_addr))&&(model_info->registers[i].type != PCILIB_REGISTER_BITS)) { 
		    bank_id = pcilib_find_bank_by_addr(handle, model_info->registers[i].bank);
		    format = model_info->banks[bank_id].format;
		    if (!format) format = "%lu";

		    err = pcilib_read_register_by_id(handle, i, &value);
		    if (err) printf(" %s = error reading value", model_info->registers[i].name);
	    	    else {
			printf(" %s = ", model_info->registers[i].name);
			printf(format, value);
		    }

		    printf(" [");
		    printf(format, model_info->registers[i].defvalue);
		    printf("]");
		    printf("\n");
		}
	    }
	} else {
	    printf("No registers");
	}
	printf("\n");
    }
}

int ReadRegisterRange(pcilib_t *handle, pcilib_model_description_t *model_info, const char *bank, uintptr_t addr, size_t n) {
    int err;
    int i;

    pcilib_register_bank_description_t *banks = model_info->banks;
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
	printf("%0*lx", access * 2, (unsigned long)buf[i]);
    }
    printf("\n\n");
}

int WriteData(pcilib_t *handle, ACCESS_MODE mode, pcilib_dma_engine_addr_t dma, pcilib_bar_t bar, uintptr_t addr, size_t n, access_t access, int endianess, char ** data) {
    int read_back = 0;
    void *buf, *check;
    int res, i, err;
    int size = n * abs(access);
    size_t ret;
    pcilib_dma_engine_t dmaid;

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

    switch (mode) {
      case ACCESS_DMA:
	dmaid = pcilib_find_dma_by_addr(handle, PCILIB_DMA_TO_DEVICE, dma);
	if (dmaid == PCILIB_DMA_ENGINE_INVALID) Error("Invalid DMA engine (%lu) is specified", dma);
	err = pcilib_write_dma(handle, dmaid, addr, size, buf, &ret);
	if ((err)||(ret != size)) {
	    if (!ret) Error("No data is written by DMA engine");
	    else Error("Only %lu bytes of %lu is written by DMA engine", ret, size);
	}
      break;
      case ACCESS_FIFO:
	pcilib_write_fifo(handle, bar, addr, access, n, buf);
      break;
      default:
	pcilib_write(handle, bar, addr, size, buf);
	pcilib_read(handle, bar, addr, size, check);
	read_back = 1;
    }

    if ((read_back)&&(memcmp(buf, check, size))) {
	printf("Write failed: the data written and read differ, the foolowing is read back:\n");
        if (endianess) pcilib_swap(check, check, abs(access), n);
	ReadData(handle, mode, dma, bar, addr, n, access, endianess);
	exit(-1);
    }

    free(check);
    free(buf);
}

int WriteRegisterRange(pcilib_t *handle, pcilib_model_description_t *model_info, const char *bank, uintptr_t addr, size_t n, char ** data) {
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
	ReadRegisterRange(handle, model_info, bank, addr, n);
	exit(-1);
    }

    free(check);
    free(buf);

}

int WriteRegister(pcilib_t *handle, pcilib_model_description_t *model_info, const char *bank, const char *reg, char ** data) {
    int err;
    int i;

    unsigned long val;
    pcilib_register_value_t value;

    const char *format;

/*
    pcilib_register_bank_t bank_id;
    pcilib_register_bank_addr_t bank_addr;

    pcilib_register_t regid = pcilib_find_register(handle, bank, reg);
    if (regid == PCILIB_REGISTER_INVALID) Error("Can't find register (%s) from bank (%s)", reg, bank?bank:"autodetected");
    bank_id = pcilib_find_bank_by_addr(handle, model_info->registers[regid].bank);
    if (bank_id == PCILIB_REGISTER_BANK_INVALID) Error("Can't find bank of the register (%s)", reg);
    format = model_info->banks[bank_id].format;
    if (!format) format = "%lu";
*/

    if (isnumber(*data)) {
	if (sscanf(*data, "%li", &val) != 1) {
	    Error("Can't parse data value (%s) is not valid decimal number", *data);
	}

	format = "%li";
    } else if (isxnumber(*data)) {
	if (sscanf(*data, "%lx", &val) != 1) {
	    Error("Can't parse data value (%s) is not valid decimal number", *data);
	}
	
	format = "0x%lx";
    } else {
	    Error("Can't parse data value (%s) is not valid decimal number", *data);
    }

    value = val;
    
    err = pcilib_write_register(handle, bank, reg, value);
    if (err) Error("Error writting register %s\n", reg);

    err = pcilib_read_register(handle, bank, reg, &value);
    if (err) Error("Error reading back register %s for verification\n", reg);

    if (val != value) {
	Error("Failed to write register %s: %lu is written and %lu is read back", reg, val, value);
    } else {
	printf("%s = ", reg);
	printf(format, value);
	printf("\n");
    }
    
    return 0;
}

int Grab(pcilib_t *handle, const char *event, const char *output) {
    int err;
    
    void *data = NULL;
    size_t size, written;
    
    FILE *o;
    
    // ignoring event for now
	
    err = pcilib_grab(handle, PCILIB_EVENTS_ALL, &size, &data, NULL);
    if (err) {
	Error("Grabbing event is failed");
    }

    if (output) {
	o = fopen(output, "w");
	if (!o) {
	    Error("Failed to open file \"%s\"", output);
	}
	
	printf("Writting %zu bytes into %s...\n", size, output);
    } else o = stdout;
    
    written = fwrite(data, 1, size, o);
    if (written != size) {
	if (written > 0) Error("Write failed, only %z bytes out of %z are stored", written, size);
	else Error("Write failed");
    }
    
    if (o != stdout) fclose(o);

    return 0;
}


int StartStopDMA(pcilib_t *handle,  pcilib_model_description_t *model_info, pcilib_dma_engine_addr_t dma, pcilib_dma_direction_t dma_direction, int start) {
    int err;
    pcilib_dma_engine_t dmaid;
    
    if (dma == PCILIB_DMA_ENGINE_ADDR_INVALID) {
        const pcilib_dma_info_t *dma_info = pcilib_get_dma_info(handle);

        if (start) Error("DMA engine should be specified");

	for (dmaid = 0; dma_info->engines[dmaid]; dmaid++) {
	    err = pcilib_start_dma(handle, dmaid, 0);
	    if (err) Error("Error starting DMA Engine (%s %i)", ((dma_info->engines[dmaid]->direction == PCILIB_DMA_FROM_DEVICE)?"C2S":"S2C"), dma_info->engines[dmaid]->addr);
	    err = pcilib_stop_dma(handle, dmaid, PCILIB_DMA_FLAG_PERSISTENT);
	    if (err) Error("Error stopping DMA Engine (%s %i)", ((dma_info->engines[dmaid]->direction == PCILIB_DMA_FROM_DEVICE)?"C2S":"S2C"), dma_info->engines[dmaid]->addr);
	}
	
	return 0;
    }
    
    if (dma_direction&PCILIB_DMA_FROM_DEVICE) {
	dmaid = pcilib_find_dma_by_addr(handle, PCILIB_DMA_FROM_DEVICE, dma);
	if (dmaid == PCILIB_DMA_ENGINE_INVALID) Error("Invalid DMA engine (C2S %lu) is specified", dma);
	
	if (start) {
	    err = pcilib_start_dma(handle, dmaid, PCILIB_DMA_FLAG_PERSISTENT);
    	    if (err) Error("Error starting DMA engine (C2S %lu)", dma);
	} else {
	    err = pcilib_start_dma(handle, dmaid, 0);
    	    if (err) Error("Error starting DMA engine (C2S %lu)", dma);
	    err = pcilib_stop_dma(handle, dmaid, PCILIB_DMA_FLAG_PERSISTENT);
    	    if (err) Error("Error stopping DMA engine (C2S %lu)", dma);
	}
    }
    
    if (dma_direction&PCILIB_DMA_TO_DEVICE) {
	dmaid = pcilib_find_dma_by_addr(handle, PCILIB_DMA_TO_DEVICE, dma);
	if (dmaid == PCILIB_DMA_ENGINE_INVALID) Error("Invalid DMA engine (S2C %lu) is specified", dma);
	
	if (start) {
	    err = pcilib_start_dma(handle, dmaid, PCILIB_DMA_FLAG_PERSISTENT);
    	    if (err) Error("Error starting DMA engine (S2C %lu)", dma);
	} else {
	    err = pcilib_start_dma(handle, dmaid, 0);
    	    if (err) Error("Error starting DMA engine (S2C %lu)", dma);
	    err = pcilib_stop_dma(handle, dmaid, PCILIB_DMA_FLAG_PERSISTENT);
    	    if (err) Error("Error stopping DMA engine (S2C %lu)", dma);
	}
    }

    return 0;
}


typedef struct {
    unsigned long use;
    
    int referenced;
    int hw_lock;
    int reusable;
    int persistent;
    int open;
    
    size_t count;
    size_t size;
} kmem_use_info_t;

#define MAX_USES 64

size_t FindUse(size_t *n_uses, kmem_use_info_t *uses, unsigned long use) {
    size_t i, n = *n_uses;
    
    if (uses[n - 1].use == use) return n - 1;

    for (i = 1; i < (n - 1); i++) {
	if (uses[i].use == use) return i;
    }
    
    if (n == MAX_USES) return 0;

    uses[n].use = use;
    return (*n_uses)++;
}

char *PrintSize(char *str, size_t size) {
    if (size >= 1073741824) sprintf(str, "%.1lf GB", 1.*size / 1073741824);
    else if (size >= 1048576) sprintf(str, "%.1lf MB", 1.*size / 1048576);
    else if (size >= 1024) sprintf(str, "%lu KB", size / 1024);
    else sprintf(str, "%lu B ", size);
    
    return str;
}

int ListKMEM(pcilib_t *handle, const char *device) {
    DIR *dir;
    struct dirent *entry;
    const char *pos;
    char sysdir[256];
    char fname[256];
    char info[256];
    char stmp[256];
    
    size_t useid, i, n_uses = 1;	// Use 0 is for others
    kmem_use_info_t uses[MAX_USES];

    memset(uses, 0, sizeof(uses));
    
    pos = strrchr(device, '/');
    if (pos) ++pos;
    else pos = device;
    
    snprintf(sysdir, 255, "/sys/class/fpga/%s", pos);

    dir = opendir(sysdir);
    if (!dir) Error("Can't open directory (%s)", sysdir);
    
    while ((entry = readdir(dir)) != NULL) {
	FILE *f;
	unsigned long use;
	unsigned long size;
	unsigned long refs;
	unsigned long mode;
	unsigned long hwref;
	
	if (strncmp(entry->d_name, "kbuf", 4)) continue;
	if (!isnumber(entry->d_name+4)) continue;
	
	snprintf(fname, 255, "%s/%s", sysdir, entry->d_name);
	f = fopen(fname, "r");
	if (!f) Error("Can't access file (%s)", fname);

	while(!feof(f)) {
	    fgets(info, 256, f);
	    if (!strncmp(info, "use:", 4)) use = strtoul(info+4, NULL, 16);
	    if (!strncmp(info, "size:", 5)) size = strtoul(info+5, NULL, 10);
	    if (!strncmp(info, "refs:", 5)) refs = strtoul(info+5, NULL, 10);
	    if (!strncmp(info, "mode:", 5)) mode = strtoul(info+5, NULL, 16);
	    if (!strncmp(info, "hw ref:", 7)) hwref = strtoul(info+7, NULL, 10);
	}
	fclose(f);
	
	useid = FindUse(&n_uses, uses, use);
	uses[useid].count++;
	uses[useid].size += size;
	if (refs) uses[useid].referenced = 1;
	if (hwref) uses[useid].hw_lock = 1;
	if (mode&KMEM_MODE_REUSABLE) uses[useid].reusable = 1;
	if (mode&KMEM_MODE_PERSISTENT) uses[useid].persistent = 1;
	if (mode&KMEM_MODE_COUNT) uses[useid].open = 1;
    }
    closedir(dir);

    printf("Use Type               Count         Total Size        REF           Mode \n");
    printf("--------------------------------------------------------------------------------\n");
    for (useid = 0; useid < n_uses; useid++) {
	if (useid + 1 == n_uses) {
	    if (!uses[0].count) continue;
	    i = 0;
	} else i = useid + 1;
	
	if (!i) printf("0x%08lx Others", 0);
	else if ((uses[i].use >> 16) == PCILIB_KMEM_USE_DMA_RING) printf("DMA%u %s Ring    ", uses[i].use&0x7F, ((uses[i].use&0x80)?"C2S":"S2C"));
	else if ((uses[i].use >> 16) == PCILIB_KMEM_USE_DMA_PAGES) printf("DMA%u %s Pages   ", uses[i].use&0x7F, ((uses[i].use&0x80)?"C2S":"S2C"));
	else printf("0x%08lx       ", uses[i].use);
	printf("    ");
	printf("% 9lu", uses[i].count);
	printf("     ");
	printf("% 12s", PrintSize(stmp, uses[i].size));
	printf("         ");
	if (uses[i].referenced&&uses[i].hw_lock) printf("HW+SW");
	else if (uses[i].referenced) printf("   SW");
	else if (uses[i].hw_lock) printf("HW   ");
	else printf("  -  ");
	printf("         ");
	if (uses[i].persistent) printf("Persistent");
	else if (uses[i].open) printf("Open      ");
	else if (uses[i].reusable) printf("Reusable  ");
	else printf("Closed    ");
	printf("\n");
    }
    printf("--------------------------------------------------------------------------------\n");
    printf("REF - Software/Hardware Reference, MODE - Reusable/Persistent/Open\n");


    return 0;
}

int WaitIRQ(pcilib_t *handle, pcilib_model_description_t *model_info, pcilib_irq_source_t irq_source, pcilib_timeout_t timeout) {
    int err;
    size_t count;
    
    err = pcilib_enable_irq(handle, PCILIB_EVENT_IRQ, 0);
    if (err) Error("Error enabling IRQs");

    err = pcilib_wait_irq(handle, irq_source, timeout, &count);
    if (err) {
	if (err == PCILIB_ERROR_TIMEOUT) Error("Timeout waiting for IRQ");
	else Error("Error waiting for IRQ");
    }

    return 0;
}


int main(int argc, char **argv) {
    int i;
    long itmp;
    unsigned char c;

    const char *num_offset;

    int details = 0;
    int quiete = 0;
    
    pcilib_model_t model = PCILIB_MODEL_DETECT;
    pcilib_model_description_t *model_info;
    MODE mode = MODE_INVALID;
    const char *type = NULL;
    ACCESS_MODE amode = ACCESS_BAR;
    const char *fpga_device = DEFAULT_FPGA_DEVICE;
    pcilib_bar_t bar = PCILIB_BAR_DETECT;
    const char *addr = NULL;
    const char *reg = NULL;
    const char *bank = NULL;
    char **data = NULL;
    const char *event = NULL;
    const char *dma_channel = NULL;
    const char *use = NULL;
    pcilib_irq_source_t irq_source;
    pcilib_dma_direction_t dma_direction = PCILIB_DMA_BIDIRECTIONAL;
    
    pcilib_dma_engine_addr_t dma = PCILIB_DMA_ENGINE_ADDR_INVALID;
    uintptr_t start = -1;
    size_t size = 1;
    access_t access = 4;
    int skip = 0;
    int endianess = 0;
    size_t timeout = 0;
    const char *output = NULL;
    size_t iterations = BENCHMARK_ITERATIONS;

    pcilib_t *handle;
    
    int size_set = 0;
    
    
    while ((c = getopt_long(argc, argv, "hqilpr::w::g::d:m:t:b:a:s:e:o:", long_options, NULL)) != (unsigned char)-1) {
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
		if (mode == MODE_LIST) details++;
		else if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_LIST;
	    break;
	    case OPT_RESET:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_RESET;
	    break;
	    case OPT_BENCHMARK:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_BENCHMARK;

		if (optarg) addr = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) addr = argv[optind++];
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
	    case OPT_GRAB:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_GRAB;
		if (optarg) event = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) event = argv[optind++];
	    break;
	    case OPT_START_DMA:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_START_DMA;
		if (optarg) dma_channel = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) dma_channel = argv[optind++];
	    break;
	    case OPT_STOP_DMA:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_STOP_DMA;
		if (optarg) dma_channel = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) dma_channel = argv[optind++];
	    break;
	    case OPT_WAIT_IRQ:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_WAIT_IRQ;
		if (optarg) num_offset = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-'))  num_offset = argv[optind++];
		
		if ((!isnumber(num_offset))||(sscanf(num_offset, "%li", &itmp) != 1))
		    Usage(argc, argv, "Invalid IRQ source is specified (%s)", num_offset);

		irq_source = itmp;
	    break;
	    case OPT_LIST_KMEM:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		mode = MODE_LIST_KMEM;
	    break;
	    case OPT_FREE_KMEM:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		mode = MODE_FREE_KMEM;

		if (optarg) use = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) use = argv[optind++];
	    break;
	    case OPT_DEVICE:
		fpga_device = optarg;
	    break;
	    case OPT_MODEL:
		if (!strcasecmp(optarg, "pci")) model = PCILIB_MODEL_PCI;
		else if (!strcasecmp(optarg, "ipecamera")) model = PCILIB_MODEL_IPECAMERA;
		else Usage(argc, argv, "Invalid memory model (%s) is specified", optarg);
	    break;
	    case OPT_BAR:
		bank = optarg;
//		if ((sscanf(optarg,"%li", &itmp) != 1)||(itmp < 0)||(itmp >= PCILIB_MAX_BANKS)) Usage(argc, argv, "Invalid data bank (%s) is specified", optarg);
//		else bar = itmp;
	    break;
	    case OPT_ACCESS:
		if (!strncasecmp(optarg, "fifo", 4)) {
		    type = "fifo";
		    num_offset = optarg + 4;
		    amode = ACCESS_FIFO;
		} else if (!strncasecmp(optarg, "dma", 3)) {
		    type = "dma";
		    num_offset = optarg + 3;
		    amode = ACCESS_DMA;
		} else if (!strncasecmp(optarg, "bar", 3)) {
		    type = "plain";
		    num_offset = optarg + 3;
		    amode = ACCESS_BAR;
		} else if (!strncasecmp(optarg, "plain", 5)) {
		    type = "plain";
		    num_offset = optarg + 5;
		    amode = ACCESS_BAR;
		} else {
		    num_offset = optarg;
		}

		if (*num_offset) {
		    if ((!isnumber(num_offset))||(sscanf(num_offset, "%li", &itmp) != 1))
			Usage(argc, argv, "Invalid access type (%s) is specified", optarg);

	    	    switch (itmp) {
			case 8: access = 1; break;
			case 16: access = 2; break;
			case 32: access = 4; break;
			case 64: access = 8; break;
			default: Usage(argc, argv, "Invalid data width (%s) is specified", num_offset);
		    }	
		}
	    break;
	    case OPT_SIZE:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &size) != 1))
		    Usage(argc, argv, "Invalid size is specified (%s)", optarg);
		size_set = 1;
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
	    case OPT_TIMEOUT:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &timeout) != 1))
		    Usage(argc, argv, "Invalid timeout is specified (%s)", optarg);
	    break;
	    case OPT_OUTPUT:
		output = optarg;
	    break;
	    case OPT_ITERATIONS:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &iterations) != 1))
		    Usage(argc, argv, "Invalid number of iterations is specified (%s)", optarg);
		size_set = 1;
	    break;
	    case OPT_QUIETE:
		quiete = 1;
	    break;
	    default:
		Usage(argc, argv, "Unknown option (%s)", argv[optind]);
	}
    }

    if (mode == MODE_INVALID) {
	if (argc > 1) Usage(argc, argv, "Operation is not specified");
	else Usage(argc, argv, NULL);
    }

    pcilib_set_error_handler(&Error, quiete?Silence:NULL);

    handle = pcilib_open(fpga_device, model);
    if (handle < 0) Error("Failed to open FPGA device: %s", fpga_device);

    model = pcilib_get_model(handle);
    model_info = pcilib_get_model_description(handle);

    switch (mode) {
     case MODE_WRITE:
        if (!addr) Usage(argc, argv, "The address is not specified");
        if (((argc - optind) == 1)&&(*argv[optind] == '*')) {
    	    int vallen = strlen(argv[optind]);
    	    if (vallen > 1) {
		data = (char**)malloc(size * (vallen + sizeof(char*)));
		if (!data) Error("Error allocating memory for data array");

		for (i = 0; i < size; i++) {
		    data[i] = ((char*)data) + size * sizeof(char*) + i * vallen;
		    strcpy(data[i], argv[optind] + 1);
		}
	    } else {
		data = (char**)malloc(size * (9 + sizeof(char*)));
		if (!data) Error("Error allocating memory for data array");
		
		for (i = 0; i < size; i++) {
		    data[i] = ((char*)data) + size * sizeof(char*) + i * 9;
		    sprintf(data[i], "%x", i);
		}
	    }
        } else if ((argc - optind) == size) data = argv + optind;
        else Usage(argc, argv, "The %i data values is specified, but %i required", argc - optind, size);
     break;
     case MODE_READ:
        if (!addr) {
	    if (model == PCILIB_MODEL_PCI) {
		Usage(argc, argv, "The address is not specified");
	    } else ++mode;
	}
     break;
     case MODE_START_DMA:
     case MODE_STOP_DMA:
        if ((dma_channel)&&(*dma_channel)) {
	    itmp = strlen(dma_channel) - 1;
	    if (dma_channel[itmp] == 'r') dma_direction = PCILIB_DMA_FROM_DEVICE;
	    else if (dma_channel[itmp] == 'w') dma_direction = PCILIB_DMA_TO_DEVICE;

	    if (dma_direction != PCILIB_DMA_BIDIRECTIONAL) itmp--;
	    
	    if (strncmp(dma_channel, "dma", 3)) num_offset = dma_channel;
	    else {
		num_offset = dma_channel + 3;
		itmp -= 3;
	    }
	    
	    if (bank) {
		if (strncmp(num_offset, bank, itmp)) Usage(argc, argv, "Conflicting DMA channels are specified in mode parameter (%s) and bank parameter (%s)", dma_channel, bank);
	    }
		 
	    if (!isnumber_n(num_offset, itmp))
		 Usage(argc, argv, "Invalid DMA channel (%s) is specified", dma_channel);

	    dma = atoi(num_offset);
	}
     break;
     default:
        if (argc > optind) Usage(argc, argv, "Invalid non-option parameters are supplied");
    }


    if (addr) {
	if ((!strncmp(addr, "dma", 3))&&((addr[3]==0)||isnumber(addr+3))) {
	    if ((type)&&(amode != ACCESS_DMA)) Usage(argc, argv, "Conflicting access modes, the DMA read is requested, but access type is (%s)", type);
	    if (bank) {
		if ((addr[3] != 0)&&(strcmp(addr + 3, bank))) Usage(argc, argv, "Conflicting DMA channels are specified in read parameter (%s) and bank parameter (%s)", addr + 3, bank);
	    } else {
		if (addr[3] == 0) Usage(argc, argv, "The DMA channel is not specified");
	    }
	    dma = atoi(addr + 3);
	    amode = ACCESS_DMA;
	} else if ((!strncmp(addr, "bar", 3))&&((addr[3]==0)||isnumber(addr+3))) {
	    if ((type)&&(amode != ACCESS_BAR)) Usage(argc, argv, "Conflicting access modes, the plain PCI read is requested, but access type is (%s)", type);
	    if ((addr[3] != 0)&&(strcmp(addr + 3, bank))) Usage(argc, argv, "Conflicting PCI bars are specified in read parameter (%s) and bank parameter (%s)", addr + 3, bank);
	    bar = atoi(addr + 3);
	    amode = ACCESS_BAR;
	} else if ((isxnumber(addr))&&(sscanf(addr, "%lx", &start) == 1)) {
		// check if the address in the register range
	    pcilib_register_range_t *ranges =  model_info->ranges;
	    
	    if (ranges) {
		for (i = 0; ranges[i].start != ranges[i].end; i++) 
		    if ((start >= ranges[i].start)&&(start <= ranges[i].end)) break;
	    		
		    // register access in plain mode
		if (ranges[i].start != ranges[i].end) ++mode;	
	    }
	} else {
	    if (pcilib_find_register(handle, bank, addr) == PCILIB_REGISTER_INVALID) {
	        Usage(argc, argv, "Invalid address (%s) is specified", addr);
	    } else {
	        reg = addr;
		++mode;
	    }
	} 
    }
    

    if ((bank)&&(amode == ACCESS_DMA)) {
	if ((!isnumber(bank))||(sscanf(bank,"%li", &itmp) != 1)||(itmp < 0)) 
	    Usage(argc, argv, "Invalid DMA channel (%s) is specified", bank);
	else dma = itmp;
    } else if (bank) {
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
        Info(handle, model_info);
     break;
     case MODE_LIST:
        List(handle, model_info, bank, details);
     break;
     case MODE_BENCHMARK:
        Benchmark(handle, amode, dma, bar, start, size_set?size:0, access, iterations);
     break;
     case MODE_READ:
        if (addr) {
	    ReadData(handle, amode, dma, bar, start, size, access, endianess);
	} else {
	    Error("Address to read is not specified");
	}
     break;
     case MODE_READ_REGISTER:
        if ((reg)||(!addr)) ReadRegister(handle, model_info, bank, reg);
	else ReadRegisterRange(handle, model_info, bank, start, size);
     break;
     case MODE_WRITE:
	WriteData(handle, amode, dma, bar, start, size, access, endianess, data);
     break;
     case MODE_WRITE_REGISTER:
        if (reg) WriteRegister(handle, model_info, bank, reg, data);
	else WriteRegisterRange(handle, model_info, bank, start, size, data);
     break;
     case MODE_RESET:
        pcilib_reset(handle);
     break;
     case MODE_GRAB:
        Grab(handle, event, output);
     break;
     case MODE_START_DMA:
        StartStopDMA(handle, model_info, dma, dma_direction, 1);
     break;
     case MODE_STOP_DMA:
        StartStopDMA(handle, model_info, dma, dma_direction, 0);
     break;
     case MODE_WAIT_IRQ:
        WaitIRQ(handle, model_info, irq_source, timeout);
     break;
     case MODE_LIST_KMEM:
        ListKMEM(handle, fpga_device);
     break;
    }

    pcilib_close(handle);
    
    if (data != argv + optind) free(data);
}
