#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <ulib/syscall.h>


static char hostname[64];
static char osname[64];

static char mem_total[64];
static char mem_free[64];
static char heap_total[64];
static char heap_free[64];

static char cpu_vendor[64];
static char cpu_model[128];

static char art[2048];


static void read_file(const char *path, char *buf, int max)
{
    int fd = sys_open(path);
    if (fd < 0) {
        buf[0] = 0;
        return;
    }

    int64_t n = sys_fread(fd, buf, max - 1);
    if (n < 0)
            n = 0;

    buf[n] = 0;

    if (n > 0 && buf[n - 1] == '\n')
            buf[n - 1] = 0;

    sys_close(fd);
}

void main(void)
{
    read_file("/etc/hostname", hostname, sizeof(hostname));
    read_file("/etc/osname",   osname,   sizeof(osname));

    read_file("/proc/mem_total",  mem_total,  sizeof(mem_total));
    read_file("/proc/mem_free",   mem_free,   sizeof(mem_free));
    read_file("/proc/heap_total", heap_total, sizeof(heap_total));
    read_file("/proc/heap_free",  heap_free,  sizeof(heap_free));

    read_file("/proc/cpu_vendor", cpu_vendor, sizeof(cpu_vendor));
    read_file("/proc/cpu_model",  cpu_model,  sizeof(cpu_model));

    read_file("/tmp/silex_kernel.txt", art, sizeof(art));

    printf("\n");
    printf("%s\n", art);

    printf("%s@%s\n", hostname, osname);
    printf("----------------\n");

    printf("OS:        %s\n", osname);
    printf("Host:      %s\n", hostname);

    printf("Memory:    %s total / %s free\n", mem_total, mem_free);
    printf("Heap:      %s total / %s free\n", heap_total, heap_free);

    printf("CPU:       %s\n", cpu_model);
    printf("Vendor:    %s\n", cpu_vendor);

    printf("\n");

    exit(0);
}
