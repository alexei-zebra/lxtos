#pragma once
#include <stdint.h>

void kputdec(uint64_t val);
void kputhex(uint64_t val);
void kputs_col(const char *s, uint32_t fg);
void kputs(const char *s);
