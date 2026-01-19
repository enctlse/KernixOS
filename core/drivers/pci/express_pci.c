#include "express_pci.h"
#include "configuration.h"

#define KEY_PCIE 0x10

// describe speed first
const char* bus_pcie_speed_description(pcie_link_speed_t thing) {
    if (thing == PCIE_LINK_SPEED_GEN1) return "2.5 gt/s gen1";
    if (thing == PCIE_LINK_SPEED_GEN2) return "5.0 gt/s gen2";
    if (thing == PCIE_LINK_SPEED_GEN3) return "8.0 gt/s gen3";
    if (thing == PCIE_LINK_SPEED_GEN4) return "16.0 gt/s gen4";
    if (thing == PCIE_LINK_SPEED_GEN5) return "32.0 gt/s gen5";
    return "unknown speed";
}

// get width
u8 bus_pcie_get_width(u8 path, u8 spot, u8 role) {
    u16 state = bus_config_read_word(path, spot, role, BUS_REG_STATUS);
    if (!(state & 0x10)) return 0;

    u8 walker = bus_config_read_byte(path, spot, role, BUS_REG_CAP_PTR);
    do {
        if (walker == 0) return 0;
        u8 current = bus_config_read_byte(path, spot, role, walker);
        if (current == KEY_PCIE) {
            u16 info = bus_config_read_word(path, spot, role, walker + 0x12);
            return (info >> 4) & 0x3F;
        }
        walker = bus_config_read_byte(path, spot, role, walker + 1);
    } while (1);
}

// check device
int bus_pcie_check_device(u8 path, u8 spot, u8 role) {
    u16 state = bus_config_read_word(path, spot, role, BUS_REG_STATUS);
    if (!(state & 0x10)) return 0;

    u8 walker = bus_config_read_byte(path, spot, role, BUS_REG_CAP_PTR);
    while (walker != 0) {
        u8 current = bus_config_read_byte(path, spot, role, walker);
        if (current == KEY_PCIE) return 1;
        walker = bus_config_read_byte(path, spot, role, walker + 1);
    }
    return 0;
}

// get speed
pcie_link_speed_t bus_pcie_get_speed(u8 path, u8 spot, u8 role) {
    u16 state = bus_config_read_word(path, spot, role, BUS_REG_STATUS);
    if (!(state & 0x10)) return PCIE_LINK_SPEED_UNKNOWN;

    u8 walker = bus_config_read_byte(path, spot, role, BUS_REG_CAP_PTR);
    for (;;) {
        if (walker == 0) return PCIE_LINK_SPEED_UNKNOWN;
        u8 current = bus_config_read_byte(path, spot, role, walker);
        if (current == KEY_PCIE) {
            u16 info = bus_config_read_word(path, spot, role, walker + 0x12);
            u8 value = info & 0x0F;
            switch (value) {
                case 1: return PCIE_LINK_SPEED_GEN1;
                case 2: return PCIE_LINK_SPEED_GEN2;
                case 3: return PCIE_LINK_SPEED_GEN3;
                case 4: return PCIE_LINK_SPEED_GEN4;
                case 5: return PCIE_LINK_SPEED_GEN5;
                default: return PCIE_LINK_SPEED_UNKNOWN;
            }
        }
        walker = bus_config_read_byte(path, spot, role, walker + 1);
    }
}