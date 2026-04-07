#include <drivers/keyboard.h>
#include <stdint.h>
#include <arch/x86_64/io.h>


#define KB_DATA    0x60
#define KB_STATUS  0x64
#define KB_CMD     0x64


static const char scancode_map[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q',  'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,   ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char scancode_map_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,   ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0
};

#define SC_LSHIFT   0x2A
#define SC_RSHIFT   0x36
#define SC_LSHIFT_R 0xAA
#define SC_RSHIFT_R 0xB6

static int shift_held = 0;

void kb_init(void)
{
    while (inb(KB_STATUS) & 0x01)
        inb(KB_DATA);
}

int kb_has_key(void)
{
    return inb(KB_STATUS) & 0x01;
}

char kb_getchar(void)
{
    char c = 0;
    while (!c) {
        while (!kb_has_key());
        uint8_t sc = inb(KB_DATA);

        if (sc == SC_LSHIFT || sc == SC_RSHIFT) { shift_held = 1; continue; }
        if (sc == SC_LSHIFT_R || sc == SC_RSHIFT_R) { shift_held = 0; continue; }
        if (sc & 0x80) continue;
        if (sc >= 128) continue;

        c = shift_held ? scancode_map_shift[sc] : scancode_map[sc];
    }
    return c;
}