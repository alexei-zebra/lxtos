#include <stdint.h>
#include <console/kprint.h>
#include <drivers/framebuffer.h>

void kputs(const char *s)
{
    while (*s)
        fb_putchar_cursor(*s++, COLOR_FG, COLOR_BG);
}

void kputs_col(const char *s, uint32_t fg)
{
    while (*s)
        fb_putchar_cursor(*s++, fg, COLOR_BG);
}

void kputhex(uint64_t val)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[19];
    buf[0]  = '0';
    buf[1]  = 'x';
    buf[18] = '\0';
    for (int i = 17; i >= 2; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    kputs(buf);
}

void kputdec(uint64_t val)
{
    if (val == 0) {
        fb_putchar_cursor('0', COLOR_FG, COLOR_BG);
        return;
    }
    char buf[21];
    int  i = 20;
    buf[20] = '\0';
    while (val > 0 && i > 0) {
        buf[--i] = '0' + (val % 10);
        val      /= 10;
    }
    kputs(buf + i);
}

