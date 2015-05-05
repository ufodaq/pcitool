#ifndef _PCILIB_BAR_H
#define _PCILIB_BAR_H

pcilib_bar_t pcilib_detect_bar(pcilib_t *ctx, uintptr_t addr, size_t size);
int pcilib_detect_address(pcilib_t *ctx, pcilib_bar_t *bar, uintptr_t *addr, size_t size);

#endif /* _PCILIB_BAR_H */
