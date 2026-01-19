#include "configuration.h"
#include <kernel/include/io.h>

// Чтение 32-битного слова из конфигурационного пространства PCI
u32 bus_config_read_dword(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset) {
    // Формируем адрес: бит 31 установлен, шина, устройство, функция, смещение
    u32 config_addr = (u32)((bus_num << 16) | (dev_num << 11) |
                            (func_num << 8) | (reg_offset & 0xFC) | 0x80000000);
    outl(BUS_CFG_ADDR_PORT, config_addr);
    return inl(BUS_CFG_DATA_PORT);
}

// Запись 32-битного слова в конфигурационное пространство PCI
void bus_config_write_dword(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset, u32 data) {
    u32 config_addr = (u32)((bus_num << 16) | (dev_num << 11) |
                            (func_num << 8) | (reg_offset & 0xFC) | 0x80000000);
    outl(BUS_CFG_ADDR_PORT, config_addr);
    outl(BUS_CFG_DATA_PORT, data);
}

// Чтение 16-битного слова
u16 bus_config_read_word(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset) {
    u32 full_value = bus_config_read_dword(bus_num, dev_num, func_num, reg_offset & 0xFC);
    u8 byte_offset = reg_offset & 2;
    return (u16)((full_value >> (byte_offset * 8)) & 0xFFFF);
}

// Чтение байта
u8 bus_config_read_byte(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset) {
    u32 full_value = bus_config_read_dword(bus_num, dev_num, func_num, reg_offset & 0xFC);
    u8 byte_offset = reg_offset & 3;
    return (u8)((full_value >> (byte_offset * 8)) & 0xFF);
}

// Запись 16-битного слова
void bus_config_write_word(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset, u16 data) {
    u32 current_value = bus_config_read_dword(bus_num, dev_num, func_num, reg_offset & 0xFC);
    u32 shift_amount = (reg_offset & 2) * 8;
    u32 mask = 0xFFFF << shift_amount;
    u32 updated_value = (current_value & ~mask) | ((u32)data << shift_amount);
    bus_config_write_dword(bus_num, dev_num, func_num, reg_offset & 0xFC, updated_value);
}

// Запись байта
void bus_config_write_byte(u8 bus_num, u8 dev_num, u8 func_num, u8 reg_offset, u8 data) {
    u32 current_value = bus_config_read_dword(bus_num, dev_num, func_num, reg_offset & 0xFC);
    u32 shift_amount = (reg_offset & 3) * 8;
    u32 mask = 0xFF << shift_amount;
    u32 updated_value = (current_value & ~mask) | ((u32)data << shift_amount);
    bus_config_write_dword(bus_num, dev_num, func_num, reg_offset & 0xFC, updated_value);
}