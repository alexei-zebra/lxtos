#pragma once
#include <stdint.h>
#include <stddef.h>

void  kmalloc_init(void *heap_start, uint64_t heap_size);
void *kmalloc(uint64_t size);
void *kzalloc(uint64_t size);
void  kfree(void *ptr);
uint64_t kmalloc_free_size(void);
uint64_t kmalloc_heap_size(void);
