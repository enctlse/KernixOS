#ifndef BUS_PCIE_H
#define BUS_PCIE_H
#include <outputs/types.h>
typedef enum {
    PCIE_LINK_SPEED_UNKNOWN = 0,
    PCIE_LINK_SPEED_GEN1 = 1,
    PCIE_LINK_SPEED_GEN2 = 2,
    PCIE_LINK_SPEED_GEN3 = 3,
    PCIE_LINK_SPEED_GEN4 = 4,
    PCIE_LINK_SPEED_GEN5 = 5
} pcie_link_speed_t;
int bus_pcie_check_device(u8 bus_id, u8 slot_id, u8 func_id);
pcie_link_speed_t bus_pcie_get_speed(u8 bus_id, u8 slot_id, u8 func_id);
u8 bus_pcie_get_width(u8 bus_id, u8 slot_id, u8 func_id);
const char* bus_pcie_speed_description(pcie_link_speed_t speed);
#endif