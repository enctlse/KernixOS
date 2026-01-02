#ifndef E1000_H
#define E1000_H
#include <types.h>
#define E1000_VENDOR_ID 0x8086
#define E1000_DEVICE_ID 0x100E
void e1000_init(void);
void e1000_send_packet(const void *data, u32 len);
void e1000_receive_packet(void *buffer, u32 *len);
#endif