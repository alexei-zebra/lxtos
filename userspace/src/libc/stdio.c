#include <libc/stdio.h>
#include <libc/string.h>
#include <ulib/syscall.h>
#include <stdarg.h>


void puts(const char *s)
{
    sys_puts(s);
}

void gets(char *buf, int max)
{
    sys_gets(buf, (uint64_t)max);
}

void putchar(char c)
{
    char buf[2] = {c, 0};
    sys_puts(buf);
}

static int itoa_buf(long n, int base, char *out)
{
    static const char digits[] = "0123456789abcdef";
    char tmp[32];
    int i = 0, neg = 0;
    if (n == 0) {
        out[0] = '0';
        out[1] = 0;
        return 1;
    }
    if (n < 0 && base == 10) {
        neg = 1;
        n = -n;
    }
    while (n) {
        tmp[i++] = digits[n % base];
        n /= base;
    }
    if (neg)
            tmp[i++] = '-';
    int j = 0;
    while (i--)
            out[j++] = tmp[i];
    out[j] = 0;
    return j;
}

static int utoa_buf(unsigned long n, int base, char *out)
{
    static const char digits[] = "0123456789abcdef";
    char tmp[32];
    int i = 0;
    if (n == 0) {
        out[0] = '0';
        out[1] = 0;
        return 1;
    }
    while (n) {
        tmp[i++] = digits[n % base];
        n /= base;
    }
    int j = 0;
    while (i--)
            out[j++] = tmp[i];
    out[j] = 0;
    return j;
}

void printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    static char out[1024];
    int pos = 0;
    char tmp[64];

    for (const char *p = fmt; *p && pos < 1023; p++) {
        if (*p != '%') {
            out[pos++] = *p;
            continue;
        }
        p++;
        switch (*p) {
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s) s = "(null)";
                while (*s && pos < 1023)
                        out[pos++] = *s++;
                break;
            }
            case 'd':
                itoa_buf((long)va_arg(ap, int), 10, tmp);
                for (int i = 0; tmp[i] && pos < 1023; i++)
                        out[pos++] = tmp[i];
                break;
            case 'u':
                utoa_buf((unsigned long)va_arg(ap, unsigned int), 10, tmp);
                for (int i = 0; tmp[i] && pos < 1023; i++)
                        out[pos++] = tmp[i];
                break;
            case 'x':
                utoa_buf((unsigned long)va_arg(ap, unsigned int), 16, tmp);
                for (int i = 0; tmp[i] && pos < 1023; i++)
                        out[pos++] = tmp[i];
                break;
            case 'l':
                p++;
                if (*p == 'u')
                        utoa_buf(va_arg(ap, unsigned long), 10, tmp);
                else if (*p == 'x')
                        utoa_buf(va_arg(ap, unsigned long), 16, tmp);
                else
                        itoa_buf(va_arg(ap, long), 10, tmp);
                for (int i = 0; tmp[i] && pos < 1023; i++)
                        out[pos++] = tmp[i];
                break;
            case 'c':
                out[pos++] = (char)va_arg(ap, int);
                break;
            case '%':
                out[pos++] = '%';
                break;
            default:
                out[pos++] = '%';
                out[pos++] = *p;
                break;
        }
    }

    out[pos] = 0;
    sys_puts(out);

    va_end(ap);
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int pos = 0;
    char tmp[64];

    for (const char *p = fmt; *p && pos < (int)size - 1; p++) {
        if (*p != '%') {
            buf[pos++] = *p;
            continue;
        }
        p++;
        switch (*p) {
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s) s = "(null)";
                while (*s && pos < (int)size - 1)
                    buf[pos++] = *s++;
                break;
            }
            case 'd':
                itoa_buf((long)va_arg(ap, int), 10, tmp);
                for (int i = 0; tmp[i] && pos < (int)size - 1; i++)
                    buf[pos++] = tmp[i];
                break;
            case 'u':
                utoa_buf((unsigned long)va_arg(ap, unsigned int), 10, tmp);
                for (int i = 0; tmp[i] && pos < (int)size - 1; i++)
                    buf[pos++] = tmp[i];
                break;
            case 'x':
                utoa_buf((unsigned long)va_arg(ap, unsigned int), 16, tmp);
                for (int i = 0; tmp[i] && pos < (int)size - 1; i++)
                    buf[pos++] = tmp[i];
                break;
            case 'c':
                buf[pos++] = (char)va_arg(ap, int);
                break;
            case '%':
                buf[pos++] = '%';
                break;
            default:
                buf[pos++] = '%';
                if (pos < (int)size - 1)
                    buf[pos++] = *p;
                break;
        }
    }

    buf[pos] = 0;
    va_end(ap);
    return pos;
}
