#include <ulib/io.h>
#include <ulib/syscall.h>

void main(int argc, char **argv)
{
    if (argc < 2) { u_puts("\nusage: mkdir <path>\n"); sys_exit(); }
    sys_mkdir(argv[1]) == 0 ? u_puts("\nok\n") : u_puts("\nfailed\n");
    sys_exit();
}