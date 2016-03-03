#ifndef _PCIDRIVER_H
#define _PCIDRIVER_H

/**
 * Evaluates if the supplied user-space address is actually BAR mapping.
 * @param[in] address	- the user-space address
 * @return		- the hardware address of BAR or 0 if the \p address is not BAR mapping
 */
extern unsigned long pcidriver_resolve_bar(unsigned long address);

#endif /* _PCIDRIVER_H */