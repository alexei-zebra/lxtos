#include <fs/ext2.h>
#include <fs/vfs.h>
#include <drivers/ata.h>
#include <lib/kmalloc.h>
#include <lib/kstring.h>

typedef struct {
    int      bus;
    int      drive;
    uint32_t block_size;
    uint32_t inodes_per_group;
    uint32_t blocks_per_group;
    uint32_t inode_size;
    uint32_t first_data_block;
} ext2_fs_t;

typedef struct {
    ext2_fs_t   *fs;
    uint32_t     ino_num;
    ext2_inode_t inode;
} ext2_node_data_t;

static uint32_t ext2_create_file(ext2_fs_t *fs, uint32_t dir_ino_num, const char *name);

static int ext2_read_block(ext2_fs_t *fs, uint32_t block, uint8_t *buf)
{
    uint32_t sectors_per_block = fs->block_size / 512;
    uint32_t lba = block * sectors_per_block;
    for (uint32_t i = 0; i < sectors_per_block; i++) {
        if (ata_read_sector(fs->bus, fs->drive, lba + i,
                            (uint16_t *)(buf + i * 512)) < 0)
            return -1;
    }
    return 0;
}

static int ext2_write_block(ext2_fs_t *fs, uint32_t block, uint8_t *buf)
{
    /* sectors per one ext2 block */
    uint32_t sectors_per_block = fs->block_size / 512;
    /* LBA of the first sector in this block */
    uint32_t lba = block * sectors_per_block;
    for (uint32_t i = 0; i < sectors_per_block; i++) {
        if (ata_write_sector(fs->bus, fs->drive, lba + i,
                             (uint16_t *)(buf + i * 512)) < 0)
            return -1;
    }
    return 0;
}

static int ext2_read_inode(ext2_fs_t *fs, uint32_t ino, ext2_inode_t *out)
{
    uint32_t group = (ino - 1) / fs->inodes_per_group;
    uint32_t index = (ino - 1) % fs->inodes_per_group;

    uint8_t *bgd_buf = kmalloc(fs->block_size);
    if (!bgd_buf) return -1;
    ext2_read_block(fs, fs->first_data_block + 1, bgd_buf);

    ext2_bgd_t *bgd = (ext2_bgd_t *)bgd_buf + group;
    uint32_t inode_table_block = bgd->bg_inode_table;
    kfree(bgd_buf);

    uint32_t offset    = index * fs->inode_size;
    uint32_t block_off = offset / fs->block_size;
    uint32_t inner_off = offset % fs->block_size;

    uint8_t *inode_buf = kmalloc(fs->block_size);
    if (!inode_buf) return -1;
    ext2_read_block(fs, inode_table_block + block_off, inode_buf);
    kmemcpy(out, inode_buf + inner_off, sizeof(ext2_inode_t));
    kfree(inode_buf);
    return 0;
}

/* @fs    - filesystem
   @ino   - inode number
   @inode - pointer to the struct with new data to write
*/
static int ext2_write_inode(ext2_fs_t *fs, uint32_t ino, ext2_inode_t *inode)
{
    /* index = inode number within the group */
    uint32_t group = (ino - 1) / fs->inodes_per_group;
    uint32_t index = (ino - 1) % fs->inodes_per_group;

    /* read BGD to find the inode table location */
    uint8_t *bgd_buf = kmalloc(fs->block_size);
    if (!bgd_buf) return -1;
    ext2_read_block(fs, fs->first_data_block + 1, bgd_buf);

    ext2_bgd_t *bgd = (ext2_bgd_t *)bgd_buf + group;
    uint32_t inode_table_block = bgd->bg_inode_table;
    kfree(bgd_buf);

    uint32_t offset    = index * fs->inode_size;
    uint32_t block_off = offset / fs->block_size;
    uint32_t inner_off = offset % fs->block_size;

    uint8_t *buf = kmalloc(fs->block_size);
    if (!buf) return -1;

    ext2_read_block(fs, inode_table_block + block_off, buf);
    kmemcpy(buf + inner_off, inode, sizeof(ext2_inode_t));
    ext2_write_block(fs, inode_table_block + block_off, buf);

    kfree(buf);
    return 0;
}

