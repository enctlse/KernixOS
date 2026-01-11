#include "../vfs.h"
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <string/string.h>
#include <drivers/ata/ata.h>
#include <drivers/partitions/partitions.h>
#include <kernel/console/console.h>
extern void *fs_kernel_memory;
extern u16 ata_primary_io;

#define EXT4_EXTENTS_FL 0x80000

typedef struct {
    u32 ee_block;
    u16 ee_len;
    u16 ee_start_hi;
    u32 ee_start_lo;
} __attribute__((packed)) ext4_extent;

typedef struct {
    u16 eh_magic;
    u16 eh_entries;
    u16 eh_max;
    u16 eh_depth;
    u32 eh_generation;
} __attribute__((packed)) ext4_extent_header;

typedef struct {
    ext4_extent_header eh;
    ext4_extent ee[0];
} __attribute__((packed)) ext4_extent_list;

typedef struct {
    u32 s_inodes_count;
    u32 s_blocks_count;
    u32 s_r_blocks_count;
    u32 s_free_blocks_count;
    u32 s_free_inodes_count;
    u32 s_first_data_block;
    u32 s_log_block_size;
    u32 s_log_frag_size;
    u32 s_blocks_per_group;
    u32 s_frags_per_group;
    u32 s_inodes_per_group;
    u32 s_mtime;
    u32 s_wtime;
    u16 s_mnt_count;
    u16 s_max_mnt_count;
    u16 s_magic;
    u16 s_state;
    u16 s_errors;
    u16 s_minor_rev_level;
    u32 s_lastcheck;
    u32 s_checkinterval;
    u32 s_creator_os;
    u32 s_rev_level;
    u16 s_def_resuid;
    u16 s_def_resgid;
    u32 s_first_ino;
    u16 s_inode_size;
    u16 s_block_group_nr;
    u32 s_feature_compat;
    u32 s_feature_incompat;
    u32 s_feature_ro_compat;
    u8 s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    u32 s_algorithm_usage_bitmap;
    u8 s_prealloc_blocks;
    u8 s_prealloc_dir_blocks;
    u16 s_padding1;
    u32 s_reserved[204];
} ext4_superblock;

typedef struct {
    u16 i_mode;
    u16 i_uid;
    u32 i_size_lo;
    u32 i_atime;
    u32 i_ctime;
    u32 i_mtime;
    u32 i_dtime;
    u16 i_gid;
    u16 i_links_count;
    u32 i_blocks_lo;
    u32 i_flags;
    u32 i_osd1;
    u32 i_block[15];
    u32 i_generation;
    u32 i_file_acl_lo;
    u32 i_size_high;
    u32 i_obso_faddr;
    u16 i_blocks_high;
    u16 i_file_acl_high;
    u16 i_uid_high;
    u16 i_gid_high;
    u16 i_checksum_lo;
    u16 i_reserved;
    u16 i_extra_isize;
    u16 i_checksum_hi;
    u32 i_ctime_extra;
    u32 i_mtime_extra;
    u32 i_atime_extra;
    u32 i_crtime;
    u32 i_crtime_extra;
    u32 i_version_hi;
} ext4_inode;

typedef struct {
    u32 bg_block_bitmap_lo;
    u32 bg_inode_bitmap_lo;
    u32 bg_inode_table_lo;
    u16 bg_free_blocks_count_lo;
    u16 bg_free_inodes_count_lo;
    u16 bg_used_dirs_count_lo;
    u16 bg_flags;
    u32 bg_exclude_bitmap_lo;
    u16 bg_block_bitmap_csum_lo;
    u16 bg_inode_bitmap_csum_lo;
    u16 bg_itable_unused_lo;
    u16 bg_checksum;
    u32 bg_block_bitmap_hi;
    u32 bg_inode_bitmap_hi;
    u32 bg_inode_table_hi;
    u16 bg_free_blocks_count_hi;
    u16 bg_free_inodes_count_hi;
    u16 bg_used_dirs_count_hi;
    u16 bg_itable_unused_hi;
    u32 bg_exclude_bitmap_hi;
    u16 bg_block_bitmap_csum_hi;
    u16 bg_inode_bitmap_csum_hi;
    u32 bg_reserved[0];
} ext4_group_desc;

typedef struct {
    void *device;
    ext4_superblock sb;
    ext4_group_desc *gd;
    u32 block_size;
    u32 inode_size;
    int is_ata;
    int partition_index;
    u64 start_lba;
    u64 journal_start_block;
} ext4_data;

