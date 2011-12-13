#ifndef _PCITOOL_FORMATERS_H
#define _PCITOOL_FORMATERS_H

void PrintTime(size_t duration);
void PrintNumber(size_t num);
void PrintSize(size_t num);
void PrintPercent(size_t num, size_t total);
char *GetPrintSize(char *str, size_t size);


#endif /* _PCITOOL_FORMATERS_H */

