#ifndef ACPI_H
#define ACPI_H
#include <outputs/types.h>
typedef struct {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oem_id[6];
    char oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} __attribute__((packed)) acpi_header_t;
typedef struct {
    char signature[8];
    u8 checksum;
    char oem_id[6];
    u8 revision;
    u32 rsdt_address;
    u32 length;
    u64 xsdt_address;
    u8 extended_checksum;
    u8 reserved[3];
} __attribute__((packed)) rsdp_t;
typedef u64 xsdt_entry_t;
typedef u32 rsdt_entry_t;
typedef struct {
    acpi_header_t header;
    u32 local_apic_address;
    u32 flags;
} __attribute__((packed)) madt_t;
#define MADT_TYPE_LOCAL_APIC 0
#define MADT_TYPE_IO_APIC 1
#define MADT_TYPE_INTERRUPT_OVERRIDE 2
#define MADT_TYPE_NMI_SOURCE 3
#define MADT_TYPE_LOCAL_APIC_NMI 4
#define MADT_TYPE_LOCAL_APIC_OVERRIDE 5
#define MADT_TYPE_IO_SAPIC 6
#define MADT_TYPE_LOCAL_SAPIC 7
#define MADT_TYPE_PLATFORM_INTERRUPT_SOURCES 8
#define MADT_TYPE_PROCESSOR_LOCAL_X2APIC 9
#define MADT_TYPE_LOCAL_X2APIC_NMI 10
#define MADT_TYPE_GIC_CPU_INTERFACE 11
#define MADT_TYPE_GIC_DISTRIBUTOR 12
#define MADT_TYPE_GIC_MSI_FRAME 13
#define MADT_TYPE_GIC_REDISTRIBUTOR 14
#define MADT_TYPE_GIC_INTERRUPT_TRANSLATION 15
typedef struct {
    u8 type;
    u8 length;
} __attribute__((packed)) madt_entry_header_t;
typedef struct {
    madt_entry_header_t header;
    u8 processor_id;
    u8 apic_id;
    u32 flags;
} __attribute__((packed)) madt_local_apic_t;
int acpi_init(void);
madt_t* acpi_get_madt(void);
void acpi_parse_madt_cpus(void);
#endif