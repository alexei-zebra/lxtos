#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <ulib/syscall.h>
#include <ulib/path.h>


void main(int argc, char **argv)
{
    if (argc < 2) {
        puts("usage: umount <mountpoint>\n");
        exit(0);
    }
    char mountpoint[256];
    u_resolve_path(argv[1], mountpoint, 256);
    sys_umount(mountpoint) == 0 ? puts("unmounted successfully\n") : puts("umount failed\n");

    exit(0);
}
