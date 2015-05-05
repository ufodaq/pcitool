#define _POSIX_C_SOURCE 200809L
#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sched.h>
#include <errno.h>

#include <pcilib.h>
#include <pcilib/irq.h>
#include <pcilib/kmem.h>

//#include <sys/ipc.h>
//#include <sys/shm.h>


#define DEVICE "/dev/fpga0"

#define BAR PCILIB_BAR0
#define USE_RING PCILIB_KMEM_USE(PCILIB_KMEM_USE_USER, 1)
#define USE PCILIB_KMEM_USE(PCILIB_KMEM_USE_USER, 2)
//#define STATIC_REGION 0x80000000 //  to reserve 512 MB at the specified address, add "memmap=512M$2G" to kernel parameters

#define BUFFERS         128
#define ITERATIONS      1000
#define DESC_THRESHOLD  BUFFERS/8   // Lorenzo: after how many desc the FPGA must update the "written descriptor counter" in PC mem
                                    // if set to 0, the update only happens when INT is received

#define HUGE_PAGE       1           // number of pages per huge page
#define TLP_SIZE        32          // TLP SIZE = 64 for 256B payload, 32 for 128B payload
#define PAGE_SIZE       4096        // other values are not supported in the kernel

#define USE_64                      // Lorenzo: use 64bit addressing

//#define DUAL_CORE                 // Lorenzo: DUAL Core

//#define SHARED_MEMORY               // Lorenzo: Test for fast GUI

#define CHECK_READY                   // Lorenzo: Check if PCI-Express is ready by reading 0x0
#define MEM_COPY                      // Lorenzo: CPY data
//#define CHECK_RESULTS                 // Lorenzo: Check if data received is ok (only for counter!)   
//#define CHECK_RESULTS_LOG           // Lorenzo: Check if data received is ok (only for counter!)   
#define PRINT_RESULTS                 // Lorenzo: Save the received data in "data.out"
//#define EXIT_ON_EMPTY               // Lorenzo: Exit if an "empty_detected" signal is received

#define TIMEOUT         1000000



/* IRQs are slow for some reason. REALTIME mode is slower. Adding delays does not really help,
   otherall we have only 3 checks in average. Check ready seems to be not needed and adds quite 
   much extra time */

//#define USE_IRQ
//#define REALTIME
//#define ADD_DELAYS


#define FPGA_CLOCK 250 // Lorenzo: in MHz !

//#define WR(addr, value) { val = value; pcilib_write(pci, BAR, addr, sizeof(val), &val); }
//#define RD(addr, value) { pcilib_read(pci, BAR, addr, sizeof(val), &val); value = val; }
#define WR(addr, value) { *(uint32_t*)(bar + addr + offset) = value; }
#define RD(addr, value) { value = *(uint32_t*)(bar + addr + offset); }

// **************************************************************************************
// Progress BAR
// Process has done x out of n rounds,
// and we want a bar of width w and resolution r.
   static inline void loadBar(int x, int n, int r, int w)
   {
    // Only update r times.
    if ( x % (n/r +1) != 0 ) return;

    // Calculuate the ratio of complete-to-incomplete.
    float ratio = x/(float)n;
    int   c     = ratio * w;

    // Show the percentage complete.
    printf("%3d%% [", (int)(ratio*100) );

    // Show the load bar.
        for (x=0; x<c; x++)
           printf("=");

       for (x=c; x<w; x++)
           printf(" ");

    // ANSI Control codes to go back to the
    // previous line and clear it.
       printf("]\n\033[F\033[J");
   }
// **************************************************************************************


   static void fail(const char *msg, ...) {
    va_list va;

    va_start(va, msg);
    vprintf(msg, va);
    va_end(va);
    printf("\n");

    exit(-1);
}

void hpsleep(size_t ns) {
    struct timespec wait, tv;

    clock_gettime(CLOCK_REALTIME, &wait);

    wait.tv_nsec += ns;
    if (wait.tv_nsec > 999999999) {
        wait.tv_sec += 1;
        wait.tv_nsec = 1000000000 - wait.tv_nsec;
    }

    do {
        clock_gettime(CLOCK_REALTIME, &tv);
    } while ((wait.tv_sec > tv.tv_sec)||((wait.tv_sec == tv.tv_sec)&&(wait.tv_nsec > tv.tv_nsec)));
}


