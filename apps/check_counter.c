#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    int block = 0;
    uint32_t value = 0;
    uint32_t buf[1024];
    
    if (argc < 2) {
	printf("Usage:\n\t\t%s <file-to-check>\n", argv[0]);
	exit(0);
    }
    
    FILE *f = fopen(argv[1], "r");
    if (!f) {
	printf("Failed to open file %s\n", argv[1]);
	exit(1);
    }
    
    
    while (!feof(f)) {
	int i, n = fread(buf, 4, 1024, f);

	if (block) i = 0;
	else {
	    i = 1;
	    value = (buf[0]);
	}

	for (; i < n; i++) {
	    if ((buf[i]) != ++value) {
		printf("Pos %lx (Block %i, dword %i) expected %x, but got %x\n", block * 4096 + i * 4, block, i, value, (buf[i]));
		exit(1);
	    }
	}
	
	if (n) block++;
    }

    fclose(f);
    
    printf("Checked %i blocks. All is fine\n", block);
    return 0;
}
