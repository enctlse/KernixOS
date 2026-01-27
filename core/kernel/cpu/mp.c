#include "mp.h"
#include <kernel/include/defs.h>
#include <string/string.h>
#include <drivers/memory/mem.h>
#include <ui/theme/colors.h>
#include <config/boot.h>
static mp_floating_pointer_t* mp_fp = NULL;
mp_config_table_t* mp_config = NULL;
static u8 mp_checksum(void* ptr, u32 length) {
    u8 sum = 0;
    u8* data = (u8*)ptr;
    for (u32 i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}
static mp_floating_pointer_t* mp_find_floating_pointer(void) {
    SYSTEM_PRINT("[MP] Searching EBDA for MP floating pointer\n", gray_70);
    u16* ebda_segment = (u16*)0x40E;
    u32 ebda_addr = (*ebda_segment) << 4;
    u32 ebda_end = ebda_addr + 1024;
    SYSTEM_PRINT("[MP] EBDA addr: 0x", gray_70);
    SYSTEM_PRINT_INT(ebda_addr, cyan);
    SYSTEM_PRINT(" to 0x", gray_70);
    SYSTEM_PRINT_INT(ebda_end, cyan);
    SYSTEM_PRINT("\n", gray_70);
    for (u32 addr = ebda_addr; addr < ebda_end; addr += 16) {
        SYSTEM_PRINT("[MP] Checking EBDA addr 0x", gray_70);
        SYSTEM_PRINT_INT(addr, cyan);
        SYSTEM_PRINT("\n", gray_70);
        if (memcmp((void*)addr, "_MP_", 4) == 0) {
            mp_floating_pointer_t* fp = (mp_floating_pointer_t*)addr;
            if (mp_checksum(fp, fp->length * 16) == 0) {
                SYSTEM_PRINT("[MP] Found valid MP FP in EBDA\n", gray_70);
                return fp;
            }
        }
    }
    SYSTEM_PRINT("[MP] Searching base memory for MP floating pointer\n", gray_70);
    for (u32 addr = 0xF0000; addr < 0x100000; addr += 16) {
        SYSTEM_PRINT("[MP] Checking base addr 0x", gray_70);
        SYSTEM_PRINT_INT(addr, cyan);
        SYSTEM_PRINT("\n", gray_70);
        if (memcmp((void*)addr, "_MP_", 4) == 0) {
            mp_floating_pointer_t* fp = (mp_floating_pointer_t*)addr;
            if (mp_checksum(fp, fp->length * 16) == 0) {
                SYSTEM_PRINT("[MP] Found valid MP FP in base memory\n", gray_70);
                return fp;
            }
        }
    }
    SYSTEM_PRINT("[MP] MP floating pointer not found\n", red);
    return NULL;
}
int mp_init(void) {
    mp_fp = mp_find_floating_pointer();
    if (!mp_fp) {
        SYSTEM_PRINT("[MP] MP floating pointer not found\n", red);
        return -1;
    }
    if (mp_fp->configuration_table == 0) {
        SYSTEM_PRINT("[MP] Default configuration not supported\n", red);
        return -1;
    }
    SYSTEM_PRINT("[MP] MP config table at 0x", gray_70);
    SYSTEM_PRINT_INT(mp_fp->configuration_table >> 32, cyan);
    SYSTEM_PRINT_INT(mp_fp->configuration_table & 0xFFFFFFFF, cyan);
    SYSTEM_PRINT(", mapping to virtual\n", gray_70);
    int config_valid = 0;
    if (memmap_request.response) {
        for (u64 i = 0; i < memmap_request.response->entry_count; i++) {
            struct limine_memmap_entry *entry = memmap_request.response->entries[i];
            if (entry->type == LIMINE_MEMMAP_USABLE &&
                mp_fp->configuration_table >= entry->base &&
                mp_fp->configuration_table < entry->base + entry->length) {
                config_valid = 1;
                break;
            }
        }
    }
    if (!config_valid) {
        SYSTEM_PRINT("[MP] MP config address not in usable memory\n", red);
        return -1;
    }
    mp_config = (mp_config_table_t*)(mp_fp->configuration_table + hhdm_request.response->offset);
    SYSTEM_PRINT("[MP] MP config mapped\n", gray_70);
    if (memcmp(mp_config->signature, "PCMP", 4) != 0) {
        SYSTEM_PRINT("[MP] Invalid MP configuration table signature\n", red);
        return -1;
    }
    if (mp_checksum(mp_config, mp_config->length) != 0) {
        SYSTEM_PRINT("[MP] MP configuration table checksum failed\n", red);
        return -1;
    }
    SYSTEM_PRINT("[MP] MP table found\n", gray_70);
    return 0;
}
void mp_parse_cpus(void) {
    if (!mp_config) return;
    SYSTEM_PRINT("[MP] Parsing MP table for CPUs\n", gray_70);
    u8* entry_ptr = (u8*)mp_config + sizeof(mp_config_table_t);
    u8* table_end = (u8*)mp_config + mp_config->length;
    while (entry_ptr < table_end) {
        u8 entry_type = *entry_ptr;
        if (entry_type == MP_ENTRY_PROCESSOR) {
            mp_processor_entry_t* proc = (mp_processor_entry_t*)entry_ptr;
            if (proc->cpu_flags & 1) {
                SYSTEM_PRINT("  CPU APIC ID ", gray_70);
                SYSTEM_PRINT_INT(proc->local_apic_id, cyan);
                SYSTEM_PRINT("\n", gray_70);
            }
            entry_ptr += sizeof(mp_processor_entry_t);
        } else {
            entry_ptr += 8;
        }
    }
}