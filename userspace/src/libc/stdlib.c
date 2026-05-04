#include <libc/stdlib.h>
#include <ulib/syscall.h>


int atoi(const char *s) {
    int n = 0, neg = 0;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    while (*s >= '0' && *s <= '9')
            n = n * 10 + (*s++ - '0');
    return neg ? -n : n;
}

void itoa(int n, char *buf, int base) {
    static const char digits[] = "0123456789abcdef";
    char tmp[32];
    int i = 0, neg = 0;
    if (n < 0 && base == 10) {
        neg = 1;
        n = -n;
    }
    if (n == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    while (n) {
        tmp[i++] = digits[n % base];
        n /= base;
    }
    if (neg)
            tmp[i++] = '-';
    int j = 0;
    while (i--)
            buf[j++] = tmp[i];
    buf[j] = 0;
}

void exit(int code) {
    (void)code;
    sys_exit();
}
