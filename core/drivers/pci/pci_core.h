#ifndef BUS_PCI_CORE_H
#define BUS_PCI_CORE_H

// Основные заголовки для работы с PCI шиной
#include <outputs/types.h>
#include "devices.h"
#include "configuration.h"
#include "express_pci.h"

// Адреса портов для доступа к конфигурационному пространству PCI
#define BUS_CONFIG_ADDR_PORT 0xCF8
#define BUS_CONFIG_DATA_PORT 0xCFC

// Инициализация подсистемы PCI
void bus_pci_initialize(void);

// Получение количества найденных устройств
int bus_pci_device_total(void);

// Получение устройства по индексу
pci_device_t* bus_pci_device_at(int index);

#endif