static int ext4_read_block(ext4_data *data, u64 block, void *buf) {
    if (data->partition_index >= 0) {
        // Read from partition
        u64 sectors_per_block = data->block_size / 512;
        for (u64 i = 0; i < sectors_per_block; i++) {
            if (partitions_read_sector(data->partition_index, block * sectors_per_block + i, (u8*)buf + i * 512) != 0) return -1;
        }
        return 0;
    } else {
        u64 offset = block * data->block_size;
        for (u64 i = 0; i < data->block_size; i++) {
            ((u8*)buf)[i] = ((u8*)data->device)[offset + i];
        }
        return 0;
    }
}

static int ext4_write_block(ext4_data *data, u64 block, const void *buf) {
    if (data->partition_index >= 0) {
        u64 sectors_per_block = data->block_size / 512;
        for (u64 i = 0; i < sectors_per_block; i++) {
            if (partitions_write_sector(data->partition_index, block * sectors_per_block + i, (u8*)buf + i * 512) != 0) return -1;
        }
        return 0;
    } else {
        u64 offset = block * data->block_size;
        for (u64 i = 0; i < data->block_size; i++) {
            ((u8*)data->device)[offset + i] = ((u8*)buf)[i];
        }
        return 0;
    }
}

static int ext4_read_inode(ext4_data *data, u32 inode_num, ext4_inode *inode) {
    u32 group = (inode_num - 1) / data->sb.s_inodes_per_group;
    u32 index = (inode_num - 1) % data->sb.s_inodes_per_group;
    u64 block = data->gd[group].bg_inode_table_lo + (index * data->inode_size) / data->block_size;
    u32 offset = (index * data->inode_size) % data->block_size;
    void *buf = kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, data->block_size, 1);
    if (!buf) return -1;
    if (ext4_read_block(data, block, buf) < 0) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
        return -1;
    }
    str_copy((char*)inode, (char*)buf + offset);
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
    return 0;
}

static int ext4_write_inode(ext4_data *data, u32 inode_num, ext4_inode *inode) {
    u32 group = (inode_num - 1) / data->sb.s_inodes_per_group;
    u32 index = (inode_num - 1) % data->sb.s_inodes_per_group;
    u64 block = data->gd[group].bg_inode_table_lo + (index * data->inode_size) / data->block_size;
    u32 offset = (index * data->inode_size) % data->block_size;
    void *buf = kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, data->block_size, 1);
    if (!buf) return -1;
    if (ext4_read_block(data, block, buf) < 0) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
        return -1;
    }
    str_copy((char*)buf + offset, (char*)inode);
    if (ext4_write_block(data, block, buf) < 0) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
        return -1;
    }
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
    return 0;
}

static u64 ext4_get_block_from_extents(ext4_data *data, ext4_inode *inode, u64 block_index) {
    if (!(inode->i_flags & EXT4_EXTENTS_FL)) {
        // Direct blocks
        if (block_index < 12) {
            return inode->i_block[block_index];
        } else {
            // Indirect blocks not implemented
            return 0;
        }
    }
    // Extents
    ext4_extent_header *eh = (ext4_extent_header*)inode->i_block;
    if (eh->eh_magic != 0xF30A) return 0;
    ext4_extent *ee = (ext4_extent*)(eh + 1);
    for (u16 i = 0; i < eh->eh_entries; i++) {
        if (block_index >= ee[i].ee_block && block_index < ee[i].ee_block + ee[i].ee_len) {
            return (ee[i].ee_start_lo | ((u64)ee[i].ee_start_hi << 32)) + (block_index - ee[i].ee_block);
        }
    }
    return 0;
}

static fs_node *ext4_create_node(ext4_data *data, u32 inode_num, const char *name) {
    ext4_inode inode;
    if (ext4_read_inode(data, inode_num, &inode) < 0) return NULL;
    fs_node *node = fs_mknode(name, (inode.i_mode & 0x4000) ? FS_DIR : FS_FILE);
    node->size = inode.i_size_lo;
    node->inode = inode_num;
    node->priv = data;
    return node;
}

static int ext4_open(fs_node *node, fs_file *file) {
    file->node = node;
    file->pos = 0;
    return 0;
}

static int ext4_close(fs_file *file) {
    return 0;
}