/* returns a free inode number, or 0 if none available */
static uint32_t ext2_alloc_inode(ext2_fs_t *fs)
{
    uint8_t *bgd_buf = kmalloc(fs->block_size);
    if (!bgd_buf) return 0;
    ext2_read_block(fs, fs->first_data_block + 1, bgd_buf);
    ext2_bgd_t *bgd = (ext2_bgd_t *)bgd_buf; /* group 0 */

    uint8_t *bitmap = kmalloc(fs->block_size);
    if (!bitmap) { kfree(bgd_buf); return 0; }
    ext2_read_block(fs, bgd->bg_inode_bitmap, bitmap);

    uint32_t result = 0;

    for (uint32_t byte_i = 0; byte_i < fs->inodes_per_group / 8; byte_i++) {
        if (bitmap[byte_i] == 0xFF) continue; /* all 8 inodes taken */
        for (int bit = 0; bit < 8; bit++) {
            if (!(bitmap[byte_i] & (1 << bit))) {
                /* mark as used */
                bitmap[byte_i] |= (1 << bit);
                ext2_write_block(fs, bgd->bg_inode_bitmap, bitmap);
                bgd->bg_free_inodes_count--;
                ext2_write_block(fs, fs->first_data_block + 1, bgd_buf);
                result = byte_i * 8 + bit + 1; /* inodes are 1-indexed */
                goto done;
            }
        }
    }
done:
    kfree(bitmap);
    kfree(bgd_buf);
    return result; /* 0 if not found */
}

/* returns a free block number, or 0 if disk is full */
static uint32_t ext2_alloc_block(ext2_fs_t *fs)
{
    uint8_t *bgd_buf = kmalloc(fs->block_size);
    if (!bgd_buf) return 0;
    ext2_read_block(fs, fs->first_data_block + 1, bgd_buf);
    ext2_bgd_t *bgd = (ext2_bgd_t *)bgd_buf;

    uint8_t *bitmap = kmalloc(fs->block_size);
    if (!bitmap) { kfree(bgd_buf); return 0; }
    ext2_read_block(fs, bgd->bg_block_bitmap, bitmap);

    uint32_t result = 0;

    for (uint32_t byte_i = 0; byte_i < fs->blocks_per_group / 8; byte_i++) {
        if (bitmap[byte_i] == 0xFF) continue;
        for (int bit = 0; bit < 8; bit++) {
            if (!(bitmap[byte_i] & (1 << bit))) {
                bitmap[byte_i] |= (1 << bit);
                ext2_write_block(fs, bgd->bg_block_bitmap, bitmap);
                bgd->bg_free_blocks_count--;
                ext2_write_block(fs, fs->first_data_block + 1, bgd_buf);
                result = fs->first_data_block + byte_i * 8 + bit;
                goto done;
            }
        }
    }
done:
    kfree(bitmap);
    kfree(bgd_buf);
    return result;
}

static int64_t ext2_read_inode_data(ext2_fs_t *fs, ext2_inode_t *ino,
                                    void *buf, uint64_t offset, uint64_t size)
{
    uint8_t *block_buf = kmalloc(fs->block_size);
    if (!block_buf) return -1;

    uint64_t read = 0;
    uint64_t end  = offset + size;
    if (end > ino->i_size) end = ino->i_size;

    while (offset + read < end) {
        uint64_t pos       = offset + read;
        uint32_t block_idx = pos / fs->block_size;
        uint32_t inner     = pos % fs->block_size;

        if (block_idx >= 12) break; /* indirect blocks not supported */

        uint32_t block_no = ino->i_block[block_idx];
        if (!block_no) break;

        ext2_read_block(fs, block_no, block_buf);

        uint64_t avail = fs->block_size - inner;
        uint64_t want  = end - (offset + read);
        uint64_t chunk = avail < want ? avail : want;

        kmemcpy((uint8_t *)buf + read, block_buf + inner, chunk);
        read += chunk;
    }

    kfree(block_buf);
    return (int64_t)read;
}

