#include <ulib/io.h>
#include <ulib/string.h>
#include <ulib/syscall.h>

void main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "/"; // just folder
    char name[256]; // buffer for readdir
    uint32_t i = 0; // index, 0 = first element, 1 = second element..
    int64_t r; // return readdir value

    u_puts("\n");
    // sys_readdir(path, index, name_buf)
    // return 1 if folder
    // return 0 if file
    // return -1 if no have elements
    while ((r = sys_readdir(path, i, name)) != (int64_t)-1) {
        u_puts("  ");
        u_puts(name);
        if (r == 1) u_puts("/");
        u_puts("\n");
        i++; // go to next element
    }
    sys_exit();
}