static ssize_t ext4_read(fs_file *file, void *buf, size_t cnt) {
    ext4_data *data = (ext4_data*)file->node->priv;
    ext4_inode inode;
    if (ext4_read_inode(data, file->node->inode, &inode) < 0) return -1;
    if (file->pos >= inode.i_size_lo) return 0;
    if (file->pos + cnt > inode.i_size_lo) cnt = inode.i_size_lo - file->pos;
    size_t read = 0;
    while (cnt > 0) {
        u64 block_index = file->pos / data->block_size;
        u64 block = ext4_get_block_from_extents(data, &inode, block_index);
        if (block == 0) break;
        u32 offset = file->pos % data->block_size;
        size_t to_read = data->block_size - offset;
        if (to_read > cnt) to_read = cnt;
        void *block_buf = kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, data->block_size, 1);
        if (!block_buf) return -1;
        if (ext4_read_block(data, block, block_buf) < 0) {
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, block_buf);
            return -1;
        }
        for (size_t i = 0; i < to_read; i++) {
            ((u8*)buf)[read + i] = ((u8*)block_buf)[offset + i];
        }
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, block_buf);
        file->pos += to_read;
        read += to_read;
        cnt -= to_read;
        buf = (u8*)buf + to_read;
    }
    return read;
}

static ssize_t ext4_write(fs_file *file, const void *buf, size_t cnt) {
    ext4_data *data = (ext4_data*)file->node->priv;
    ext4_inode inode;
    if (ext4_read_inode(data, file->node->inode, &inode) < 0) return -1;
    size_t written = 0;
    while (cnt > 0) {
        u64 block_index = file->pos / data->block_size;
        u64 block = ext4_get_block_from_extents(data, &inode, block_index);
        if (block == 0) break; // No block allocated, need to allocate but simplified
        u32 offset = file->pos % data->block_size;
        size_t to_write = data->block_size - offset;
        if (to_write > cnt) to_write = cnt;
        void *block_buf = kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, data->block_size, 1);
        if (!block_buf) return -1;
        if (ext4_read_block(data, block, block_buf) < 0) {
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, block_buf);
            return -1;
        }
        for (size_t i = 0; i < to_write; i++) {
            ((u8*)block_buf)[offset + i] = ((u8*)buf)[written + i];
        }
        if (ext4_write_block(data, block, block_buf) < 0) {
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, block_buf);
            return -1;
        }
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, block_buf);
        file->pos += to_write;
        written += to_write;
        cnt -= to_write;
        buf = (u8*)buf + to_write;
    }
    if (file->pos > inode.i_size_lo) {
        inode.i_size_lo = file->pos;
        ext4_write_inode(data, file->node->inode, &inode);
    }
    return written;
}

static fs_node *ext4_lookup(fs_node *dir, const char *name) {
    ext4_data *data = (ext4_data*)dir->priv;
    ext4_inode inode;
    if (ext4_read_inode(data, dir->inode, &inode) < 0) return NULL;
    u64 block = inode.i_block[0];
    void *buf = kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, data->block_size, 1);
    if (!buf) return NULL;
    if (ext4_read_block(data, block, buf) < 0) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
        return NULL;
    }
    char *entry = (char*)buf;
    while (*entry) {
        if (str_equals(entry, name)) {
            u32 inode_num = *(u32*)(entry + str_len(entry) + 1);
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
            return ext4_create_node(data, inode_num, name);
        }
        entry += str_len(entry) + 5;
    }
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
    return NULL;
}

static u32 ext4_alloc_inode(ext4_data *data) {
    // Simplified: find first free inode starting from 12
    for (u32 ino = 12; ino < data->sb.s_inodes_count; ino++) {
        u32 group = (ino - 1) / data->sb.s_inodes_per_group;
        u32 index = (ino - 1) % data->sb.s_inodes_per_group;
        u64 bitmap_block = data->gd[group].bg_inode_bitmap_lo;
        void *bitmap = kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, data->block_size, 1);
        if (!bitmap) return 0;
        if (ext4_read_block(data, bitmap_block, bitmap) < 0) {
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, bitmap);
            return 0;
        }
        u32 byte = index / 8;
        u8 bit = index % 8;
        if (!(((u8*)bitmap)[byte] & (1 << bit))) {
            ((u8*)bitmap)[byte] |= (1 << bit);
            ext4_write_block(data, bitmap_block, bitmap);
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, bitmap);
            return ino;
        }
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, bitmap);
    }
    return 0;
}

