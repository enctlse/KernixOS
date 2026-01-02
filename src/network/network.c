#include "network.h"
#include <drivers/ethernet/e1000.h>
#include <string/string.h>
#include <memory/main.h>
static u8 my_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static u32 my_ip = (192 << 24) | (168 << 16) | (1 << 8) | 1;
void network_init(void) {
}
void network_send_packet(const void *data, u32 len) {
    e1000_send_packet(data, len);
}
void network_receive_packet(void) {
    u8 buffer[2048];
    u32 len;
    e1000_receive_packet(buffer, &len);
}