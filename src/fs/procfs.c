#include <fs/procfs.h>
#include <lib/kmalloc.h>



#define PROCFS_MAX_ENTRIES 32

static int kstrlen(const char *s) { int n=0; while(s[n]) n++; return n; }
static int kstrcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}
static void kstrcpy(char *d, const char *s, int max) {
    int i=0; while(s[i] && i<max-1){d[i]=s[i];i++;} d[i]=0;
}

typedef struct {
    procfs_read_fn fn;
} procfs_file_data_t;

typedef struct {
    vfs_node_t *entries[PROCFS_MAX_ENTRIES];
    int         count;
} procfs_dir_data_t;

static vfs_node_t      *proc_root = NULL;
static procfs_dir_data_t proc_dir_data;

static int64_t procfs_read(vfs_node_t *node, void *buf, uint64_t offset, uint64_t size)
{
    procfs_file_data_t *d = (procfs_file_data_t *)node->fs_data;
    if (!d->fn) return 0;

    
    uint8_t tmp[4096];
    int64_t total = d->fn(tmp, sizeof(tmp));
    if (total <= 0) return 0;
    if (offset >= (uint64_t)total) return 0;

    uint64_t avail   = (uint64_t)total - offset;
    uint64_t to_read = size < avail ? size : avail;
    uint8_t *src = tmp + offset;
    uint8_t *dst = (uint8_t *)buf;
    for (uint64_t i = 0; i < to_read; i++) dst[i] = src[i];
    return (int64_t)to_read;
}

static vfs_node_t *procfs_finddir(vfs_node_t *node, const char *name)
{
    (void)node;
    for (int i = 0; i < proc_dir_data.count; i++) {
        if (kstrcmp(proc_dir_data.entries[i]->name, name) == 0)
            return proc_dir_data.entries[i];
    }
    return NULL;
}

static vfs_node_t *procfs_readdir_fn(vfs_node_t *node, uint32_t index)
{
    (void)node;
    if ((int)index >= proc_dir_data.count) return NULL;
    return proc_dir_data.entries[index];
}

static vfs_ops_t procfs_dir_ops = {
    .finddir = procfs_finddir,
    .readdir = procfs_readdir_fn,
};

static vfs_ops_t procfs_file_ops = {
    .read = procfs_read,
};

vfs_node_t *procfs_create(void)
{
    proc_root = kzalloc(sizeof(vfs_node_t));
    if (!proc_root) return NULL;
    kstrcpy(proc_root->name, "proc", VFS_MAX_NAME);
    proc_root->flags   = VFS_FLAG_DIR | VFS_FLAG_MOUNTPT;
    proc_root->fs_data = &proc_dir_data;
    proc_root->ops     = &procfs_dir_ops;
    proc_dir_data.count = 0;
    return proc_root;
}

vfs_node_t *procfs_register(const char *name, procfs_read_fn fn)
{
    if (!proc_root) return NULL;
    if (proc_dir_data.count >= PROCFS_MAX_ENTRIES) return NULL;

    vfs_node_t         *node = kzalloc(sizeof(vfs_node_t));
    procfs_file_data_t *data = kzalloc(sizeof(procfs_file_data_t));
    if (!node || !data) return NULL;

    kstrcpy(node->name, name, VFS_MAX_NAME);
    node->flags   = VFS_FLAG_FILE;
    node->ops     = &procfs_file_ops;
    node->fs_data = data;
    node->parent  = proc_root;
    data->fn      = fn;

    proc_dir_data.entries[proc_dir_data.count++] = node;
    return node;
}
