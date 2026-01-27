#include "drivers/partitions/partitions.h"
#include "drivers/ata/ata.h"
#include <kernel/mem/kernel_memory/kernel_memory.h>
#define MAX_PARTITIONS 16
static partition_t partitions[MAX_PARTITIONS];
static int partition_count = 0;
void partitions_init(u16 ata_base, u8 ata_drive) {
    partition_count = 0;
    u8 sector[512];
    if (ata_read_sector(ata_base, ata_drive, 0, sector) != 0) return;
    mbr_t *mbr = (mbr_t*)sector;
    if (mbr->signature == MBR_SIGNATURE) {
        for(int i = 0; i < 4; i++) {
            if (mbr->partitions[i].type != 0 && partition_count < MAX_PARTITIONS) {
                partitions[partition_count].ata_base = ata_base;
                partitions[partition_count].ata_drive = ata_drive;
                partitions[partition_count].start_lba = mbr->partitions[i].lba_start;
                partitions[partition_count].size = mbr->partitions[i].sector_count;
                partitions[partition_count].type = mbr->partitions[i].type;
                partition_count++;
            }
        }
    } else {
        gpt_header_t *gpt = (gpt_header_t*)sector;
        if (gpt->signature == 0x5452415020494645ULL) {
            u64 array_lba = gpt->partition_array_lba;
            u32 count = gpt->partition_count;
            u32 entry_size = gpt->partition_entry_size;
            u32 entries_per_sector = 512 / entry_size;
            for(u32 i = 0; i < count && partition_count < MAX_PARTITIONS; i++) {
                u64 sector_num = array_lba + i / entries_per_sector;
                u32 offset = (i % entries_per_sector) * entry_size;
                if (ata_read_sector(ata_base, ata_drive, sector_num, sector) != 0) break;
                gpt_partition_entry_t *entry = (gpt_partition_entry_t*)(sector + offset);
                int zero = 1;
                for(int j = 0; j < 16; j++) if (entry->type_guid[j]) zero = 0;
                if (!zero) {
                    partitions[partition_count].ata_base = ata_base;
                    partitions[partition_count].ata_drive = ata_drive;
                    partitions[partition_count].start_lba = entry->first_lba;
                    partitions[partition_count].size = entry->last_lba - entry->first_lba + 1;
                    partitions[partition_count].type = 0;
                    partition_count++;
                }
            }
        }
    }
}
int partitions_get_count(void) {
    return partition_count;
}
partition_t* partitions_get(int index) {
    if (index >= 0 && index < partition_count) return &partitions[index];
    return NULL;
}
int partitions_read_sector(int index, u64 lba, void *buf) {
    partition_t *p = partitions_get(index);
    if (!p) return -1;
    if (lba >= p->size) return -1;
    return ata_read_sector(p->ata_base, p->ata_drive, p->start_lba + lba, buf);
}
int partitions_write_sector(int index, u64 lba, const void *buf) {
    partition_t *p = partitions_get(index);
    if (!p) return -1;
    if (lba >= p->size) return -1;
    return ata_write_sector(p->ata_base, p->ata_drive, p->start_lba + lba, buf);
}