static uint32_t ext2_dir_lookup(ext2_fs_t *fs, ext2_inode_t *dir_ino,
                                const char *name)
{
    uint32_t read_size = dir_ino->i_size ? dir_ino->i_size : fs->block_size;
    uint8_t *buf = kmalloc(read_size);
    if (!buf) return 0;

    int64_t got = ext2_read_inode_data(fs, dir_ino, buf, 0, read_size);
    if (got <= 0) { kfree(buf); return 0; }

    uint32_t off     = 0;
    int      namelen = kstrlen(name);
    uint32_t result  = 0;

    while (off + 8 <= (uint32_t)got) {
        ext2_dirent_t *de = (ext2_dirent_t *)(buf + off);
        if (!de->rec_len) break;
        if (de->inode && de->name_len == (uint8_t)namelen &&
            kstrncmp(de->name, name, namelen) == 0) {
            result = de->inode;
            break;
        }
        off += de->rec_len;
    }

    kfree(buf);
    return result;
}

static int ext2_dir_add_entry(ext2_fs_t *fs, ext2_inode_t *dir_inode,
                               uint32_t child_ino, const char *name,
                               uint8_t file_type)
{
    uint8_t  namelen = (uint8_t)kstrlen(name);
    uint16_t needed  = (8 + namelen + 3) & ~3; /* size aligned to 4 bytes */

    uint32_t block_no = dir_inode->i_block[0];
    uint8_t *buf = kmalloc(fs->block_size);
    if (!buf) return -1;
    ext2_read_block(fs, block_no, buf);

    uint32_t off = 0;

    while (off + 8 <= fs->block_size) {
        ext2_dirent_t *de = (ext2_dirent_t *)(buf + off);

        if (!de->rec_len) break; /* end of entries */

        /* actual size of this entry vs allocated size */
        uint16_t real_size  = (8 + de->name_len + 3) & ~3;
        uint16_t free_space = de->rec_len - real_size;

        if (free_space >= needed) {
            uint16_t old_rec_len = de->rec_len;
            de->rec_len = real_size; /* shrink current entry */

            /* place new entry in the freed space */
            ext2_dirent_t *ne = (ext2_dirent_t *)(buf + off + real_size);
            ne->inode     = child_ino;
            ne->rec_len   = old_rec_len - real_size;
            ne->name_len  = namelen;
            ne->file_type = file_type;
            kmemcpy(ne->name, name, namelen);

            ext2_write_block(fs, block_no, buf);
            kfree(buf);
            return 0;
        }

        off += de->rec_len;
    }

    kfree(buf);
    return -1; /* no space in directory block */
}

static uint32_t ext2_create_file(ext2_fs_t *fs, uint32_t dir_ino_num, const char *name)
{
    uint32_t new_ino = ext2_alloc_inode(fs);
    if (!new_ino) return 0; /* no free inodes */

    ext2_inode_t new_inode;
    kmemset(&new_inode, 0, sizeof(new_inode));
    new_inode.i_mode        = 0x81A4; /* regular file, rw-r--r-- */
    new_inode.i_links_count = 1;
    new_inode.i_size        = 0;

    ext2_write_inode(fs, new_ino, &new_inode);

    ext2_inode_t dir_inode;
    ext2_read_inode(fs, dir_ino_num, &dir_inode);

    if (ext2_dir_add_entry(fs, &dir_inode, new_ino, name, 1) < 0)
        return 0;

    return new_ino;
}

