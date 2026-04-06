#pragma once
#include <fs/vfs.h>
#include <stdint.h>

int initramfs_unpack(void *data, uint64_t size, vfs_node_t *root);
