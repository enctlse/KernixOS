#ifndef BUS_DEVICE_H
#define BUS_DEVICE_H
#include <outputs/types.h>
typedef struct {
    u16 manufacturer_id;
    u16 product_id;
    u8 bus_number;
    u8 slot_number;
    u8 function_number;
    u8 device_class;
    u8 device_subclass;
    u8 prog_interface;
    u8 revision_id;
    u8 header_format;
    u8 is_multifunction;
    u32 base_addresses[6];
} pci_device_t;
void bus_device_initialize(void);
void bus_device_scan_bus(u8 bus_id);
int bus_device_count(void);
pci_device_t* bus_device_get(int device_index);
pci_device_t* bus_device_find_by_class(u8 class_id, u8 subclass_id);
pci_device_t* bus_device_find_by_vendor(u16 vendor_id, u16 device_id);
const char* bus_device_class_description(pci_device_t *device);
#endif