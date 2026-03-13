#ifndef MEM_H
#define MEM_H

void memory_copy(void *dest, const void *src, int n);
void memory_set(void *dest, int c, int n);
int memory_compare(const void *s1, const void *s2, int n);

#endif
