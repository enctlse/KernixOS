#ifndef BUS_DEVICE_H
#define BUS_DEVICE_H

#include <outputs/types.h>

// thing that holds pci device info
typedef struct {
    u16 manufacturer_id;    // maker's mark
    u16 product_id;         // thing's id
    u8 bus_number;          // which bus
    u8 slot_number;         // where it sits
    u8 function_number;     // what it does
    u8 device_class;        // what kind
    u8 device_subclass;     // sub kind
    u8 prog_interface;      // how to talk
    u8 revision_id;         // version
    u8 header_format;       // header shape
    u8 is_multifunction;    // many jobs?
    u32 base_addresses[6];  // address bars
} pci_device_t;

// start the device system
void bus_device_initialize(void);

// look for devices on a bus
void bus_device_scan_bus(u8 bus_id);

// how many devices found
int bus_device_count(void);

// get device by number
pci_device_t* bus_device_get(int device_index);

// find by class and sub
pci_device_t* bus_device_find_by_class(u8 class_id, u8 subclass_id);

// find by maker and model
pci_device_t* bus_device_find_by_vendor(u16 vendor_id, u16 device_id);

// describe the class
const char* bus_device_class_description(pci_device_t *device);

#endif