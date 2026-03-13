#include <stddef.h>

#ifndef STRING_H
#define STRING_H

int strcmp(const char *s1, const char *s2);
int atoi(const char *str);
char *strlower(char *str);
char *strupper(char *str);
size_t strlen(const char *str);
void itoa(int num, char *str);


#endif
