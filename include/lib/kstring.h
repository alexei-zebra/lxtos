#pragma once
#include <stdint.h>

int   kstrlen (const char *s);
int   kstrcmp (const char *a, const char *b);
int   kstrncmp(const char *a, const char *b, int n);
void  kstrcpy (char *dst, const char *src, int max);
void  kmemcpy (void *dst, const void *src, uint64_t n);
void  kmemset (void *dst, uint8_t val, uint64_t n);
