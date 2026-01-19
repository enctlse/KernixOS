#include <outputs/types.h>
#include <kernel/include/io.h>
#include <string/string.h>
#include "ata.h"
#include <drivers/pci/pci_core.h>
#include <drivers/pci/devices.h>
#include "drivers/partitions/partitions.h"
#include "fs/vfs/vfs.h"
#include <string/string.h>
#include <kernel/shell/acsh.h>
u16 ata_primary_io = ATA_DEFAULT_PRIMARY_IO;
u16 ata_primary_ctrl = ATA_DEFAULT_PRIMARY_CONTROL;
u16 ata_secondary_io = ATA_DEFAULT_SECONDARY_IO;
u16 ata_secondary_ctrl = ATA_DEFAULT_SECONDARY_CONTROL;
u16 ata_bm_primary = ATA_DEFAULT_PRIMARY_DMA_COMMAND;
u16 ata_bm_secondary = ATA_DEFAULT_SECONDARY_DMA_COMMAND;
#define ATA_DATA 0
#define ATA_ERROR 1
#define ATA_SECTOR_COUNT 2
#define ATA_LBA_LOW 3
#define ATA_LBA_MID 4
#define ATA_LBA_HIGH 5
#define ATA_DRIVE 6
#define ATA_STATUS 7
#define ATA_COMMAND 7
#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_IDENTIFY 0xEC
// LBA48 commands
#define ATA_CMD_READ_EXT 0x24
#define ATA_CMD_WRITE_EXT 0x34
#define ATA_STATUS_BSY 0x80
#define ATA_STATUS_RDY 0x40
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DF 0x20