// **************************************************************************************
int main() {



    int err;
    long i, j;
    pcilib_t *pci;
    pcilib_kmem_handle_t *kdesc;
    pcilib_kmem_handle_t *kbuf;
    struct timeval start, end;
    size_t run_time;
    long long int size_mb;
    void* volatile bar;
    uintptr_t bus_addr[BUFFERS];
    uintptr_t kdesc_bus;
    volatile uint32_t *desc;
    typedef volatile uint32_t *Tbuf;
    Tbuf ptr[BUFFERS];

#ifdef SWITCH_GENERATOR
    int switch_generator = 0;
#endif /* SWITCH_GENERATOR */
#if defined(CHECK_RESULTS)||defined(CHECK_RESULTS_LOG)
    long k;
    int mem_diff;
#endif /* CHECK_RESULTS */

   
    float performance, perf_counter; 
    pcilib_bar_t bar_tmp = BAR; 
    uintptr_t offset = 0;

    unsigned int temp;
    int iterations_completed, buffers_filled;


//    int shmid;
    

    printf("\n\n**** **** **** KIT-DMA TEST **** **** ****\n\n");

    //size = ITERATIONS * BUFFERS * HUGE_PAGE * PAGE_SIZE;
    size_mb = ITERATIONS * BUFFERS * HUGE_PAGE * 4 / 1024;
    printf("Total size of memory buffer: \t %.3lf GBytes\n", (float)size_mb/1024 );
    printf("Using %d Buffers with %d iterations\n\n", BUFFERS, ITERATIONS );

#ifdef ADD_DELAYS
    long rpt = 0, rpt2 = 0;
    size_t best_time;
    best_time = 1000000000L * HUGE_PAGE * PAGE_SIZE / (4L * 1024 * 1024 * 1024);
#endif /* ADD_DELAYS */


    pcilib_kmem_flags_t flags = PCILIB_KMEM_FLAG_HARDWARE|PCILIB_KMEM_FLAG_PERSISTENT|PCILIB_KMEM_FLAG_EXCLUSIVE/*|PCILIB_KMEM_FLAG_REUSE*/; // Lorenzo: if REUSE = 1, the re-allocation fails!
    pcilib_kmem_flags_t free_flags = PCILIB_KMEM_FLAG_HARDWARE/*|PCILIB_KMEM_FLAG_EXCLUSIVE|PCILIB_KMEM_FLAG_REUSE*/;
    pcilib_kmem_flags_t clean_flags = PCILIB_KMEM_FLAG_HARDWARE|PCILIB_KMEM_FLAG_PERSISTENT|PCILIB_KMEM_FLAG_EXCLUSIVE;

    pci = pcilib_open(DEVICE, "pci");
    if (!pci) fail("pcilib_open");

    bar = pcilib_map_bar(pci, BAR);
    if (!bar) {
        pcilib_close(pci);
        fail("map bar");
    }

    pcilib_detect_address(pci, &bar_tmp, &offset, 1);

    pcilib_enable_irq(pci, PCILIB_IRQ_TYPE_ALL, 0);
    pcilib_clear_irq(pci, PCILIB_IRQ_SOURCE_DEFAULT);

    pcilib_clean_kernel_memory(pci, USE, clean_flags);
    pcilib_clean_kernel_memory(pci, USE_RING, clean_flags);

    kdesc = pcilib_alloc_kernel_memory(pci, PCILIB_KMEM_TYPE_CONSISTENT, 1, 128, 4096, USE_RING, flags);
    kdesc_bus = pcilib_kmem_get_block_ba(pci, kdesc, 0);
    desc = (uint32_t*)pcilib_kmem_get_block_ua(pci, kdesc, 0);
    memset((void*)desc, 0, 5*sizeof(uint32_t));

#ifdef REALTIME
    pid_t pid;
    struct sched_param sched = {0};

    pid = getpid();
    sched.sched_priority = sched_get_priority_min(SCHED_FIFO);
    if (sched_setscheduler(pid, SCHED_FIFO, &sched))
        printf("Warning: not able to get real-time priority\n");
#endif /* REALTIME */

    // ******************************************************************
    // ****      MEM: check 4k boundary                             ***** 
    // ******************************************************************

    do  {
        printf("* Allocating KMem, ");
#ifdef STATIC_REGION
        kbuf = pcilib_alloc_kernel_memory(pci, PCILIB_KMEM_TYPE_REGION_C2S, BUFFERS, HUGE_PAGE * PAGE_SIZE, STATIC_REGION, USE, flags);
#else
        kbuf = pcilib_alloc_kernel_memory(pci, PCILIB_KMEM_TYPE_DMA_C2S_PAGE, BUFFERS, HUGE_PAGE * PAGE_SIZE, 4096, USE, flags);
#endif

        if (!kbuf) {
            printf("KMem allocation failed\n");
            exit(0);
        }

        // Pointers for Virtualized Mem
        for (j = 0; j < BUFFERS; j++) {
            ptr[j] = (volatile uint32_t*)pcilib_kmem_get_block_ua(pci, kbuf, j);
            memset((void*)(ptr[j]), 0, HUGE_PAGE * PAGE_SIZE);
        }

        err = 0;

        // Check if HW addresses satisfy 4k boundary condition, if not -> free (!!) and reallocate memory
        printf("4k boundary test: ");
        for (j = 0; j < BUFFERS; j++) {
            temp = (((unsigned int)pcilib_kmem_get_block_ba(pci, kbuf, j)) % 4096);
            //printf("%u", temp);
            if (temp  != 0) {
                err = 1;
            }
        }
        if (err == 1) {
            pcilib_clean_kernel_memory(pci, USE, clean_flags);
            pcilib_clean_kernel_memory(pci, USE_RING, clean_flags);
            pcilib_free_kernel_memory(pci, kbuf,  free_flags);
            printf("failed \xE2\x9C\x98\n");
        }
        else printf("passed \xE2\x9C\x93\n");

    } while (err == 1);


    // ******************************************************************
    // ****      Allocate RAM buffer Memory                         ***** 
    // ******************************************************************
    
    FILE * Output;
    FILE * error_log;

#ifdef MEM_COPY

    uint32_t *temp_data[ITERATIONS][BUFFERS];

    for (j=0; j < ITERATIONS; j++) {
        for (i=0; i < BUFFERS; i++) {
            temp_data[j][i] = (uint32_t *)malloc(HUGE_PAGE*PAGE_SIZE);
            if (temp_data[j][i] == 0) {
                printf("******* Error: could not allocate memory! ********\n");
                exit(0);
            }
            memset((void*)(temp_data[j][i]), 0, HUGE_PAGE * PAGE_SIZE);
        }
    }
#endif

#ifdef SHARED_MEMORY
    // give your shared memory an id, anything will do
    key_t key = 123456;
    char *shared_memory;

    // Setup shared memory, 11 is the size
/*    if ((shmid = shmget(key, HUGE_PAGE*PAGE_SIZE, IPC_CREAT | 0666)) < 0)
    {
      printf("Error getting shared memory id");
      exit(1);
    }

    // Attached shared memory
    if ((shared_memory = shmat(shmid, NULL, 0)) == (char *) -1)
    {
      printf("Error attaching shared memory id");
      exit(1);
    }
    printf("* Shared memory created... Id:\t %d\n", key);
    //////////////// SHARED MEMORY TEST */
#endif

    Output = fopen ("data.out", "w");
    fclose(Output);

    error_log = fopen ("error_log.txt", "w");
    fclose(error_log);
   
    // ******************************************************************
    // ****      PCIe TEST                                          ***** 
    // ******************************************************************

    // Reset DMA
    printf("* DMA: Reset...\n");
    WR(0x00, 0x1);
    usleep(100000);
    WR(0x00, 0x0);
    usleep(100000);
 
#ifdef CHECK_READY       
    printf("* PCIe: Testing...");
    RD(0x0, err);
    if (err == 335746816 || err == 335681280) {
        printf("\xE2\x9C\x93 \n");
    } else {
        printf("\xE2\x9C\x98\n PCIe not ready!\n");
        exit(0);
    }
#endif
    

    // ******************************************************************
    // ****      DMA CONFIGURATION                                  ***** 
    // ******************************************************************

    printf("* DMA: Send Data Amount\n");
#ifdef DUAL_CORE
    WR(0x10, (HUGE_PAGE * (PAGE_SIZE / (4 * TLP_SIZE)))/2);
#else  
    WR(0x10, (HUGE_PAGE * (PAGE_SIZE / (4 * TLP_SIZE))));
#endif   

    printf("* DMA: Running mode: ");

#ifdef USE_64   
    if (TLP_SIZE == 64) 
    {
        WR(0x0C, 0x80040);
        printf ("64bit - 256B Payload\n");
    }
    else if (TLP_SIZE == 32) 
    {
        WR(0x0C, 0x80020);
        printf ("64bit - 128B Payload\n");
    }
#else  
    if (TLP_SIZE == 64) 
    {
        WR(0x0C, 0x0040);
        printf ("32bit - 256B Payload\n");
    }
    else if (TLP_SIZE == 32) 
    {
        WR(0x0C, 0x0020);
        printf ("32bit - 128B Payload\n");
    }
#endif

    printf("* DMA: Reset Desc Memory...\n");
    WR(0x5C, 0x00); // RST Desc Memory

    printf("Writing SW Read Descriptor\n");
    WR(0x58, BUFFERS-1);
    //WR(0x58, 0x01);

    printf("Writing the Descriptor Threshold\n");
    WR(0x60, DESC_THRESHOLD);

    printf("Writing HW write Descriptor Address: %lx\n", kdesc_bus);
    WR(0x54, kdesc_bus);
    usleep(100000);

    printf("* DMA: Writing Descriptors\n");
    for (j = 0; j < BUFFERS; j++ ) {
        bus_addr[j] = pcilib_kmem_get_block_ba(pci, kbuf, j);
        // LEAVE THIS DELAY???!?!?!?!
        usleep(1000);
        //printf("Writing descriptor num. %ld: \t %08lx \n", j, bus_addr[j]);
        WR(0x50, bus_addr[j]);
    }

    // ******************************************************************
    // ****     START DMA                                           *****
    // ******************************************************************

    //printf ("\n ---- Press ENTER to start DMA ---- \n");
    //getchar();

    printf("* DMA: Start \n");
    WR(0x04, 0x1);
    gettimeofday(&start, NULL);

    // ******************************************************************
    // ****     Handshaking DMA                                     *****
    // ******************************************************************

    uint32_t curptr = 0, hwptr;
    uint32_t curbuf = 0;
    int empty = 0;
    i = 0;


    while (i < ITERATIONS) {
        j = 0;
        //printf("\ndesc0: %lx", desc[0]); 
        //printf("\ndesc1: %lx", desc[1]); 
        //printf("\ndesc2: %lx", desc[2]); 
        //printf("\ndesc3: %lx", desc[3]); 
        //printf("\ndesc4: %lx", desc[4]);
        // printf("\ndesc5: %lx", htonl(desc[5]));
        //printf("Iteration: %li of %li \r", i+1, ITERATIONS); 
        //getchar();
        //loadBar(i+1, ITERATIONS, ITERATIONS, 30);
        // printf("\nhwptr: %zu", hwptr);  
        // printf("\ncurptr: %zu", curptr); 

        do {
#ifdef USE_64   
                hwptr = desc[3];
#else // 32-bit
                hwptr = desc[4];
#endif
        j++;    
        //printf("\rcurptr: %lx \t \t hwptr: %lx", curptr, hwptr);
        } while (hwptr == curptr);

        do {    
            pcilib_kmem_sync_block(pci, kbuf, PCILIB_KMEM_SYNC_FROMDEVICE, curbuf);
#ifdef MEM_COPY   
            memcpy(temp_data[i][curbuf], (void*)ptr[curbuf], 4096);
#endif
#ifdef CHECK_RESULTS
for (k = 0; k < 1024 ; k++) 
            {
                mem_diff = (ptr[curbuf][k] - ptr[curbuf][k]);
                //if ((mem_diff == 1) || (mem_diff == (-7)) || (k == 1023) ) 
                if (mem_diff == -1)
                    {;}
                else {
                    //fprintf(error_log, "Error in: \t IT %li \t BUF : %li \t OFFSET: %li \t | %08x --> %08x - DIFF: %d \n", i, j, k, temp_data[i][j][k], temp_data[i][j][k+1], mem_diff);
                    err++;
                }
            }
#endif
#ifdef SHARED_MEMORY
            memcpy(shared_memory, ptr[curbuf], 4096); 
#endif            
            //printf("\ncurbuf: %08x", curbuf); 
            //printf("\nbus_addr[curbuf]\n: %08x",bus_addr[curbuf]);
            // for (k = 0; k < 63; k++){
            // if (k%16 == 0) printf("\n# %d # :", k);
            // printf(" %08x", ptr[curbuf][k]);
            // }
            //pcilib_kmem_sync_block(pci, kbuf, PCILIB_KMEM_SYNC_TODEVICE, curbuf);
            curbuf++;
            if (curbuf == BUFFERS) {
                i++;
                curbuf = 0;
#ifdef SWITCH_GENERATOR                 
                if (switch_generator == 1) {
                    switch_generator = 0;
                    WR(0x9040, 0x100007F0);
                } else {
                    WR(0x9040, 0x180007F0);
                    switch_generator = 1;
                }
#endif
                if (i >= ITERATIONS) break;
                //if (i >= (ITERATIONS - 4) ) WR(0x04, 0x0f); 
            }
        } while (bus_addr[curbuf] != hwptr);

#ifdef EXIT_ON_EMPTY
#ifdef USE_64                 
        if (desc[1] != 0) 
#else // 32bit  
        if (desc[2] != 0)  
#endif                                 
        {
            if (bus_addr[curbuf] == hwptr) {
                empty = 1;
                break;
            }
        }
#endif  

        WR(0x58, curbuf + 1); 
        //printf("WR %d\n", curbuf + 1); 
        //printf("%u (%lu)\n", curbuf, j);
        curptr = hwptr;
    }


    // ******************************************************************
    // **** Read performance and stop DMA                         *******
    // ******************************************************************

    gettimeofday(&end, NULL);
    WR(0x04, 0x00);
    usleep(100);
    RD(0x28, perf_counter);
    usleep(100);
    WR(0x00, 0x01);




    iterations_completed   = i;
    buffers_filled      = curbuf;
    if (empty) printf("* DMA: Empty FIFO! Last iteration: %li of %i\n", i+1, ITERATIONS);
    printf ("* DMA: Stop\n\n");

#ifdef MEM_COPY
    printf ("First value:\t %08x\n", temp_data[0][0][0]);
    printf ("Last value:\t %08x\n\n", temp_data[ITERATIONS-1][BUFFERS-1][(PAGE_SIZE/4)-4]);
#endif
    
    // ******************************************************************
    // **** Performance                                           *******
    // ******************************************************************
    printf("Iterations done: %d\n", iterations_completed);
    printf("Buffers filled on last iteration: %d\n", buffers_filled);


    run_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    //size = (long long int) (( BUFFERS * (iterations_completed)  + buffers_filled) * HUGE_PAGE * PAGE_SIZE);
    size_mb = (long long int) (( BUFFERS * (iterations_completed)  + buffers_filled) * HUGE_PAGE * 4 / 1024);
    printf("Performance: transfered %llu Mbytes in %zu us using %d buffers\n", (size_mb), run_time, BUFFERS);
    //printf("Buffers: \t %d \n", BUFFERS);
    //printf("Buf_Size: \t %d \n", PAGE_SIZE);
    //printf("Perf_counter: \t %f \n", perf_counter);
    performance = ((size_mb * FPGA_CLOCK * 1000000)/(perf_counter*256));
    printf("DMA perf counter:\t%d\n", (int)perf_counter); 
    printf("DMA side:\t\t%.3lf MB/s\n", performance);  
    printf("PC side:\t\t%.3lf MB/s\n\n", 1000000. * size_mb / run_time );

    // ******************************************************************
    // **** Read Data                                             *******
    // ******************************************************************


    #ifdef PRINT_RESULTS
    printf("Writing Data to HDD... \n");
    for (i=0; i < iterations_completed; i++) {
        for (j=0; j < BUFFERS; j++)
        {
            Output = fopen("data.out", "a");
            fwrite(temp_data[i][j], 4096, 1, Output);
            fclose(Output);
        }   
        loadBar(i+1, ITERATIONS, ITERATIONS, 30);
    }
    // Save last partially filled iteration
    for (j=0; j < buffers_filled; j++)
    {
        Output = fopen("data.out", "a");
        fwrite(temp_data[iterations_completed][j], 4096, 1, Output);
        fclose(Output);
    }   
    printf("Data saved in data.out. \n");
    #endif

   #ifdef CHECK_RESULTS_LOG
    err = 0;
    error_log = fopen ("error_log.txt", "a");
    printf("\nChecking data ...\n");
    for (i=0; i < iterations_completed; i++) {
        for (j = 0; j < BUFFERS; j++) {
            for (k = 0; k < 1024 ; k++) 
            {
                mem_diff = ((uint32_t)temp_data[i][j][k] - (uint32_t)temp_data[i][j][k+1]);
                //if ((mem_diff == 1) || (mem_diff == (-7)) || (k == 1023) ) 
                if ((mem_diff == -1) || (k == 1023) ) 
                    {;}
                else {
                    fprintf(error_log, "Error in: \t IT %li \t BUF : %li \t OFFSET: %li \t | %08x --> %08x - DIFF: %d \n", i, j, k, temp_data[i][j][k], temp_data[i][j][k+1], mem_diff);
                    err++;
                }
            }
            if (j != BUFFERS-1) {
            // Check first and Last
                mem_diff = (uint32_t)(temp_data[i][j+1][0] - temp_data[i][j][1023]);
                if (mem_diff == (1)) 
                    {;}
                else {
                    fprintf(error_log, "Error_2 in: \t IT %li \t BUF : %li \t OFFSET: %li \t | %08x --> %08x - DIFF: %d \n", i, j, k, temp_data[i][j+1][0], temp_data[i][j][1023], mem_diff);
                    err++;
                }
            }

        }
        loadBar(i+1, ITERATIONS, ITERATIONS, 30);
    }
    for (j = 0; j < buffers_filled; j++) {
        for (k = 0; k < 1024 ; k++) 
        {
            mem_diff = ((uint32_t)temp_data[iterations_completed][j][k] - (uint32_t)temp_data[iterations_completed][j][k+1]);
                if ((mem_diff == -1) || (k == 1023) ) 
                {;}
            else {
                fprintf(error_log, "Error in: \t IT %li \t BUF : %li \t OFFSET: %li \t | %08x --> %08x - DIFF: %d \n", iterations_completed, j, k, temp_data[iterations_completed][j][k], temp_data[iterations_completed][j][k+1], mem_diff);
                err++;
            }
        }
        if (j != buffers_filled-1) {
        // Check first and Last
            mem_diff = (uint32_t)(temp_data[i][j+1][0] - temp_data[i][j][1023]);
            if (mem_diff == (1)) 
                {;}
            else {
                fprintf(error_log, "Error_2 in: \t IT %li \t BUF : %li \t OFFSET: %li \t | %08x --> %08x - DIFF: %d \n", iterations_completed, j, k, temp_data[iterations_completed][j+1][0], temp_data[iterations_completed][j][1023], mem_diff);
                err++;
            }
        }
    }
    if (err != 0) printf("\rChecking data: \xE2\x9C\x98 %d errors found  \n See \"error_log.txt\" for details \n\n", err);
    else printf("\rChecking data: \xE2\x9C\x93 no errors found  \n\n");
    fclose(error_log);
    #endif

    // *********** Free Memory
#ifdef MEM_COPY
    for (i=0; i < ITERATIONS; i++) {
        for (j=0; j < BUFFERS; j++)
        {
            free(temp_data[i][j]);
        }
    }
#endif

    pcilib_free_kernel_memory(pci, kbuf,  free_flags);
    pcilib_free_kernel_memory(pci, kdesc,  free_flags);
    pcilib_disable_irq(pci, 0);
    pcilib_unmap_bar(pci, BAR, bar);
    pcilib_close(pci);

//    shmdt(shmid);
//    shmctl(shmid, IPC_RMID, NULL);

}
