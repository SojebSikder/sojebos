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

// int to string
void itoa(int num, char *str) {
  int i = 0;
  int is_negative = 0;

  if (num == 0) {
    str[i++] = '0';
    str[i] = '\0';
    return;
  }

  // Handle negative numbers
  if (num < 0) {
    is_negative = 1;
    unsigned int u_num = (unsigned int)-num;
    while (u_num > 0) {
      str[i++] = (u_num % 10) + '0';
      u_num /= 10;
    }
  } else {
    while (num > 0) {
      str[i++] = (num % 10) + '0';
      num /= 10;
    }
  }

  if (is_negative) {
    str[i++] = '-';
  }

  str[i] = '\0';

  // Now REVERSE the string to get the correct order
  int start = 0;
  int end = i - 1;
  while (start < end) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }
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
