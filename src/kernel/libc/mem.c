#include "mem.h"


void memcpy(void *dest, const void *src, int n) {
    char *d = dest;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
    }
}

void memset(void *dest, int c, int n) {
    char *d = dest;
    while (n--) {
        *d++ = c;
    }
}

// memcmp
int memcmp(const void *s1, const void *s2, int n) {
    const char *a = s1;
    const char *b = s2;
    while (n--) {
        if (*a != *b) return *a - *b;
        a++;
        b++;
    }
    return 0;
}
