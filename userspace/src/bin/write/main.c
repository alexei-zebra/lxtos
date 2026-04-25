#include <ulib/io.h>
#include <ulib/string.h>
#include <ulib/syscall.h>

void main(int argc, char **argv)
{
    if (argc < 3) { u_puts("\nusage: write <file> <text>\n"); sys_exit(); }

    char buf[512];
    int pos = 0;
    for (int i = 2; i < argc; i++) {
        int j = 0;
        while (argv[i][j] && pos < (int)sizeof(buf)-1)
            buf[pos++] = argv[i][j++];
        if (i != argc-1 && pos < (int)sizeof(buf)-1)
            buf[pos++] = ' ';
    }
    buf[pos] = 0;

    sys_write(argv[1], buf, u_strlen(buf)) >= 0 ? u_puts("\nok\n") : u_puts("\nfailed\n");
    sys_exit();
}