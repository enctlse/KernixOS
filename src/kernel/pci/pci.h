#ifndef PCI_H
#define PCI_H
#include <types.h>
#include "device.h"
#include "config.h"
#include "express.h"
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC
void pci_init(void);
int pci_get_device_count(void);
pci_device_t* pci_get_device(int index);
#endif