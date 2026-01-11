#include "pci.h"
#include "device.h"
#include "config.h"
#include "express.h"
#include <string/string.h>
#include <ui/theme/colors.h>
#include <kernel/graph/theme.h>
#include <config/boot.h>
void pci_init(void) {
    char buf[64];
    buf[0] = '\0';
    BOOTUP_PRINT("[PCI] ", gray_70);
    BOOTUP_PRINT("Init PCI/PCIe \n", theme_white());
    pci_device_init();
    int count = pci_device_get_count();
    int pcie_count = 0;
    for (int i = 0; i < count; i++) {
        pci_device_t *dev = pci_device_get(i);
        if (dev && pcie_is_device(dev->bus, dev->device, dev->function)) {
            pcie_count++;
        }
    }
    BOOTUP_PRINT("[PCI] ", gray_70);
    BOOTUP_PRINT("found: ", theme_white());
    str_append_uint(buf, pcie_count);
    BOOTUP_PRINT(buf, theme_white());
    BOOTUP_PRINT(" device(s)\n", theme_white());
}
int pci_get_device_count(void) {
    return pci_device_get_count();
}
pci_device_t* pci_get_device(int index) {
    return pci_device_get(index);
}