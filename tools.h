#define memcpy0 memcpy32

void * memcpy8(void * dst, void const * src, size_t len);
void * memcpy32(void * dst, void const * src, size_t len);
int get_page_mask();
