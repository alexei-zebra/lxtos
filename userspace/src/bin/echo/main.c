#include <ulib/io.h>
#include <ulib/syscall.h>

void main(int argc, char **argv)
{
    u_puts("\n");
    for (int i = 1; i < argc; i++) {
        u_puts(argv[i]);
        if (i != argc-1) u_puts(" ");
    }
    u_puts("\n");
    sys_exit();
}