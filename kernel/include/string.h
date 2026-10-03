#ifndef THEOS_STRING_H
#define THEOS_STRING_H

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *dest, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);

size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);

/* Formats value into buf (base 10 or 16, unsigned). buf must be large
 * enough (11 bytes covers 32-bit decimal, 9 covers 32-bit hex). */
void utoa(unsigned int value, char *buf, int base);

#endif
