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


#include "driver/pciDriver.h"

#include "kernel.h"
#include "tools.h"

/* defines */
#define MAX_KBUF 14
//#define BIGBUFSIZE (512*1024*1024)
#define BIGBUFSIZE (1024*1024)


#define DEFAULT_FPGA_DEVICE "/dev/fpga0"
#define MAX_BANKS 6

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
    MODE_WRITE
} MODE;

typedef enum {
    OPT_DEVICE = 'd',
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
"	-l		- List Data Banks\n"
"	-p		- Performance Evaluation\n"
"	-r <addr>	- Read Data\n"
"	-w <addr>	- Write Data\n"
"	--help		- Help message\n"
"\n"
"  Addressing:\n"
"	-d <device>	- FPGA device (/dev/fpga0)\n"
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

static pci_board_info board_info;
static page_mask = -1;

void GetBoardInfo(int handle) {
    int ret;
    
    if (page_mask < 0) {
	ret = ioctl( handle, PCIDRIVER_IOC_PCI_INFO, &board_info );
	if (ret) Error("PCIDRIVER_IOC_PCI_INFO ioctl have failed");
	
	page_mask = get_page_mask();
    }
}

void List(int handle) {
    int i;

    GetBoardInfo(handle);

    for (i = 0; i < MAX_BANKS; i++) {
	if (board_info.bar_length[i] > 0) {
	    printf(" BAR %d - ", i);

	    switch ( board_info.bar_flags[i]&IORESOURCE_TYPE_BITS) {
		case IORESOURCE_IO:  printf(" IO"); break;
		case IORESOURCE_MEM: printf("MEM"); break;
		case IORESOURCE_IRQ: printf("IRQ"); break;
		case IORESOURCE_DMA: printf("DMA"); break;
	    }
	    
	    if (board_info.bar_flags[i]&IORESOURCE_MEM_64) printf("64");
	    else printf("32");
	    
	    printf(", Start: 0x%08lx, Length: 0x% 8lx, Flags: 0x%08lx\n", board_info.bar_start[i], board_info.bar_length[i], board_info.bar_flags[i] );
	}
    }
}

void Info(int handle) {
    GetBoardInfo(handle);

    printf("Vendor: %x, Device: %x, Interrupt Pin: %i, Interrupt Line: %i\n", board_info.vendor_id, board_info.device_id, board_info.interrupt_pin, board_info.interrupt_line);
    List(handle);
}


int DetectBar(int handle, unsigned long addr, int size) {
    int ret,i;
	
    GetBoardInfo(handle);
		
    for (i = 0; i < MAX_BANKS; i++) {
	if ((addr >= board_info.bar_start[i])&&((board_info.bar_start[i] + board_info.bar_length[i]) >= (addr + size))) return i;
    }
	
    return -1;
}

void *DetectAddress(int handle, int *bar, unsigned long *addr, int size) {
    if (*bar < 0) {
	*bar = DetectBar(handle, *addr, size);
	if (*bar < 0) Error("The requested data block at address 0x%x with size 0x%x does not belongs to any available memory bank", *addr, size);
    } else {
	GetBoardInfo(handle);
	
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

void *MapBar(int handle, int bar) {
    void *res;
    int ret; 

    GetBoardInfo(handle);
    
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

void UnmapBar(int handle, int bar, void *data) {
    munmap(data, board_info.bar_length[bar]);
#ifdef FILE_IO
    close(file_io_handle);
#endif
}



int Read(void *buf, int handle, int bar, unsigned long addr, int size) {
    int i;
    void *data;
    unsigned int offset;
    char local_buf[size];
    


    DetectAddress(handle, &bar, &addr, size);
    data = MapBar(handle, bar);

/*
    for (i = 0; i < size/4; i++)  {
	((uint32_t*)((char*)data+addr))[i] = 0x100 * i + 1;
    }
*/

    memcpy0(buf, data + addr, size);
    
    UnmapBar(handle, bar, data);    
}

int Write(void *buf, int handle, int bar, unsigned long addr, int size) {
    int i;
    void *data;
    unsigned int offset;
    char local_buf[size];
    

    DetectAddress(handle, &bar, &addr, size);
    data = MapBar(handle, bar);

    memcpy0(data + addr, buf, size);
    
    UnmapBar(handle, bar, data);    
}


int Benchmark(int handle, int bar) {
    int i, errors;
    void *data, *buf, *check;
    struct timeval start, end;
    unsigned long time;
    unsigned int size, max_size;
    
    GetBoardInfo(handle);
		
    if (bar < 0) {
	for (i = 0; i < MAX_BANKS; i++) {
	    if (board_info.bar_length[i] > 0) {
		bar = i;
		break;
	    }
	}
	
	if (bar < 0) Error("Data banks are not available");
    }
    

    max_size = board_info.bar_length[bar];
    
    posix_memalign( (void**)&buf, 256, max_size );
    posix_memalign( (void**)&check, 256, max_size );
    if ((!buf)||(!check)) Error("Allocation of %i bytes of memory have failed", max_size);

    printf("Transfer time:\n");    
    data = MapBar(handle, bar);
    
    for (size = 4 ; size < max_size; size *= 8) {
	gettimeofday(&start,NULL);
	for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
	    memcpy0(buf, data, size);
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf("%8i bytes - read: %8.2lf MB/s", size, 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));
	
	fflush(0);

	gettimeofday(&start,NULL);
	for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
	    memcpy0(data, buf, size);
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf(", write: %8.2lf MB/s\n", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));
    }
    
    UnmapBar(handle, bar, data);

    printf("\n\nOpen-Transfer-Close time: \n");
    
    for (size = 4 ; size < max_size; size *= 8) {
	gettimeofday(&start,NULL);
	for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
	    Read(buf, handle, bar, 0, size);
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf("%8i bytes - read: %8.2lf MB/s", size, 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));
	
	fflush(0);

	gettimeofday(&start,NULL);
	for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
	    Write(buf, handle, bar, 0, size);
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf(", write: %8.2lf MB/s", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024 * 1024));

	gettimeofday(&start,NULL);
	for (i = 0, errors = 0; i < BENCHMARK_ITERATIONS; i++) {
	    Write(buf, handle, bar, 0, size);
	    Read(check, handle, bar, 0, size);
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
    int i;
    int size = n * abs(access);
    int block_width, blocks_per_line;
    int numbers_per_block, numbers_per_line; 
    
    numbers_per_block = BLOCK_SIZE / access;

    block_width = numbers_per_block * ((access * 2) +  SEPARATOR_WIDTH);
    blocks_per_line = (LINE_WIDTH - 10) / (block_width +  BLOCK_SEPARATOR_WIDTH);
    if ((blocks_per_line > 1)&&(blocks_per_line % 2)) --blocks_per_line;
    numbers_per_line = blocks_per_line * numbers_per_block;

//    buf = alloca(size);
    posix_memalign( (void**)&buf, 256, size );

    if (!buf) Error("Allocation of %i bytes of memory have failed", size);
    
    Read(buf, handle, bar, addr, size);

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

int WriteData(int handle, int bar, unsigned long addr, int n, int access, char ** data) {
    void *buf, *check;
    int res, i;
    int size = n * abs(access);
    
    posix_memalign( (void**)&buf, 256, size );
    posix_memalign( (void**)&check, 256, size );
    if ((!buf)||(!check)) Error("Allocation of %i bytes of memory have failed", size);

    for (i = 0; i < n; i++) {
	switch (access) {
	    case 1: res = sscanf(data[i], "%hhx", ((uint8_t*)buf)+i); break;
	    case 2: res = sscanf(data[i], "%hx", ((uint16_t*)buf)+i); break;
	    case 4: res = sscanf(data[i], "%x", ((uint32_t*)buf)+i); break;
	    case 8: res = sscanf(data[i], "%lx", ((uint64_t*)buf)+i); break;
	}
	
	if (res != 1) Error("Can't parse data value at poition %i, (%s) is not valid hex number", i, data[i]);
    }

    Write(buf, handle, bar, addr, size);
//    ReadData(handle, bar, addr, n, access);
    Read(check, handle, bar, addr, size);
    
    if (memcmp(buf, check, size)) {
	printf("Write failed: the data written and read differ, the foolowing is read back:\n");
	ReadData(handle, bar, addr, n, access);
	exit(-1);
    }

    free(check);
    free(buf);
}



int main(int argc, char **argv) {
    unsigned char c;

    MODE mode = MODE_INVALID;
    const char *fpga_device = DEFAULT_FPGA_DEVICE;
    int bar = -1;
    const char *addr = NULL;
    unsigned long start = -1;
    int size = 1;
    int access = 4;
    int skip = 0;

    int handle;
    
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
	    case OPT_BAR:
		if ((sscanf(optarg,"%u", &bar) != 1)||(bar < 0)||(bar >= MAX_BANKS)) Usage(argc, argv, "Invalid data bank (%s) is specified", optarg);
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

    if (addr) {
	if (sscanf(addr, "%lx", &start) != 1) Usage(argc, argv, "Invalid address (%s) is specified", addr);
    }
    
    switch (mode) {
     case MODE_WRITE:
        if ((argc - optind) != size) Usage(argc, argv, "The %i data values is specified, but %i required", argc - optind, size);
     case MODE_READ:
        if (!addr) Usage(argc, argv, "The address is not specified");
     break;
     default:
        if (argc > optind) Usage(argc, argv, "Invalid non-option parameters are supplied");
    }

    handle = open(fpga_device, O_RDWR);
    if (handle < 0) Error("Failed to open FPGA device: %s", fpga_device);
    
    switch (mode) {
     case MODE_INFO:
        Info(handle);
     break;
     case MODE_LIST:
        List(handle);
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
     case MODE_WRITE:
	WriteData(handle, bar, start, size, access, argv + optind);
     break;
    }

    close(handle);
}
