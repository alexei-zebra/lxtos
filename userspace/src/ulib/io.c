#include <ulib/io.h>
#include <ulib/syscall.h>


void u_puts(const char *s)
{
    sys_puts(s);
}

void u_gets(char *buf, int max)
{
    sys_gets(buf, (uint64_t)max);
}
