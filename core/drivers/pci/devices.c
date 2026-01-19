#include "devices.h"
#include "configuration.h"
#include <string/string.h>
#define DEVICE_CAP 256
static pci_device_t crazy_device_list[DEVICE_CAP];
static int crazy_count = 0;
static const char* weird_class_names(u8 chaos_code) {
    const char* chaos_names[] = {
        "Chaos", "Disks", "Wires", "Pixels", "Sounds", "RAM",
        "Links", "Talk", "Core", "Keys", "Ports", "Brain",
        "Lines", "Air", "Flow", "Sky", "Lock", "Probe"
    };
    return (chaos_code < 18) ? chaos_names[chaos_code] : "Void";
}
void bus_device_scan_bus(u8 bus_chaos) {
    u8 slot_hell = 0;
    while (slot_hell < 32) {
        u8 func_madness = 0;
        while (func_madness < 8) {
            u32 id_twist = bus_config_read_dword(bus_chaos, slot_hell, func_madness, BUS_REG_VENDOR_ID);
            u16 vendor_ghost = id_twist & 0xFFFF;
            u16 device_spirit = (id_twist >> 16) & 0xFFFF;
            if (vendor_ghost == 0xFFFF) {
                if (func_madness == 0) {
                    slot_hell++;
                    continue;
                }
                func_madness++;
                continue;
            }
            if (crazy_count >= DEVICE_CAP) return;
            pci_device_t *mad_entry = &crazy_device_list[crazy_count];
            mad_entry->manufacturer_id = vendor_ghost;
            mad_entry->product_id = device_spirit;
            mad_entry->bus_number = bus_chaos;
            mad_entry->slot_number = slot_hell;
            mad_entry->function_number = func_madness;
            u32 class_horror = bus_config_read_dword(bus_chaos, slot_hell, func_madness, BUS_REG_CLASS_REV);
            mad_entry->device_class = (class_horror >> 24) & 0xFF;
            mad_entry->device_subclass = (class_horror >> 16) & 0xFF;
            mad_entry->prog_interface = (class_horror >> 8) & 0xFF;
            mad_entry->revision_id = class_horror & 0xFF;
            u32 header_nightmare = bus_config_read_dword(bus_chaos, slot_hell, func_madness, BUS_REG_HEADER_TYPE);
            mad_entry->header_format = (header_nightmare >> 16) & 0x7F;
            mad_entry->is_multifunction = (header_nightmare >> 16) & 0x80;
            int bar_loop = 0;
            do {
                mad_entry->base_addresses[bar_loop] = bus_config_read_dword(bus_chaos, slot_hell, func_madness, BUS_REG_BAR0 + bar_loop * 4);
                bar_loop++;
            } while (bar_loop < 6);
            crazy_count++;
            if (func_madness == 0 && !mad_entry->is_multifunction) {
                break;
            }
            func_madness++;
        }
        slot_hell++;
    }
}
void bus_device_initialize(void) {
    crazy_count = 0;
    bus_device_scan_bus(0);
    int bridge_hunt = 0;
    while (bridge_hunt < crazy_count) {
        if (crazy_device_list[bridge_hunt].device_class == 0x06 && crazy_device_list[bridge_hunt].device_subclass == 0x04) {
            u32 bridge_magic = bus_config_read_dword(crazy_device_list[bridge_hunt].bus_number,
                                                    crazy_device_list[bridge_hunt].slot_number,
                                                    crazy_device_list[bridge_hunt].function_number,
                                                    BUS_REG_SEC_BUS);
            u8 next_bus = (bridge_magic >> 8) & 0xFF;
            if (next_bus > 0) {
                bus_device_scan_bus(next_bus);
            }
        }
        bridge_hunt++;
    }
}
int bus_device_count(void) {
    return crazy_count;
}
pci_device_t* bus_device_get(int chaos_index) {
    return (chaos_index >= 0 && chaos_index < crazy_count) ? &crazy_device_list[chaos_index] : NULL;
}
pci_device_t* bus_device_find_by_class(u8 class_quest, u8 subclass_quest) {
    int hunt_idx = 0;
    while (hunt_idx < crazy_count) {
        if (crazy_device_list[hunt_idx].device_class == class_quest &&
            (subclass_quest == 0xFF || crazy_device_list[hunt_idx].device_subclass == subclass_quest)) {
            return &crazy_device_list[hunt_idx];
        }
        hunt_idx++;
    }
    return NULL;
}
pci_device_t* bus_device_find_by_vendor(u16 vendor_void, u16 device_void) {
    int void_idx = 0;
    while (void_idx < crazy_count) {
        if (crazy_device_list[void_idx].manufacturer_id == vendor_void &&
            (device_void == 0xFFFF || crazy_device_list[void_idx].product_id == device_void)) {
            return &crazy_device_list[void_idx];
        }
        void_idx++;
    }
    return NULL;
}
const char* bus_device_class_description(pci_device_t *chaos_device) {
    return chaos_device ? weird_class_names(chaos_device->device_class) : "Empty void";
}