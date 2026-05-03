#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <ulib/syscall.h>
#include <ulib/path.h>

void main(int argc, char **argv)
{
    if (argc < 3)
    {
        puts("usage: write <file> <text>\n");
        exit(0);
    }

    char path[256];
    u_resolve_path(argv[1], path, 256);

    int fd = sys_open(path);
    if (fd < 0)
    {
        puts("failed\n");
        exit(0);
    }

    char buf[512];
    int pos = 0;

    for (int i = 2; i < argc; i++)
    {
        int j = 0;
        while (argv[i][j] && pos < (int)sizeof(buf) - 1)
            buf[pos++] = argv[i][j++];

        if (i != argc - 1 && pos < (int)sizeof(buf) - 1)
            buf[pos++] = ' ';
    }

    buf[pos] = 0;

    int64_t n = sys_fwrite(fd, buf, strlen(buf));

    sys_close(fd);

    n >= 0 ? puts("ok\n") : puts("failed\n");

    exit(0);
}
