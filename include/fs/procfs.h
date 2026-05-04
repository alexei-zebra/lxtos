#pragma once

#include <fs/vfs.h>


typedef int64_t (*procfs_read_fn)(void *buf, uint64_t size);


// Create the /proc filesystem
vfs_node_t *procfs_create(void);

// Register a new /proc entry
vfs_node_t *procfs_register(const char *name, procfs_read_fn fn);

// Total memory read
int64_t proc_mem_total_read(void *buf, uint64_t max);

// Free memory read
int64_t proc_mem_free_read(void *buf, uint64_t max);

// Heap total
int64_t proc_heap_total_read(void *buf, uint64_t max);

// Heap free
int64_t proc_heap_free_read(void *buf, uint64_t max);

// CPU vendor
int64_t proc_cpu_vendor_read(void *buf, uint64_t max);

// Model CPU
int64_t proc_cpu_model_read(void *buf, uint64_t max);