// Prototypes for LBA48 functions
int ata_read_sector_lba48(u16 base, u8 drive, u64 lba, void *buf);
int ata_write_sector_lba48(u16 base, u8 drive, u64 lba, const void *buf);
static inline void ata_wait_bsy(u16 base) {
    while (inb(base + ATA_STATUS) & ATA_STATUS_BSY);
}
static inline void ata_wait_rdy(u16 base) {
    while (!(inb(base + ATA_STATUS) & ATA_STATUS_RDY));
}
static inline void ata_select_drive(u16 base, u8 drive) {
    outb(base + ATA_DRIVE, 0xA0 | (drive << 4));
    inb(base + ATA_STATUS);
    inb(base + ATA_STATUS);
    inb(base + ATA_STATUS);
    inb(base + ATA_STATUS);
}
static inline int ata_poll(u16 base) {
    int timeout = 1000000;
    for(int i = 0; i < timeout; i++) {
        u8 status = inb(base + ATA_STATUS);
        if (!(status & ATA_STATUS_BSY)) {
            if (status & ATA_STATUS_ERR) return -1;
            if (status & ATA_STATUS_DF) return -1;
            if (status & ATA_STATUS_DRQ) return 0;
        }
    }
    return -2; // timeout
}
int ata_read_sector(u16 base, u8 drive, u64 lba, void *buf) {
    int retries = 3;
    for(int r = 0; r < retries; r++) {
        if (lba >= 0x10000000ULL) {
            int res = ata_read_sector_lba48(base, drive, lba, buf);
            if (res == 0) return 0;
            continue;
        }
        // LBA28
        ata_select_drive(base, drive);
        outb(base + ATA_SECTOR_COUNT, 1);
        outb(base + ATA_LBA_LOW, lba & 0xFF);
        outb(base + ATA_LBA_MID, (lba >> 8) & 0xFF);
        outb(base + ATA_LBA_HIGH, (lba >> 16) & 0xFF);
        outb(base + ATA_DRIVE, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
        outb(base + ATA_COMMAND, ATA_CMD_READ);
        int poll_res = ata_poll(base);
        if (poll_res != 0) continue;
        for (int i = 0; i < 256; i++) {
            ((u16*)buf)[i] = inw(base + ATA_DATA);
        }
        return 0;
    }
    return -1;
}
int ata_write_sector(u16 base, u8 drive, u64 lba, const void *buf) {
    if (lba >= 0x10000000ULL) { // Use LBA48 for large addresses
        return ata_write_sector_lba48(base, drive, lba, buf);
    }
    // LBA28
    ata_select_drive(base, drive);
    outb(base + ATA_SECTOR_COUNT, 1);
    outb(base + ATA_LBA_LOW, lba & 0xFF);
    outb(base + ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(base + ATA_DRIVE, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
    outb(base + ATA_COMMAND, ATA_CMD_WRITE);
    if (ata_poll(base) != 0) return -1;
    for (int i = 0; i < 256; i++) {
        outw(base + ATA_DATA, ((u16*)buf)[i]);
    }
    outb(base + ATA_COMMAND, 0xE7);
    if (ata_poll(base) != 0) return -1;
    return 0;
}
int ata_identify(u16 base, u8 drive, void *buf) {
    ata_select_drive(base, drive);
    // Delay
    for(int i = 0; i < 4; i++) inb(0x80);
    outb(base + ATA_SECTOR_COUNT, 0);
    outb(base + ATA_LBA_LOW, 0);
    outb(base + ATA_LBA_MID, 0);
    outb(base + ATA_LBA_HIGH, 0);
    outb(base + ATA_COMMAND, ATA_CMD_IDENTIFY);
    // Delay
    for(int i = 0; i < 4; i++) inb(0x80);
    u8 status = inb(base + ATA_STATUS);
    if (status == 0) {
        print("ATA IDENTIFY: status 0, no device\n", red);
        return -1;
    }
    int timeout = 1000000;
    for (int i = 0; i < timeout; i++) {
        status = inb(base + ATA_STATUS);
        if (!(status & ATA_STATUS_BSY)) {
            if (status & ATA_STATUS_ERR) {
                print("ATA IDENTIFY: error status\n", red);
                return -1;
            }
            if (status & ATA_STATUS_DRQ) {
                print("ATA IDENTIFY: data ready\n", green);
                for (int j = 0; j < 256; j++) {
                    ((u16*)buf)[j] = inw(base + ATA_DATA);
                }
                return 0;
            }
        }
    }
    print("ATA IDENTIFY: timeout\n", red);
    return -1;
}
int ata_read_sector_lba48(u16 base, u8 drive, u64 lba, void *buf) {
    ata_select_drive(base, drive);
    outb(base + ATA_SECTOR_COUNT, 0); // High byte of sector count (0 for 1 sector)
    outb(base + ATA_LBA_LOW, (lba >> 24) & 0xFF); // LBA 24-31
    outb(base + ATA_LBA_MID, (lba >> 32) & 0xFF); // LBA 32-39
    outb(base + ATA_LBA_HIGH, (lba >> 40) & 0xFF); // LBA 40-47
    outb(base + ATA_SECTOR_COUNT, 1); // Low byte of sector count
    outb(base + ATA_LBA_LOW, lba & 0xFF); // LBA 0-7
    outb(base + ATA_LBA_MID, (lba >> 8) & 0xFF); // LBA 8-15
    outb(base + ATA_LBA_HIGH, (lba >> 16) & 0xFF); // LBA 16-23
    outb(base + ATA_DRIVE, 0x40 | (drive << 4)); // LBA mode
    outb(base + ATA_COMMAND, ATA_CMD_READ_EXT);
    if (ata_poll(base) != 0) return -1;
    for (int i = 0; i < 256; i++) {
        ((u16*)buf)[i] = inw(base + ATA_DATA);
    }
    return 0;
}

int ata_write_sector_lba48(u16 base, u8 drive, u64 lba, const void *buf) {
    ata_select_drive(base, drive);
    outb(base + ATA_SECTOR_COUNT, 0); // High byte of sector count
    outb(base + ATA_LBA_LOW, (lba >> 24) & 0xFF); // LBA 24-31
    outb(base + ATA_LBA_MID, (lba >> 32) & 0xFF); // LBA 32-39
    outb(base + ATA_LBA_HIGH, (lba >> 40) & 0xFF); // LBA 40-47
    outb(base + ATA_SECTOR_COUNT, 1); // Low byte of sector count
    outb(base + ATA_LBA_LOW, lba & 0xFF); // LBA 0-7
    outb(base + ATA_LBA_MID, (lba >> 8) & 0xFF); // LBA 8-15
    outb(base + ATA_LBA_HIGH, (lba >> 16) & 0xFF); // LBA 16-23
    outb(base + ATA_DRIVE, 0x40 | (drive << 4)); // LBA mode
    outb(base + ATA_COMMAND, ATA_CMD_WRITE_EXT);
    if (ata_poll(base) != 0) return -1;
    for (int i = 0; i < 256; i++) {
        outw(base + ATA_DATA, ((u16*)buf)[i]);
    }
    outb(base + ATA_COMMAND, 0xE7); // Cache flush
    if (ata_poll(base) != 0) return -1;
    return 0;
}

typedef struct {
    u32 prdt_addr;
    u16 byte_count;
    u16 last;
} __attribute__((packed)) prdt_entry;

int ata_read_sector_dma(u16 base, u8 drive, u64 lba, void *buf) {
    // Simplified DMA implementation, assumes PRDT is set up
    // For full implementation, need to set up PRDT in memory
    // This is a placeholder
    return ata_read_sector(base, drive, lba, buf);
}

int ata_write_sector_dma(u16 base, u8 drive, u64 lba, const void *buf) {
    // Simplified DMA implementation
    return ata_write_sector(base, drive, lba, buf);
}

void ata_init(void) {
    // PCI scan for ATA controllers
    pci_device_t *dev = bus_device_find_by_class(0x01, 0x01);
    if (dev) {
        u32 bar0 = dev->base_addresses[0];
        u32 bar1 = dev->base_addresses[1];
        u32 bar2 = dev->base_addresses[2];
        u32 bar3 = dev->base_addresses[3];
        u32 bar4 = dev->base_addresses[4];
        if ((bar0 & 1) != 0) { // I/O space
            ata_primary_io = bar0 & ~1;
            ata_primary_ctrl = bar1 & ~1;
            ata_secondary_io = bar2 & ~1;
            ata_secondary_ctrl = bar3 & ~1;
            ata_bm_primary = bar4 & ~1;
            ata_bm_secondary = (bar4 & ~1) + 8;
        }
    }
    // If not found, use defaults
    u16 identify[256];
    if (ata_identify(ata_primary_io, 0, identify) == 0) {
        print("ATA: primary master detected\n", green);
        partitions_init(ata_primary_io, 0);
        print("ATA: partitions for primary master initialized\n", green);
    } else {
        print("ATA: primary master not detected\n", red);
    }
    if (ata_identify(ata_primary_io, 1, identify) == 0) {
        print("ATA: primary slave detected\n", green);
        partitions_init(ata_primary_io, 1);
    } else {
        print("ATA: primary slave not detected\n", red);
    }
    if (ata_identify(ata_secondary_io, 0, identify) == 0) {
        print("ATA: secondary master detected\n", green);
        partitions_init(ata_secondary_io, 0);
    } else {
        print("ATA: secondary master not detected\n", red);
    }
    if (ata_identify(ata_secondary_io, 1, identify) == 0) {
        print("ATA: secondary slave detected\n", green);
        partitions_init(ata_secondary_io, 1);
    } else {
        print("ATA: secondary slave not detected\n", red);
    }
    int part_count = partitions_get_count();
    print("Total partitions: ", white);
    printInt(part_count, cyan);
    print("\n", white);
    // Partitions initialized, mount manually in shell: mount ata0p0 /disk ext4
}