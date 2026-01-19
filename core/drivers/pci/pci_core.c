#include "pci_core.h"
#include "devices.h"
#include "configuration.h"
#include "express_pci.h"
#include <string/string.h>
#include <ui/theme/colors.h>
#include <config/boot.h>
void bus_pci_initialize(void) {
    char message_buffer[64];
    message_buffer[0] = '\0';
    SYSTEM_PRINT("[PCI] ", gray_70);
    SYSTEM_PRINT("Starting PCI/PCIe initialization\n", theme_white);
    bus_device_initialize();
    int total_devices = bus_device_count();
    int pcie_devices_found = 0;
    for (int device_index = 0; device_index < total_devices; device_index++) {
        pci_device_t *current_device = bus_device_get(device_index);
        if (current_device && bus_pcie_check_device(current_device->bus_number, current_device->slot_number, current_device->function_number)) {
            pcie_devices_found++;
        }
    }
    SYSTEM_PRINT("[PCI] ", gray_70);
    SYSTEM_PRINT("Detected PCIe devices: ", theme_white);
    str_append_uint(message_buffer, pcie_devices_found);
    SYSTEM_PRINT(message_buffer, theme_white);
    SYSTEM_PRINT(" device(s)\n", theme_white);
}
int bus_pci_device_total(void) {
    return bus_device_count();
}
pci_device_t* bus_pci_device_at(int index) {
    return bus_device_get(index);
}