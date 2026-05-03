#pragma once

#include <stdint.h>
#include <fs/vfs.h>

#define FD_MAX 64


typedef struct {
    vfs_node_t *node;
    uint64_t    offset;
    int         used;
} fd_entry_t;


int         alloc_fd(vfs_node_t *node);
fd_entry_t *get_fd(int fd);
void        free_fd(int fd);
void        fd_table_init(void);
