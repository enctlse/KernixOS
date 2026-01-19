#include "io.h"
void write_port_byte(u16 address, u8 value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(address));
}
u8 read_port_byte(u16 address) {
    u8 result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(address));
    return result;
}
void write_port_word(u16 address, u16 value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(address));
}
u16 read_port_word(u16 address) {
    u16 result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(address));
    return result;
}
void write_port_dword(u16 address, u32 value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(address));
}
u32 read_port_dword(u16 address) {
    u32 result;
    __asm__ volatile("inl %1, %0" : "=a"(result) : "Nd"(address));
    return result;
}
void pause_io(void) {
    __asm__ volatile("outb %%al, $0x80" : : "a"(0));
}