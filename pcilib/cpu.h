#ifndef _PCILIB_CPU_H
#define _PCILIB_CPU_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return the mask of system memory page
 * @return 	- page mask, the bits which will correspond to offset within the page are set to 1
 */
int pcilib_get_page_mask();

/**
 * Number of CPU cores in the system (including HyperThreading cores)
 * @return	- number of available CPU cores
 */
int pcilib_get_cpu_count();

/**
 * Returns the generation of Intel Core architecture 
 * Processors up to Intel Core gen4 are recognized. 
 * @return 	- Generation of Intel Core architecture (1 to 4) or 0 for non-Intel and Intel pre-Core architectures
 */
int pcilib_get_cpu_gen();

#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_CPU_H */
