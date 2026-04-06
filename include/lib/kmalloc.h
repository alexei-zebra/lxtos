#pragma once
#include <stdint.h>
#include <stddef.h>

void  kmalloc_init(void *heap_start, uint64_t heap_size);
void *kmalloc(uint64_t size);
void *kzalloc(uint64_t size);
void  kfree(void *ptr);
