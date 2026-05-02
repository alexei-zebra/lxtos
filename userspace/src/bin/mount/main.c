#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <ulib/syscall.h>
#include <ulib/path.h>

void main(int argc, char **argv)
{
	// some like this:
    // argv0 = "/bin/mount"
    // argv1 = source
    // argv2 = mountpoint
    // argv3 = fstype (optional, idk)
    if (argc < 2)
    {
        puts("usage: mkdir <path>\n");
        exit(0);
    }
    char path[256];
    u_resolve_path(argv[1], path, 256);
    sys_mkdir(path) == 0 ? puts("ok\n") : puts("failed\n");
    exit(0);
}
