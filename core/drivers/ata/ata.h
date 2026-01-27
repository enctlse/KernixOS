#define ATA_DEFAULT_PRIMARY_IO 0x1F0
#define ATA_DEFAULT_PRIMARY_CONTROL 0x3F6
#define ATA_DEFAULT_SECONDARY_IO 0x170
#define ATA_DEFAULT_SECONDARY_CONTROL 0x376
#define ATA_DEFAULT_PRIMARY_DMA_COMMAND 0x1F0
#define ATA_DEFAULT_PRIMARY_DMA_STATUS 0x1F2
#define ATA_DEFAULT_PRIMARY_DMA_PRDT 0x1F4
#define ATA_DEFAULT_SECONDARY_DMA_COMMAND 0x170
#define ATA_DEFAULT_SECONDARY_DMA_STATUS 0x172
#define ATA_DEFAULT_SECONDARY_DMA_PRDT 0x174
void ata_init(void);
int ata_read_sector(u16 base, u8 drive, u64 lba, void *buf);
int ata_write_sector(u16 base, u8 drive, u64 lba, const void *buf);
int ata_identify(u16 base, u8 drive, void *buf);
int ata_read_sector_dma(u16 base, u8 drive, u64 lba, void *buf);
int ata_write_sector_dma(u16 base, u8 drive, u64 lba, const void *buf);