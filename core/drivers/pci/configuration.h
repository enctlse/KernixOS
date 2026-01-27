#ifndef BUS_CONFIG_H
#define BUS_CONFIG_H
#include <outputs/types.h>
#define BUS_CFG_ADDR_PORT 0xCF8
#define BUS_CFG_DATA_PORT 0xCFC
#define BUS_REG_VENDOR_ID      0x00
#define BUS_REG_DEVICE_ID      0x02
#define BUS_REG_COMMAND        0x04
#define BUS_REG_STATUS         0x06
#define BUS_REG_REVISION_ID    0x08
#define BUS_REG_PROG_IF        0x09
#define BUS_REG_SUBCLASS       0x0A
#define BUS_REG_CLASS_CODE     0x0B
#define BUS_REG_CACHE_LINE     0x0C
#define BUS_REG_LATENCY        0x0D
#define BUS_REG_HEADER_TYPE    0x0E
#define BUS_REG_BIST           0x0F
#define BUS_REG_BAR0           0x10
#define BUS_REG_BAR1           0x14
#define BUS_REG_BAR2           0x18
#define BUS_REG_BAR3           0x1C
#define BUS_REG_BAR4           0x20
#define BUS_REG_BAR5           0x24
#define BUS_REG_CARDBUS_CIS    0x28
#define BUS_REG_SUBSYS_VENDOR  0x2C
#define BUS_REG_SUBSYS_ID      0x2E
#define BUS_REG_EXPANSION_ROM  0x30
#define BUS_REG_CAP_PTR        0x34
#define BUS_REG_INT_LINE       0x3C
#define BUS_REG_INT_PIN        0x3D
#define BUS_REG_MIN_GNT        0x3E
#define BUS_REG_MAX_LAT        0x3F
#define BUS_REG_CLASS_REV      0x08
#define BUS_REG_PRIM_BUS       0x18
#define BUS_REG_SEC_BUS        0x19
#define BUS_REG_SUB_BUS        0x1A
u32 bus_config_read_dword(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset);
void bus_config_write_dword(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset, u32 data);
u16 bus_config_read_word(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset);
u8 bus_config_read_byte(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset);
void bus_config_write_word(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset, u16 data);
void bus_config_write_byte(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset, u8 data);
#endif