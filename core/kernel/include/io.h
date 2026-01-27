#ifndef IO_PORTS_H
#define IO_PORTS_H
#include <outputs/types.h>
void write_port_byte(u16 address, u8 value);
u8 read_port_byte(u16 address);
void write_port_word(u16 address, u16 value);
u16 read_port_word(u16 address);
void write_port_dword(u16 address, u32 value);
u32 read_port_dword(u16 address);
void pause_io(void);
#define outb(port, value) write_port_byte(port, value)
#define inb(port) read_port_byte(port)
#define outw(port, value) write_port_word(port, value)
#define inw(port) read_port_word(port)
#define outl(port, value) write_port_dword(port, value)
#define inl(port) read_port_dword(port)
#define io_wait() pause_io()
#endif