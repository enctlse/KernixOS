#ifndef NETWORK_H
#define NETWORK_H
#include <types.h>
#define ETH_TYPE_ARP 0x0806
#define ETH_TYPE_IP  0x0800
typedef struct {
    u8 dest_mac[6];
    u8 src_mac[6];
    u16 ethertype;
    u8 payload[];
} __attribute__((packed)) ethernet_frame_t;
typedef struct {
    u16 htype;
    u16 ptype;
    u8 hlen;
    u8 plen;
    u16 oper;
    u8 sha[6];
    u8 spa[4];
    u8 tha[6];
    u8 tpa[4];
} __attribute__((packed)) arp_packet_t;
typedef struct {
    u8 version_ihl;
    u8 tos;
    u16 total_len;
    u16 id;
    u16 flags_frag;
    u8 ttl;
    u8 protocol;
    u16 checksum;
    u32 src_ip;
    u32 dest_ip;
    u8 payload[];
} __attribute__((packed)) ip_packet_t;
typedef struct {
    u8 type;
    u8 code;
    u16 checksum;
    u16 id;
    u16 seq;
    u8 data[];
} __attribute__((packed)) icmp_packet_t;
void network_init(void);
void network_send_packet(const void *data, u32 len);
void network_receive_packet(void);
int ping(const char *ip_str);
#endif