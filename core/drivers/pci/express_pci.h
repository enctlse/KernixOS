#ifndef BUS_PCIE_H
#define BUS_PCIE_H

#include <outputs/types.h>

// Перечисление скоростей PCIe
typedef enum {
    PCIE_LINK_SPEED_UNKNOWN = 0,
    PCIE_LINK_SPEED_GEN1 = 1,    // 2.5 GT/s
    PCIE_LINK_SPEED_GEN2 = 2,    // 5.0 GT/s
    PCIE_LINK_SPEED_GEN3 = 3,    // 8.0 GT/s
    PCIE_LINK_SPEED_GEN4 = 4,    // 16.0 GT/s
    PCIE_LINK_SPEED_GEN5 = 5     // 32.0 GT/s
} pcie_link_speed_t;

// Проверка, является ли устройство PCIe
int bus_pcie_check_device(u8 bus_id, u8 slot_id, u8 func_id);

// Получение скорости линка PCIe
pcie_link_speed_t bus_pcie_get_speed(u8 bus_id, u8 slot_id, u8 func_id);

// Получение ширины линка PCIe
u8 bus_pcie_get_width(u8 bus_id, u8 slot_id, u8 func_id);

// Получение строкового описания скорости
const char* bus_pcie_speed_description(pcie_link_speed_t speed);

#endif