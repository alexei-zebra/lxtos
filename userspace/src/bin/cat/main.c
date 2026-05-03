#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <ulib/syscall.h>
#include <ulib/path.h>

void main(int argc, char **argv)
{
    if (argc < 2)
    {
        puts("usage: cat <file>\n");
        exit(0);
    }

    char path[256];
    u_resolve_path(argv[1], path, 256);

    int fd = sys_open(path);
    if (fd < 0)
    {
        puts("not found\n");
        exit(0);
    }

    static char buf[512];
    int64_t n = sys_fread(fd, buf, sizeof(buf) - 1);

    if (n < 0)
    {
        sys_close(fd);
        puts("read error\n");
        exit(0);
    }

    buf[n] = 0;
    printf("%s\n", buf);

    sys_close(fd);
    exit(0);
}