int ext2_write_file(ext2_fs_t *fs, uint32_t ino_num,
                    const void *data, uint32_t size)
{
    ext2_inode_t inode;
    ext2_read_inode(fs, ino_num, &inode);

    /* allocate a data block if the file has none */
    if (!inode.i_block[0]) {
        uint32_t new_block = ext2_alloc_block(fs);
        if (!new_block) return -1; /* disk full */
        inode.i_block[0] = new_block;
    }

    uint8_t *buf = kmalloc(fs->block_size);
    if (!buf) return -1;

    kmemset(buf, 0, fs->block_size);
    kmemcpy(buf, data, size);
    ext2_write_block(fs, inode.i_block[0], buf);
    kfree(buf);

    inode.i_size = size;
    ext2_write_inode(fs, ino_num, &inode);

    return 0;
}

static vfs_node_t *ext2_make_vnode(ext2_fs_t *fs, uint32_t ino_num);

static int64_t ext2_vfs_read(vfs_node_t *node, void *buf,
                             uint64_t offset, uint64_t size)
{
    ext2_node_data_t *d = node->fs_data;
    return ext2_read_inode_data(d->fs, &d->inode, buf, offset, size);
}

static int64_t ext2_vfs_write(vfs_node_t *node, const void *buf,
                              uint64_t offset, uint64_t size)
{
    ext2_node_data_t *d = node->fs_data;
    (void)offset;
    if (ext2_write_file(d->fs, d->ino_num, buf, (uint32_t)size) < 0)
        return -1;
    d->inode.i_size = size;
    node->size = size;
    return (int64_t)size;
}

static vfs_node_t *ext2_vfs_mkdir(vfs_node_t *parent, const char *name)
{
    ext2_node_data_t *d = parent->fs_data;

    uint32_t new_ino = ext2_alloc_inode(d->fs);
    if (!new_ino) return NULL;

    ext2_inode_t new_inode;
    kmemset(&new_inode, 0, sizeof(new_inode));
    new_inode.i_mode        = 0x41ED; /* directory, rwxr-xr-x */
    new_inode.i_links_count = 2;
    new_inode.i_size        = d->fs->block_size;

    /* allocate a data block for the directory entries */
    uint32_t blk = ext2_alloc_block(d->fs);
    if (!blk) return NULL;
    new_inode.i_block[0] = blk;

    /* write empty directory block: . and .. entries */
    uint8_t *buf = kmalloc(d->fs->block_size);
    if (!buf) return NULL;
    kmemset(buf, 0, d->fs->block_size);

    /* . entry */
    ext2_dirent_t *dot = (ext2_dirent_t *)buf;
    dot->inode     = new_ino;
    dot->rec_len   = 12;
    dot->name_len  = 1;
    dot->file_type = 2;
    dot->name[0]   = '.';

    /* .. entry */
    ext2_dirent_t *dotdot = (ext2_dirent_t *)(buf + 12);
    dotdot->inode     = d->ino_num;
    dotdot->rec_len   = d->fs->block_size - 12;
    dotdot->name_len  = 2;
    dotdot->file_type = 2;
    dotdot->name[0]   = '.';
    dotdot->name[1]   = '.';

    ext2_write_block(d->fs, blk, buf);
    kfree(buf);

    ext2_write_inode(d->fs, new_ino, &new_inode);

    ext2_inode_t dir_inode;
    ext2_read_inode(d->fs, d->ino_num, &dir_inode);
    ext2_dir_add_entry(d->fs, &dir_inode, new_ino, name, 2);

    return ext2_make_vnode(d->fs, new_ino);
}


static vfs_node_t *ext2_vfs_finddir(vfs_node_t *node, const char *name)
{
    ext2_node_data_t *d = node->fs_data;
    uint32_t child_ino = ext2_dir_lookup(d->fs, &d->inode, name);
    if (!child_ino) return NULL;
    vfs_node_t *child = ext2_make_vnode(d->fs, child_ino);
    if (!child) return NULL;
    kstrcpy(child->name, name, VFS_MAX_NAME);
    return child;
}

