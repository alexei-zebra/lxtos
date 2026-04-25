#include <ulib/io.h>
#include <ulib/syscall.h>
#include <ulib/path.h>

void main(int argc, char **argv)
{
    if (argc < 2) { u_puts("\nusage: touch <file>\n"); sys_exit(); }
    char path[256];
    u_resolve_path(argv[1], path, 256);
    sys_mkfile(path) == 0 ? u_puts("\nok\n") : u_puts("\nfailed\n");
    sys_exit();
}