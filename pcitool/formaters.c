#include <stdio.h>

void PrintTime(size_t duration) {
    if (duration > 999999999999) printf("%4.1lf""d", 1.*duration/86400000000);
    else if (duration > 99999999999) printf("%4.1lf""h", 1.*duration/3600000000);
    else if (duration > 9999999999) printf("%4.2lf""h", 1.*duration/3600000000);
    else if (duration > 999999999) printf("%4.1lf""m", 1.*duration/60000000);
    else if (duration > 99999999) printf("%4.2lf""m", 1.*duration/60000000);
    else if (duration > 9999999) printf("%4.1lf""s", 1.*duration/1000000);
    else if (duration > 999999) printf("%4.2lf""s", 1.*duration/1000000);
    else if (duration > 999) printf("%3lu""ms", duration/1000);
    else printf("%3lu""us", duration);
}

void PrintNumber(size_t num) {
    if (num > 999999999999999999) printf("%3lue", num/1000000000000000000);
    else if (num > 999999999999999) printf("%3lup", num/1000000000000000);
    else if (num > 999999999999) printf("%3lut", num/1000000000000);
    else if (num > 999999999) printf("%3lug", num/1000000000);
    else if (num > 999999) printf("%3lum", num/1000000);
    else if (num > 9999) printf("%3luk", num/1000);
    else printf("%4lu", num);
}

void PrintSize(size_t num) {
    if (num >= 112589990684263) printf("%4.1lf PB", 1.*num/1125899906842624);
    else if (num >= 109951162778) printf("%4.1lf TB", 1.*num/1099511627776);
    else if (num >= 107374183) printf("%4.1lf GB", 1.*num/1073741824);
    else if (num >= 1048576) printf("%4lu MB", num/1048576);
    else if (num >= 1024) printf("%4lu KB", num/1024);
    else printf("%5lu B", num);
}

void PrintPercent(size_t num, size_t total) {
    if (num >= total) printf(" 100");
    else printf("%4.1lf", 100.*num/total);
    
}

char *GetPrintSize(char *str, size_t size) {
    if (size >= 1073741824) sprintf(str, "%.1lf GB", 1.*size / 1073741824);
    else if (size >= 1048576) sprintf(str, "%.1lf MB", 1.*size / 1048576);
    else if (size >= 1024) sprintf(str, "%lu KB", size / 1024);
    else sprintf(str, "%lu B ", size);
    
    return str;
}
