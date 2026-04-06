#include <drivers/framebuffer.h>
#include <console/font.h>
#include <stdint.h>

static void    *fb_addr;
uint64_t        fb_width;
uint64_t        fb_height;
static uint64_t fb_pitch;
static uint16_t fb_bpp;
static uint32_t fb_bytes_per_pixel;

uint32_t fb_cursor_x = 0;
uint32_t fb_cursor_y = 0;

void fb_init(void *addr, uint64_t width, uint64_t height, uint64_t pitch, uint16_t bpp)
{
    fb_addr            = addr;
    fb_width           = width;
    fb_height          = height;
    fb_pitch           = pitch;
    fb_bpp             = bpp;
    fb_bytes_per_pixel = bpp / 8;
}

void fb_putpixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (x >= fb_width || y >= fb_height) return;
    uint8_t *pixel = (uint8_t *)fb_addr + y * fb_pitch + x * fb_bytes_per_pixel;
    pixel[0] = (color >>  0) & 0xFF;
    pixel[1] = (color >>  8) & 0xFF;
    pixel[2] = (color >> 16) & 0xFF;
    if (fb_bytes_per_pixel == 4)
        pixel[3] = 0xFF;
}

void fb_fill(uint32_t color)
{
    for (uint64_t y = 0; y < fb_height; y++)
        for (uint64_t x = 0; x < fb_width; x++)
            fb_putpixel(x, y, color);
}

void fb_putchar(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg)
{
    if ((unsigned char)c >= 128) c = '?';
    char *glyph = font8x8_basic[(unsigned char)c];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int bit = (glyph[row] >> (7 - col)) & 1;
            fb_putpixel(x + col, y + row, bit ? fg : bg);
        }
    }
}

void fb_puts(uint32_t x, uint32_t y, const char *s, uint32_t fg, uint32_t bg)
{
    uint32_t cx = x;
    while (*s) {
        if (*s == '\n') {
            cx  = x;
            y  += 10;
        } else {
            fb_putchar(cx, y, *s, fg, bg);
            cx += 8;
        }
        s++;
    }
}

void fb_scroll(uint32_t bg)
{
    uint8_t *dst = (uint8_t *)fb_addr;
    uint8_t *src = (uint8_t *)fb_addr + 10 * fb_pitch;
    uint64_t rows_to_move = fb_height - 10;

    for (uint64_t row = 0; row < rows_to_move; row++) {
        uint8_t *d = dst + row * fb_pitch;
        uint8_t *s = src + row * fb_pitch;
        for (uint64_t col = 0; col < fb_width * fb_bytes_per_pixel; col++)
            d[col] = s[col];
    }

    for (uint64_t row = rows_to_move; row < fb_height; row++) {
        uint8_t *p = dst + row * fb_pitch;
        for (uint64_t col = 0; col < fb_width; col++) {
            p[col * fb_bytes_per_pixel + 0] = (bg >>  0) & 0xFF;
            p[col * fb_bytes_per_pixel + 1] = (bg >>  8) & 0xFF;
            p[col * fb_bytes_per_pixel + 2] = (bg >> 16) & 0xFF;
            if (fb_bytes_per_pixel == 4)
                p[col * fb_bytes_per_pixel + 3] = 0xFF;
        }
    }
}

void fb_newline(uint32_t bg)
{
    fb_cursor_x = 0;
    fb_cursor_y += 10;
    if (fb_cursor_y + 10 > fb_height) {
        fb_scroll(bg);
        fb_cursor_y -= 10;
    }
}

void fb_putchar_cursor(char c, uint32_t fg, uint32_t bg)
{
    if (c == '\n') {
        fb_newline(bg);
        return;
    }
    if (c == '\b') {
        if (fb_cursor_x >= 8) {
            fb_cursor_x -= 8;
            fb_putchar(fb_cursor_x, fb_cursor_y, ' ', fg, bg);
        }
        return;
    }
    fb_putchar(fb_cursor_x, fb_cursor_y, c, fg, bg);
    fb_cursor_x += 8;
    if (fb_cursor_x + 8 > (uint32_t)fb_width)
        fb_newline(bg);
}
