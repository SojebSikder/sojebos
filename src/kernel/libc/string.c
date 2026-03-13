#include <stddef.h>

int strcmp(const char *s1, const char *s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i])
            return s1[i] - s2[i]; // return difference
        i++;
    }
    // If one string ended, return difference of last characters
    return s1[i] - s2[i];
}

// atoi
int atoi(const char *str) {
  int result = 0;
  while (*str >= '0' && *str <= '9') {
    result = result * 10 + (*str - '0');
    str++;
  }
  return result;
}


// to lower case
char *strlower(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] >= 'A' && str[i] <= 'Z') {
      str[i] = str[i] + 32;
    }
  }
  return str;
}

// to upper case
char *strupper(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      str[i] = str[i] - 32;
    }
  }
  return str;
}


// strlen
size_t strlen(const char *str) {
    size_t i = 0;
    while (str[i] != '\0') {
        i++;
    }
    return i;
}
