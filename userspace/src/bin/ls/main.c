#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/stdlib.h>
#include <ulib/syscall.h>

void main(int argc, char **argv)
{
    char cwdbuf[256];
    const char *path;
    char b_list = 0, b_all = 0, b_path = 0;

    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            const char *arg = argv[i];
            if (arg[0] == '-')
            {
                for (char *ops = (char*)arg + 1; *ops; ops++)
                {
                    if (*ops == 'l') b_list = 1;
                    if (*ops == 'a') b_all  = 1;
                }
            }
            else
            {
                b_path = 1;
                path = arg;
            }
        }
    }

    if (!b_path)
    {
        sys_getcwd(cwdbuf, sizeof(cwdbuf));
        path = cwdbuf;
    }

    char name[256];
    uint32_t i = 0;
    int64_t r;

    printf("");
    while ((r = sys_readdir(path, i, name)) != (int64_t)-1)
    {
        i++;
        if (name[0] == '.' && !b_all) continue;
        printf("%s%s%s", name, r == 1 ? "/" : "", b_list ? "\n" : "  ");
    }
    printf("");
    exit(0);
}
