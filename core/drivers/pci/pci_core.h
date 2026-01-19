#ifndef BUS_PCI_CORE_H
#define BUS_PCI_CORE_H
#include <outputs/types.h>
#include "devices.h"
#include "configuration.h"
#include "express_pci.h"
#define BUS_CONFIG_ADDR_PORT 0xCF8
#define BUS_CONFIG_DATA_PORT 0xCFC
void bus_pci_initialize(void);
int bus_pci_device_total(void);
pci_device_t* bus_pci_device_at(int index);
#endif