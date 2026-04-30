#include <ulib/io.h>
#include <ulib/string.h>
#include <ulib/syscall.h>

void main(int argc, char **argv)
{
    // input

    char cwdbuf[256];
    const char *path;
    char b_list = 0;
    char b_all = 0;
    char b_path = 0;

    if (argc > 1) {
        const char *arg;
        for (int i = 1; i < argc; i++) {
            arg = argv[i];
            if (arg[0] == '-')
                for (char *ops = arg + 1; *ops; ops++) {
                    switch (*ops) {
                    case 'l':
                        b_list = 1;
                        break;
                    case 'a':
                        b_all = 1;
                        break;
                    }
                }
            else {
                b_path = 1;
                path = arg;
            }
        }
    }

    if (!b_path) {
        sys_getcwd(cwdbuf, sizeof(cwdbuf));
        path = cwdbuf;
    }


    // output

    char name[256];
    uint32_t i = 0;
    int64_t r;

    u_puts("\n");
    while ((r = sys_readdir(path, i, name)) != (int64_t)-1) {
        i++;
        if (name[0] == '.' && !b_all) continue;
        u_puts(name);
        if (r == 1) u_puts("/");
        u_puts(b_list ? "\n" : "  ");
    }
    u_puts("\n");
    sys_exit();
}
