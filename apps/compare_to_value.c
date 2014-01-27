#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    long i, j, size, num;
    size_t count = 0, total = 0;
    int offset = 0, toread = 1, toskip = 0;
    uint32_t value;
    uint32_t *buf;
    
    if ((argc != 4)&&(argc != 7)) {
	printf("Usage: %s <file> <dwords> <value> [offset_dwords read_dwords skip_dwords] \n", argv[0]);
	exit(0);
    }
    
    FILE *f = fopen(argv[1], "r");
    if (!f) {
	printf("Can't open %s\n", argv[1]);
	exit(1);
    }
    
    size = atol(argv[2]);
    if (size <= 0) {
	printf("Can't parse size %s\n", argv[2]);
	exit(1);
    }
    
    if (sscanf(argv[3], "%x", &value) != 1) {
	printf("Can't parse register %s\n", argv[3]);
	exit(1);
    }

    buf = malloc(size * sizeof(uint32_t));
    if (!buf) {
	printf("Can't allocate %lu bytes of memory\n", size * sizeof(uint32_t));
	exit(1);
    }
    
    if (argc == 7) {
	offset = atoi(argv[4]);
	toread = atoi(argv[5]);
	toskip = atoi(argv[6]);
    }
    

    num = fread(buf, 4, size, f);
    if (num != size) {
	printf("Only %lu of %lu dwords in the file\n", num, size);
	exit(1);
    }
    fclose(f);
    
    for (i = offset; i < size; i += toskip) {
	for (j = 0; j < toread; j++, i++) {
	    total++;
	    if (buf[i] != value) {
		count++;
	    }
	}
    }
    free(buf);
    
    printf("%lu of %lu is wrong\n", count, total);
    return 0;
}
