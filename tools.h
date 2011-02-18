#ifndef _PCITOOL_TOOLS_H
#define _PCITOOL_TOOLS_H

void * memcpy8(void * dst, void const * src, size_t len);
void * memcpy32(void * dst, void const * src, size_t len);
void * memcpy64(void * dst, void const * src, size_t len);
int get_page_mask();

#endif /* _PCITOOL_TOOS_H */
