#include <kernel/syscall.h>
#include <uapi/syscall_nr.h>
#include <console/kprint.h>
#include <drivers/keyboard.h>
#include <drivers/framebuffer.h>
#include <lib/kstring.h>
#include <fs/vfs.h>
#include <exec/elf.h>

uint64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3)
{
    switch (num) {
    case SYS_PUTCHAR:
        fb_putchar_cursor((char)a1, 0xFFFFFF, 0x0D0D1A);
        return 0;
    case SYS_PUTS: {
        const char *s = (const char *)a1;
        while (*s)
            fb_putchar_cursor_utf8(*s++, 0xFFFFFF, 0x0D0D1A);
        return 0;
    }
    case SYS_GETCHAR:
        return kb_getchar();
    case SYS_EXIT:
        elf_exec("/bin/shell");
        for (;;) __asm__ volatile("hlt");
    case SYS_GETS: {
        char    *buf = (char *)a1;
        uint64_t max = a2;
        uint64_t i   = 0;
        while (i < max - 1) {
            char c = kb_getchar();
            if (c == '\n') { fb_putchar_cursor('\n', 0xFFFFFF, 0x0D0D1A); break; }
            if (c == '\b') {
                if (i > 0) { i--; fb_putchar_cursor('\b', 0xFFFFFF, 0x0D0D1A); }
                continue;
            }
            buf[i++] = c;
            fb_putchar_cursor(c, 0xFFFFFF, 0x0D0D1A);
        }
        buf[i] = 0;
        return i;
    }
    case SYS_CLEAR:
        fb_fill(COLOR_BG);
        fb_cursor_x = 0;
        fb_cursor_y = 0;
        return 0;
    case SYS_READDIR: {
        const char *path = (const char *)a1;
        uint32_t    idx  = (uint32_t)a2;
        char       *out  = (char *)a3;
        vfs_node_t *dir  = vfs_resolve(path);
        if (!dir || !(dir->flags & VFS_FLAG_DIR)) return (uint64_t)-1;
        vfs_node_t *e = vnode_readdir(dir, idx);
        if (!e) return (uint64_t)-1;
        kstrcpy(out, e->name, 256);
        return e->flags & VFS_FLAG_DIR ? 1 : 0;
    }
    case SYS_MKDIR: {
        vfs_node_t *n = vfs_mkdir((const char *)a1);
        return n ? 0 : (uint64_t)-1;
    }
    case SYS_MKFILE: {
        vfs_node_t *n = vfs_mkfile((const char *)a1);
        return n ? 0 : (uint64_t)-1;
    }
    case SYS_RM:
        return (uint64_t)vfs_unlink((const char *)a1);
    case SYS_READ: {
        vfs_node_t *node = vfs_resolve((const char *)a1);
        if (!node || (node->flags & VFS_FLAG_DIR)) return (uint64_t)-1;
        return (uint64_t)vnode_read(node, (void *)a2, 0, a3);
    }
    case SYS_WRITE: {
        const char *path = (const char *)a1;
        const void *data = (const void *)a2;
        uint64_t    size = a3;
        vfs_node_t *node = vfs_resolve(path);
        if (!node) {
            node = vfs_mkfile(path);
            if (!node) return (uint64_t)-1;
        }
        if (node->flags & VFS_FLAG_DIR) return (uint64_t)-1;
        return (uint64_t)vnode_write(node, data, 0, size);
    }
    case SYS_EXEC: {
    const char *path     = (const char *)a1;
    const char **uargv   = (const char **)a2;
    return (uint64_t)elf_exec_argv(path, uargv);
    }
    case SYS_FSIZE: {
        vfs_node_t *node = vfs_resolve((const char *)a1);
        if (!node || (node->flags & VFS_FLAG_DIR)) return (uint64_t)-1;
        return node->size;
    }
    default:
        return (uint64_t)-1;
    }
}
