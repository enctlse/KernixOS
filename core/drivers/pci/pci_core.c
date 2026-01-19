#include "pci_core.h"
#include "devices.h"
#include "configuration.h"
#include "express_pci.h"
#include <string/string.h>
#include <ui/theme/colors.h>
#include <config/boot.h>

// Инициализация подсистемы PCI и подсчет PCIe устройств
void bus_pci_initialize(void) {
    char message_buffer[64];
    message_buffer[0] = '\0';

    // Выводим сообщение о начале инициализации
    SYSTEM_PRINT("[PCI] ", gray_70);
    SYSTEM_PRINT("Starting PCI/PCIe initialization\n", theme_white);

    // Инициализируем сканирование устройств
    bus_device_initialize();

    // Получаем общее количество устройств
    int total_devices = bus_device_count();
    int pcie_devices_found = 0;

    // Подсчитываем PCIe устройства
    for (int device_index = 0; device_index < total_devices; device_index++) {
        pci_device_t *current_device = bus_device_get(device_index);
        if (current_device && bus_pcie_check_device(current_device->bus_number, current_device->slot_number, current_device->function_number)) {
            pcie_devices_found++;
        }
    }

    // Выводим результат
    SYSTEM_PRINT("[PCI] ", gray_70);
    SYSTEM_PRINT("Detected PCIe devices: ", theme_white);
    str_append_uint(message_buffer, pcie_devices_found);
    SYSTEM_PRINT(message_buffer, theme_white);
    SYSTEM_PRINT(" device(s)\n", theme_white);
}

// Возвращает общее количество найденных устройств
int bus_pci_device_total(void) {
    return bus_device_count();
}

// Возвращает устройство по индексу
pci_device_t* bus_pci_device_at(int index) {
    return bus_device_get(index);
}