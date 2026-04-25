#include <ulib/io.h>
#include <ulib/string.h>
#include <ulib/syscall.h>
#include <ulib/path.h>

void main(int argc, char **argv)
{
    if (argc < 2) { u_puts("\nusage: cat <file>\n"); sys_exit(); }
    char path[256];
    u_resolve_path(argv[1], path, 256);
    static char buf[512];
    int64_t n = sys_read(path, buf, sizeof(buf)-1);
    if (n < 0) { u_puts("\nnot found\n"); sys_exit(); }
    buf[n] = 0;
    u_puts("\n"); u_puts(buf); u_puts("\n");
    sys_exit();
}