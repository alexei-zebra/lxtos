#include <stdbool.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/stdlib.h>
#include <ulib/syscall.h>
#include <ulib/args.h>

#define BLUE  "\033[34m"
#define GREEN "\033[32m"
#define RESET "\033[0m"

static int is_elf(const char *path)
{
    int fd = sys_open(path);
    if (fd < 0)
        return 0;
    unsigned char buf[4];
    int n = sys_fread(fd, buf, 4);
    sys_close(fd);
    return (n == 4 &&
            buf[0] == 0x7F &&
            buf[1] == 'E' &&
            buf[2] == 'L' &&
            buf[3] == 'F');
}

int main(int argc, char *argv[])
{
    char cwdbuf[256];
    const char *path;
    bool b_list = false, b_all = false, b_path = false;
    int opt;

    while ((opt = getopt(argc, argv, "al")) != -1)
        switch (opt) {
            case 'a': b_all  = true; break;
            case 'l': b_list = true; break;
        }

    if (optind == argc - 1) {
        b_path = true;
        path = argv[optind];
    }
    if (!b_path) {
        sys_getcwd(cwdbuf, sizeof(cwdbuf));
        path = cwdbuf;
    }

    char name[256];
    char full[512];
    uint32_t i = 0;
    int64_t r;

    while ((r = sys_readdir(path, i, name)) != (int64_t)-1) {
        i++;
        if (name[0] == '.' && !b_all)
            continue;

        snprintf(full, sizeof(full), "%s/%s", path, name);

        if (r == 1) {
            printf(BLUE "%s/" RESET "%s", name, b_list ? "\n" : "  ");
        } else if (is_elf(full)) {
            printf(GREEN "%s" RESET "%s", name, b_list ? "\n" : "  ");
        } else {
            printf("%s%s", name, b_list ? "\n" : "  ");
        }
    }

    if (!b_list)
        printf("\n");

    exit(0);
}
