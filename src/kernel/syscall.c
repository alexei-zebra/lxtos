#include <kernel/syscall.h>
#include <uapi/syscall_nr.h>
#include <console/kprint.h>
#include <drivers/keyboard.h>
#include <drivers/framebuffer.h>
#include <lib/kstring.h>
#include <fs/vfs.h>
#include <exec/elf.h>
#include <fs/procfs.h>
#include <fs/tmpfs.h>
#include <fs/ext2.h>
#include <drivers/ata.h>

static char kernel_cwd[256] = "/";

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
    case SYS_CHDIR: {
        const char *path = (const char *)a1;
        vfs_node_t *node = vfs_resolve(path);
        if (!node || !(node->flags & VFS_FLAG_DIR)) return (uint64_t)-1;
        kstrcpy(kernel_cwd, path, 256);
        return 0;
    }   
    case SYS_GETCWD: {
        char    *buf  = (char *)a1;
        uint64_t size = a2;
        kstrcpy(buf, kernel_cwd, size);
        return 0;
    }
    case SYS_MOUNT: {
        const char *source     = (const char *)a1;
        const char *mountpoint = (const char *)a2;
        const char *fstype     = (const char *)a3; // proc, tmpfs, ext2..

        vfs_node_t *target = vfs_resolve(mountpoint);
        if (!target || !(target->flags & VFS_FLAG_DIR))
            return (uint64_t)-1;

        vfs_node_t *fs = NULL;

        if (kstrcmp(fstype, "proc") == 0) {
            fs = procfs_create();
            if (!fs) return (uint64_t)-1;
            procfs_register("mem",     proc_mem_read);
            procfs_register("cpuinfo", proc_cpu_read);

        } else if (kstrcmp(fstype, "tmpfs") == 0) {
            fs = tmpfs_create();
            if (!fs) return (uint64_t)-1;

        } else if (kstrcmp(fstype, "ext2") == 0) {
            int bus = source[0] - '0';
            int drv = source[2] - '0';
            fs = ext2_mount(bus, drv);
            if (!fs) return (uint64_t)-1;

        } else {
            fs = vfs_resolve(source);
            if (!fs) return (uint64_t)-1;
        }

        int r = vfs_mount(mountpoint, fs);
        return r == 0 ? 0 : (uint64_t)-1;
    }   
    case SYS_UMOUNT: {
        const char *mountpoint = (const char *)a1;

        char parent[256];
        const char *last = mountpoint;
        int slash = -1;
        for (int i = 0; mountpoint[i]; i++)
            if (mountpoint[i] == '/') slash = i;

        if (slash <= 0) {
            parent[0] = '/'; parent[1] = '\0';
            last = mountpoint + slash + 1;
        } else {
            kstrcpy(parent, mountpoint, slash + 1);
            parent[slash] = '\0';
            last = mountpoint + slash + 1;
        }

        vfs_node_t *parent_node = vfs_resolve(parent);
        if (!parent_node || !(parent_node->flags & VFS_FLAG_DIR))
            return (uint64_t)-1;

        if (!parent_node->ops || !parent_node->ops->finddir)
            return (uint64_t)-1;

        vfs_node_t *target = parent_node->ops->finddir(parent_node, last);
        if (!target || !(target->flags & VFS_FLAG_MOUNTPT))
            return (uint64_t)-1;

        target->mount = NULL;
        target->flags &= ~VFS_FLAG_MOUNTPT;
        return 0;
    }

    default:
        return (uint64_t)-1;
    }
}
