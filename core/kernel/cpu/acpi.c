#include "acpi.h"
#include <kernel/include/defs.h>
#include <string/string.h>
#include <drivers/memory/mem.h>
#include <ui/theme/colors.h>
#include <config/boot.h>
static rsdp_t* rsdp = NULL;
static madt_t* madt = NULL;
static u8 acpi_checksum(void* ptr, u32 length) {
    u8 sum = 0;
    u8* data = (u8*)ptr;
    for (u32 i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}
static void* acpi_find_table(const char* signature) {
    if (!rsdp) return NULL;
    if (rsdp->revision >= 2 && rsdp->xsdt_address) {
        SYSTEM_PRINT("[ACPI] Trying XSDT at 0x", gray_70);
        SYSTEM_PRINT_INT(rsdp->xsdt_address >> 32, cyan);
        SYSTEM_PRINT_INT(rsdp->xsdt_address & 0xFFFFFFFF, cyan);
        SYSTEM_PRINT("\n", gray_70);
        int xsdt_valid = 0;
        if (memmap_request.response) {
            for (u64 i = 0; i < memmap_request.response->entry_count; i++) {
                struct limine_memmap_entry *entry = memmap_request.response->entries[i];
                if (entry->type == LIMINE_MEMMAP_USABLE &&
                    rsdp->xsdt_address >= entry->base &&
                    rsdp->xsdt_address < entry->base + entry->length) {
                    xsdt_valid = 1;
                    break;
                }
            }
        }
        if (!xsdt_valid) {
            SYSTEM_PRINT("[ACPI] XSDT address not in usable memory\n", red);
        } else {
            u64* xsdt = (u64*)(rsdp->xsdt_address + hhdm_request.response->offset);
            SYSTEM_PRINT("[ACPI] XSDT mapped, checking header\n", gray_70);
            acpi_header_t* xsdt_header = (acpi_header_t*)xsdt;
            u32 entry_count = (xsdt_header->length - sizeof(acpi_header_t)) / sizeof(u64);
            SYSTEM_PRINT("[ACPI] XSDT entry count: ", gray_70);
            SYSTEM_PRINT_INT(entry_count, cyan);
            SYSTEM_PRINT("\n", gray_70);
            for (u32 i = 0; i < entry_count; i++) {
                SYSTEM_PRINT("[ACPI] Checking XSDT entry ", gray_70);
                SYSTEM_PRINT_INT(i, cyan);
                SYSTEM_PRINT("\n", gray_70);
                acpi_header_t* table = (acpi_header_t*)xsdt[i+1];
                if (memcmp(table->signature, signature, 4) == 0) {
                    SYSTEM_PRINT("[ACPI] Found table in XSDT\n", gray_70);
                    if (acpi_checksum(table, table->length) == 0) {
                        return table;
                    }
                }
            }
        }
    }
    if (rsdp->rsdt_address) {
        SYSTEM_PRINT("[ACPI] Trying RSDT at 0x", gray_70);
        SYSTEM_PRINT_INT(rsdp->rsdt_address >> 32, cyan);
        SYSTEM_PRINT_INT(rsdp->rsdt_address & 0xFFFFFFFF, cyan);
        SYSTEM_PRINT("\n", gray_70);
        int rsdt_valid = 0;
        if (memmap_request.response) {
            for (u64 i = 0; i < memmap_request.response->entry_count; i++) {
                struct limine_memmap_entry *entry = memmap_request.response->entries[i];
                if (entry->type == LIMINE_MEMMAP_USABLE &&
                    rsdp->rsdt_address >= entry->base &&
                    rsdp->rsdt_address < entry->base + entry->length) {
                    rsdt_valid = 1;
                    break;
                }
            }
        }
        if (!rsdt_valid) {
            SYSTEM_PRINT("[ACPI] RSDT address not in usable memory\n", red);
        } else {
            u32* rsdt = (u32*)(rsdp->rsdt_address + hhdm_request.response->offset);
            SYSTEM_PRINT("[ACPI] RSDT mapped, checking header\n", gray_70);
            acpi_header_t* rsdt_header = (acpi_header_t*)rsdt;
            u32 entry_count = (rsdt_header->length - sizeof(acpi_header_t)) / sizeof(u32);
            SYSTEM_PRINT("[ACPI] RSDT entry count: ", gray_70);
            SYSTEM_PRINT_INT(entry_count, cyan);
            SYSTEM_PRINT("\n", gray_70);
            for (u32 i = 0; i < entry_count; i++) {
                SYSTEM_PRINT("[ACPI] Checking RSDT entry ", gray_70);
                SYSTEM_PRINT_INT(i, cyan);
                SYSTEM_PRINT("\n", gray_70);
                acpi_header_t* table = (acpi_header_t*)rsdt[i+1];
                if (memcmp(table->signature, signature, 4) == 0) {
                    SYSTEM_PRINT("[ACPI] Found table in RSDT\n", gray_70);
                    if (acpi_checksum(table, table->length) == 0) {
                        return table;
                    }
                }
            }
        }
    }
    return NULL;
}
int acpi_init(void) {
    SYSTEM_PRINT("[ACPI] Starting acpi_init()\n", gray_70);
    if (!rsdp_request.response || !rsdp_request.response->address) {
        SYSTEM_PRINT("[ACPI] No RSDP provided by bootloader\n", red);
        return -1;
    }
    SYSTEM_PRINT("[ACPI] RSDP address: 0x", gray_70);
    SYSTEM_PRINT_INT((u64)rsdp_request.response->address >> 32, cyan);
    SYSTEM_PRINT_INT((u64)rsdp_request.response->address & 0xFFFFFFFF, cyan);
    SYSTEM_PRINT("\n", gray_70);
    int address_valid = 0;
    if (memmap_request.response) {
        for (u64 i = 0; i < memmap_request.response->entry_count; i++) {
            struct limine_memmap_entry *entry = memmap_request.response->entries[i];
            if (entry->type == LIMINE_MEMMAP_USABLE &&
                rsdp_request.response->address >= entry->base &&
                rsdp_request.response->address < entry->base + entry->length) {
                address_valid = 1;
                break;
            }
        }
    }
    if (!address_valid) {
        SYSTEM_PRINT("[ACPI] RSDP address not in usable memory\n", red);
        return -1;
    }
    SYSTEM_PRINT("[ACPI] About to map RSDP to virtual address\n", gray_70);
    rsdp = (rsdp_t*)(rsdp_request.response->address + hhdm_request.response->offset);
    SYSTEM_PRINT("[ACPI] RSDP mapped successfully\n", gray_70);
    SYSTEM_PRINT("[ACPI] About to check RSDP signature\n", gray_70);
    if (memcmp(rsdp->signature, "RSD PTR ", 8) != 0) {
        SYSTEM_PRINT("[ACPI] Invalid RSDP signature\n", red);
        return -1;
    }
    SYSTEM_PRINT("[ACPI] RSDP signature valid\n", gray_70);
    if (acpi_checksum(rsdp, 20) != 0) {
        SYSTEM_PRINT("[ACPI] RSDP checksum failed\n", red);
        return -1;
    }
    if (rsdp->revision >= 2 && acpi_checksum(rsdp, rsdp->length) != 0) {
        SYSTEM_PRINT("[ACPI] Extended RSDP checksum failed\n", red);
        return -1;
    }
    SYSTEM_PRINT("[ACPI] RSDP found\n", gray_70);
    return 0;
}
madt_t* acpi_get_madt(void) {
    if (!madt) {
        madt = (madt_t*)acpi_find_table("APIC");
    }
    return madt;
}
void acpi_parse_madt_cpus(void) {
    madt_t* madt = acpi_get_madt();
    if (!madt) {
        SYSTEM_PRINT("[ACPI] MADT not found\n", red);
        return;
    }
    SYSTEM_PRINT("[ACPI] Parsing MADT for CPUs\n", gray_70);
    u8* entry_ptr = (u8*)madt + sizeof(madt_t);
    u8* madt_end = (u8*)madt + madt->header.length;
    while (entry_ptr < madt_end) {
        madt_entry_header_t* entry = (madt_entry_header_t*)entry_ptr;
        if (entry->type == MADT_TYPE_LOCAL_APIC) {
            madt_local_apic_t* lapic = (madt_local_apic_t*)entry;
            if (lapic->flags & 1) {
                SYSTEM_PRINT("  CPU ", gray_70);
                SYSTEM_PRINT_INT(lapic->processor_id, cyan);
                SYSTEM_PRINT(" APIC ID ", gray_70);
                SYSTEM_PRINT_INT(lapic->apic_id, cyan);
                SYSTEM_PRINT("\n", gray_70);
            }
        }
        entry_ptr += entry->length;
    }
}