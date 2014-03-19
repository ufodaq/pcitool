#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    long i, num;
    size_t count = 0, total = 0, size;
    int offset = 3, toread = 1, toskip = 3;
    uint32_t value;
    uint32_t *buf;
    uint32_t expected = 0;
    uint32_t blocks = 0, status_good = 0;
    
    char fixed[4096];
    struct stat st_buf;
    
    if ((argc != 2)&&(argc != 5)) {
	printf("Usage: %s <file> [offset_dwords read_dwords skip_dwords] \n", argv[0]);
	exit(0);
    }
    
    FILE *f = fopen(argv[1], "r");
    if (!f) {
	printf("Can't open %s\n", argv[1]);
	exit(1);
    }

    stat(argv[1], &st_buf);
    size = st_buf.st_size / sizeof(uint32_t);


    buf = malloc(size * sizeof(uint32_t));
    if (!buf) {
	printf("Can't allocate %lu bytes of memory\n", size * sizeof(uint32_t));
	exit(1);
    }
    
    if (argc == 5) {
	offset = atoi(argv[2]);
	toread = atoi(argv[3]);
	toskip = atoi(argv[4]);
    }
    

    num = fread(buf, 4, size, f);
    if (num != size) {
	printf("Failed to read %lu dwords, only %lu read\n", size, num);
	exit(1);
    }
    fclose(f);
    
    sprintf(fixed, "%s.fixed", argv[1]);
    f = fopen(fixed, "w");
    if (!f) {
	printf("Failed to open %s for output\n", fixed);
	exit(1);
    }
    
    expected = (buf[offset]>>24) + 2;
    for (i = 1; i < size; i += (toread + toskip)) {
	total++;
	    
	value = buf[i + offset] >> 24;
//	printf("0x%lx: value (%lx) = expected (%lx)\n", i + offset, value, expected);
	if (value == expected) {
	    if (!status_good) {
		status_good = 1;
		blocks++;
	    }
	    fwrite(&buf[i], 4, toread + toskip, f);
	    expected += 2;
	    if (expected == 0xb8) 
		expected = 0;
	} else if ((!status_good)&&(value == 0)&&((i + toread + toskip)< size)) {
	    value = buf[i + offset + toread + toskip] >> 24;
	    if (value == 2) {
		status_good = 1;
		blocks++;
		fwrite(&buf[i], 4, toread + toskip, f);
		expected = 2;
	    } else {
		count++;
	    }
	} else {
	    printf("0x%lx: value (%x) = expected (%x)\n", (i + offset)*sizeof(uint32_t), value, expected);
	    status_good = 0;
	    count++;
	}
    }
    fclose(f);
    free(buf);
    
    printf("%lu of %lu is wrong\n", count, total);
    return 0;
}
