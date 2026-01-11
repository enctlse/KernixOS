#ifndef PARTITIONS_H
#define PARTITIONS_H
#include <outputs/types.h>
#define MBR_SIGNATURE 0xAA55
typedef struct {
    u8 status;
    u8 chs_start[3];
    u8 type;
    u8 chs_end[3];
    u32 lba_start;
    u32 sector_count;
} __attribute__((packed)) mbr_partition_entry_t;
typedef struct {
    u8 boot_code[446];
    mbr_partition_entry_t partitions[4];
    u16 signature;
} __attribute__((packed)) mbr_t;
typedef struct {
    u64 signature;
    u32 revision;
    u32 header_size;
    u32 crc32;
    u32 reserved;
    u64 current_lba;
    u64 backup_lba;
    u64 first_usable_lba;
    u64 last_usable_lba;
    u8 disk_guid[16];
    u64 partition_array_lba;
    u32 partition_count;
    u32 partition_entry_size;
    u32 partition_array_crc32;
} __attribute__((packed)) gpt_header_t;
typedef struct {
    u8 type_guid[16];
    u8 unique_guid[16];
    u64 first_lba;
    u64 last_lba;
    u64 attributes;
    u16 name[36];
} __attribute__((packed)) gpt_partition_entry_t;
typedef struct {
    u16 ata_base;
    u8 ata_drive;
    u64 start_lba;
    u64 size;
    u8 type;
} partition_t;
void partitions_init(u16 ata_base, u8 ata_drive);
int partitions_get_count(void);
partition_t* partitions_get(int index);
int partitions_read_sector(int index, u64 lba, void *buf);
int partitions_write_sector(int index, u64 lba, const void *buf);
#endif