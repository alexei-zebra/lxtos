#include <kernel/kapi.h>
#include <console/kprint.h>
#include <lib/kstring.h>
#include <drivers/keyboard.h>
#include <lib/kmalloc.h>
#include <fs/vfs.h>
#include <drivers/framebuffer.h>


static void     _puts   (const char *s)                          { kputs(s); }
static void     _putchar(char c)                                 { fb_putchar_cursor(c, 0xFFFFFF, 0x0D0D1A); }
static void     _puthex (uint64_t v)                             { kputhex(v); }
static void     _putdec (uint64_t v)                             { kputdec(v); }
static char     _getchar(void)                                   { return kb_getchar(); }
static void    *_malloc (uint64_t n)                             { return kmalloc(n); }
static void     _free   (void *p)                                { kfree(p); }
static int      _strlen (const char *s)                          { return kstrlen(s); }
static int      _strcmp (const char *a, const char *b)           { return kstrcmp(a, b); }
static void     _strcpy (char *d, const char *s, int max)        { kstrcpy(d, s, max); }
static void     _memcpy (void *d, const void *s, uint64_t n)    { kmemcpy(d, s, n); }
static void     _memset (void *d, uint8_t v, uint64_t n)        { kmemset(d, v, n); }
static void    *_vfs_open(const char *path)                      { return vfs_resolve(path); }
static int64_t  _vfs_node_read(void *node, void *buf, uint64_t off, uint64_t sz) {
    return vnode_read((vfs_node_t *)node, buf, off, sz);
}

static kernel_api_t kapi = {
    .puts          = _puts,
    .putchar       = _putchar,
    .puthex        = _puthex,
    .putdec        = _putdec,
    .getchar       = _getchar,
    .malloc        = _malloc,
    .free          = _free,
    .strlen        = _strlen,
    .strcmp        = _strcmp,
    .strcpy        = _strcpy,
    .memcpy        = _memcpy,
    .memset        = _memset,
    .vfs_read      = vfs_read,
    .vfs_write     = vfs_write,
    .vfs_open      = _vfs_open,
    .vfs_node_read = _vfs_node_read,
    .version       = 1,
};

kernel_api_t *kapi_get(void)
{
    return &kapi;
}
