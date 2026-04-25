#include <ulib/io.h>
#include <ulib/syscall.h>

void main(int argc, char **argv)
{
    if (argc < 2) { u_puts("\nusage: cat <file>\n"); sys_exit(); }
    static char buf[512];
    int64_t n = sys_read(argv[1], buf, sizeof(buf)-1);
    if (n < 0) { u_puts("\nnot found\n"); sys_exit(); }
    buf[n] = 0;
    u_puts("\n");
    u_puts(buf);
    u_puts("\n");
    sys_exit();
}