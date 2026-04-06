#pragma once
#include <fs/vfs.h>

typedef int64_t (*procfs_read_fn)(void *buf, uint64_t size);

vfs_node_t *procfs_create(void);
vfs_node_t *procfs_register(const char *name, procfs_read_fn fn);
