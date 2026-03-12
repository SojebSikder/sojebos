#include "mem.h"


void memory_copy(void *dest, const void *src, int n) {
    char *d = dest;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
    }
}

void memory_set(void *dest, int c, int n) {
    char *d = dest;
    while (n--) {
        *d++ = c;
    }
}
