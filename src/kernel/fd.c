#include <kernel/fd.h>
#include <lib/kmalloc.h>


static fd_entry_t fd_table[FD_MAX];


void fd_table_init(void)
{
    for (int i = 0; i < FD_MAX; i++) {
        fd_table[i].used   = 0;
        fd_table[i].node   = NULL;
        fd_table[i].offset = 0;
    }
}

int alloc_fd(vfs_node_t *node)
{
    for (int i = 0; i < FD_MAX; i++) {
        if (!fd_table[i].used) {
            fd_table[i].node   = node;
            fd_table[i].offset = 0;
            fd_table[i].used   = 1;
            return i;
        }
    }
    return -1;
}

fd_entry_t *get_fd(int fd)
{
    if (fd < 0 || fd >= FD_MAX)
            return NULL;
    if (!fd_table[fd].used)
            return NULL;
    return &fd_table[fd];
}

void free_fd(int fd)
{
    if (fd < 0 || fd >= FD_MAX)
            return;
    fd_table[fd].used   = 0;
    fd_table[fd].node   = NULL;
    fd_table[fd].offset = 0;
}
