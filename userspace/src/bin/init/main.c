#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <ulib/syscall.h>

static void log(const char *msg)
{
    printf("[init] %s\n", msg);
}

static void mkdir_safe(const char *path)
{
    sys_mkdir(path);
}

static int mount_ext2(void)
{
    char src[4] = "0:0";
    for (int bus = 0; bus < 2; bus++)
    {
        for (int drv = 0; drv < 2; drv++)
        {
            src[0] = '0' + bus;
            src[2] = '0' + drv;
            if (sys_mount(src, "/mnt", "ext2") == 0)
            {
                log("ext2 mounted at /mnt");
                return 1;
            }
        }
    }
    return 0;
}

static void print_motd(void)
{
    static char buf[4096];

    int fd = sys_open("/etc/motd");
    if (fd < 0) return;

    int64_t n = sys_fread(fd, buf, sizeof(buf) - 1);

    if (n > 0)
    {
        buf[n] = '\0';
        printf("%s", buf);
    }

    sys_close(fd);
}

void main(int argc, char **argv)
{
    log("starting");
    log("setting up filesystem hierarchy");
    mkdir_safe("/dev");
    mkdir_safe("/proc");
    mkdir_safe("/etc");
    mkdir_safe("/tmp");
    mkdir_safe("/mnt");

    log("mounting procfs");
    if (sys_mount("proc", "/proc", "proc") != 0)
        log("WARNING: failed to mount procfs");

    log("scanning for ext2 disk");
    if (!mount_ext2())
        log("WARNING: no ext2 disk found");

    print_motd();
    log("exec /bin/shell");
    const char *shell_argv[] = { "/bin/shell", 0 };
    sys_exec("/bin/shell", shell_argv);

    puts("[init] FATAL: /bin/shell exited");
    exit(0);
}

