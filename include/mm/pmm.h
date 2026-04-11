#pragma once
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE  4096ULL
#define PAGE_SHIFT 12

void     pmm_init(void);
uint64_t pmm_alloc(void);
void     pmm_free(uint64_t phys);
uint64_t pmm_free_pages(void);
uint64_t pmm_total_pages(void);
uint64_t pmm_alloc_n(uint64_t n);
uint64_t pmm_total_bytes(void);
