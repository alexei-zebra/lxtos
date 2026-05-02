#include <stdbool.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/stdlib.h>
#include <ulib/syscall.h>
#include <ulib/args.h>

int main(int argc, char *argv[])
{
    char cwdbuf[256];
    const char *path;
    bool b_list = false,
         b_all = false,
         b_path = false;

    int opt;
    while ((opt = getopt(argc, argv, "al")) != -1) {
        switch (opt) {
            case 'a': b_all = true; break;
            case 'l': b_list = true; break;
            case '?': printf("unknown/invalid option\n"); break;
        }
    }

    if (optind == argc - 1) {
        b_path = true;
        path = argv[optind];
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
