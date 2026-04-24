#include <stddef.h>
#include <stdint.h>

#ifndef STRING_H
#define STRING_H

int strcmp(const char *s1, const char *s2);
int atoi(const char *str);
char *strlower(char *str);
char *strupper(char *str);
size_t strlen(const char *str);
void itoa(int num, char *str);
char *strchr(const char *str, int c);

//
// memory manipulation
//

void memcpy(void *dest, const void *src, int n);
void memset(void *dest, int c, int n);
int memcmp(const void *s1, const void *s2, int n);

//
// net
//

uint32_t parse_ip(char *ip);

#endif
