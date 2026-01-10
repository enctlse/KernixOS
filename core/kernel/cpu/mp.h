#ifndef MP_H
#define MP_H
#include <types.h>
typedef struct {
    char signature[4];
    u32 configuration_table;
    u8 length;
    u8 version;
    u8 checksum;
    u8 features[5];
} __attribute__((packed)) mp_floating_pointer_t;
typedef struct {
    char signature[4];
    u16 length;
    u8 version;
    u8 checksum;
    char oem_id[8];
    char product_id[12];
    u32 oem_table_pointer;
    u16 oem_table_size;
    u16 entry_count;
    u32 lapic_address;
    u16 extended_table_length;
    u8 extended_table_checksum;
    u8 reserved;
} __attribute__((packed)) mp_config_table_t;
#define MP_ENTRY_PROCESSOR 0
#define MP_ENTRY_BUS 1
#define MP_ENTRY_IO_APIC 2
#define MP_ENTRY_IO_INTERRUPT 3
#define MP_ENTRY_LOCAL_INTERRUPT 4
typedef struct {
    u8 entry_type;
    u8 local_apic_id;
    u8 local_apic_version;
    u8 cpu_flags;
    u32 cpu_signature;
    u32 feature_flags;
    u32 reserved[2];
} __attribute__((packed)) mp_processor_entry_t;
int mp_init(void);
void mp_parse_cpus(void);
extern mp_config_table_t* mp_config;
#endif