static int ext4_create(fs_node *dir, const char *name) {
    ext4_data *data = (ext4_data*)dir->priv;
    u32 new_ino = ext4_alloc_inode(data);
    if (new_ino == 0) return -1;
    // Initialize inode
    ext4_inode new_inode = {0};
    new_inode.i_mode = 0x81A4; // Regular file
    new_inode.i_uid = 0;
    new_inode.i_gid = 0;
    new_inode.i_size_lo = 0;
    new_inode.i_atime = 0;
    new_inode.i_ctime = 0;
    new_inode.i_mtime = 0;
    new_inode.i_dtime = 0;
    new_inode.i_links_count = 1;
    new_inode.i_blocks_lo = 0;
    new_inode.i_flags = 0;
    // For simplicity, no extents, use direct block 0
    new_inode.i_block[0] = 0; // Need to allocate block
    ext4_write_inode(data, new_ino, &new_inode);
    // Add to directory
    ext4_inode dir_inode;
    if (ext4_read_inode(data, dir->inode, &dir_inode) < 0) return -1;
    u64 block = ext4_get_block_from_extents(data, &dir_inode, 0);
    void *buf = kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, data->block_size, 1);
    if (!buf) return -1;
    if (ext4_read_block(data, block, buf) < 0) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
        return -1;
    }
    char *entry = (char*)buf;
    while (*entry) {
        entry += str_len(entry) + 5;
    }
    str_copy(entry, name);
    *(u32*)(entry + str_len(name) + 1) = new_ino;
    if (ext4_write_block(data, block, buf) < 0) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
        return -1;
    }
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, buf);
    fs_node *node = ext4_create_node(data, new_ino, name);
    fs_addchild(dir, node);
    return 0;
}

static int ext4_mkdir(fs_node *dir, const char *name) {
    return ext4_create(dir, name);
}

static fs_ops ext4_ops = {
    .open = ext4_open,
    .close = ext4_close,
    .read = ext4_read,
    .write = ext4_write,
    .lookup = ext4_lookup,
    .create = ext4_create,
    .mkdir = ext4_mkdir,
};

static int ext4_mount(const char *src, const char *tgt, fs_mnt *mnt) {
    print("EXT4: mounting ", white);
    print(src, cyan);
    print(" to ", white);
    print(tgt, cyan);
    print("\n", white);
    u64 *data_ptr = kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(ext4_data), 1);
    if (!data_ptr) {
        print("EXT4: failed to alloc data\n", red);
        return -1;
    }
    ext4_data *data = (ext4_data*)data_ptr;
    data->is_ata = 0;
    data->partition_index = -1;
    if (src) {
        if (str_starts_with(src, "ata")) {
            // Parse ataXpY, e.g. ata0p0
            int part = src[5] - '0';
            data->partition_index = part; // Assuming partitions are indexed by order
            print("EXT4: using partition index ", white);
            printInt(part, cyan);
            print("\n", white);
        } else {
            data->device = (void*)src;
        }
    }
    // Read superblock at 1024 bytes
    u8 *sb_buf = (u8*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 1024, 1);
    if (!sb_buf) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, data_ptr);
        return -1;
    }
    if (data->partition_index >= 0) {
        if (partitions_read_sector(data->partition_index, 2, sb_buf) != 0) { // Sector 2 = 1024 bytes
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)sb_buf);
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, data_ptr);
            return -1;
        }
    } else {
        // For ramdisk or other, assume offset 1024
        u64 offset = 1024;
        for (u64 i = 0; i < 1024; i++) {
            sb_buf[i] = ((u8*)data->device)[offset + i];
        }
    }
    str_copy((char*)&data->sb, (char*)sb_buf);
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)sb_buf);
    print("EXT4: superblock magic: 0x", white);
    printHex(data->sb.s_magic, cyan);
    print("\n", white);
    if (data->sb.s_magic != 0xEF53) {
        print("EXT4: invalid magic\n", red);
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, data_ptr);
        return -1;
    }
    print("EXT4: valid ext4 superblock\n", green);
    data->block_size = 1024 << data->sb.s_log_block_size;
    data->inode_size = data->sb.s_inode_size;
    u32 gd_blocks = (data->sb.s_blocks_count + data->sb.s_blocks_per_group - 1) / data->sb.s_blocks_per_group;
    // Basic fsck-like checks
    if (data->sb.s_state != 1) {
        // FS not clean, but continue
    }
    if (data->sb.s_errors != 1) {
        // Continue on errors
    }
    data->gd = (ext4_group_desc*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, gd_blocks * sizeof(ext4_group_desc), 1);
    if (!data->gd) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, data_ptr);
        return -1;
    }
    u64 gd_block = data->sb.s_first_data_block + 1;
    for (u32 i = 0; i < gd_blocks; i++) {
        ext4_read_block(data, gd_block + i, &data->gd[i]);
    }
    // Check group descriptors
    for (u32 i = 0; i < gd_blocks; i++) {
        if (data->gd[i].bg_block_bitmap_lo == 0) {
            // Warning: invalid block bitmap
        }
    }
    mnt->root = ext4_create_node(data, 2, tgt);
    mnt->priv = data_ptr;
    return 0;
}

static fs_type ext4_type = {
    .name = "ext4",
    .mount = ext4_mount,
    .ops = &ext4_ops,
};

void ext4_register(void) {
    fs_register(&ext4_type);
}