static vfs_node_t *ext2_vfs_readdir(vfs_node_t *node, uint32_t index)
{
    ext2_node_data_t *d = node->fs_data;

    uint32_t read_size = d->inode.i_size ? d->inode.i_size : d->fs->block_size;
    uint8_t *buf = kmalloc(read_size);
    if (!buf) return NULL;

    int64_t got = ext2_read_inode_data(d->fs, &d->inode, buf, 0, read_size);
    if (got <= 0) { kfree(buf); return NULL; }

    uint32_t off   = 0;
    uint32_t cur   = 0;
    vfs_node_t *result = NULL;

    while (off + 8 <= (uint32_t)got) {
        ext2_dirent_t *de = (ext2_dirent_t *)(buf + off);
        if (!de->rec_len) break;
        if (de->inode && de->name_len > 0) {
            if (cur == index) {
                result = ext2_make_vnode(d->fs, de->inode);
                if (result) {
                    int nl = de->name_len < VFS_MAX_NAME - 1
                           ? de->name_len : VFS_MAX_NAME - 1;
                    for (int i = 0; i < nl; i++)
                        result->name[i] = de->name[i];
                    result->name[nl] = '\0';
                }
                break;
            }
            cur++;
        }
        off += de->rec_len;
    }

    kfree(buf);
    return result;
}

static vfs_node_t *ext2_vfs_mkfile(vfs_node_t *parent, const char *name)
{
    ext2_node_data_t *d = parent->fs_data;
    uint32_t new_ino = ext2_create_file(d->fs, d->ino_num, name);
    if (!new_ino) return NULL;
    return ext2_make_vnode(d->fs, new_ino);
}

static vfs_ops_t ext2_file_ops = {
    .read  = ext2_vfs_read,
    .write = ext2_vfs_write,
};

static vfs_ops_t ext2_dir_ops = {
    .finddir = ext2_vfs_finddir,
    .readdir = ext2_vfs_readdir,
    .mkfile  = ext2_vfs_mkfile,
    .mkdir   = ext2_vfs_mkdir,
};

static vfs_node_t *ext2_make_vnode(ext2_fs_t *fs, uint32_t ino_num)
{
    ext2_node_data_t *d = kzalloc(sizeof(ext2_node_data_t));
    vfs_node_t       *n = kzalloc(sizeof(vfs_node_t));
    if (!d || !n) return NULL;

    d->fs      = fs;
    d->ino_num = ino_num;
    ext2_read_inode(fs, ino_num, &d->inode);

    n->fs_data = d;
    n->size    = d->inode.i_size;

    int is_dir = (d->inode.i_mode & 0xF000) == 0x4000;
    n->flags   = is_dir ? VFS_FLAG_DIR : VFS_FLAG_FILE;
    n->ops     = is_dir ? &ext2_dir_ops : &ext2_file_ops;
    return n;
}

vfs_node_t *ext2_mount(int bus, int drive)
{
    uint8_t sb_buf[1024];
    ata_read_sector(bus, drive, 2, (uint16_t *)sb_buf);
    ata_read_sector(bus, drive, 3, (uint16_t *)(sb_buf + 512));

    ext2_superblock_t *sb = (ext2_superblock_t *)sb_buf;
    if (sb->s_magic != EXT2_SUPER_MAGIC) return NULL;

    ext2_fs_t *fs = kzalloc(sizeof(ext2_fs_t));
    fs->bus              = bus;
    fs->drive            = drive;
    fs->block_size       = 1024u << sb->s_log_block_size;
    fs->inodes_per_group = sb->s_inodes_per_group;
    fs->blocks_per_group = sb->s_blocks_per_group;
    fs->inode_size       = (sb->s_rev_level >= 1) ? sb->s_inode_size : 128;
    fs->first_data_block = sb->s_first_data_block;

    return ext2_make_vnode(fs, EXT2_ROOT_INO);
}
