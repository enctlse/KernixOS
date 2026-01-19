#include "network.h"
#include <drivers/ethernet/e1000.h>
#include <string/string.h>
#include <kernel/communication/serial.h>
#include <gui/programs/terminal.h>
#include <drivers/pci/pci_core.h>
extern u32 parse_ip(const char *str);
network_config_t net_config = {0};
void network_init() {
    int pci_count = bus_pci_device_total();
    serial_puts("PCI count: ");
    char buf[16];
    str_copy(buf, "");
    str_append_uint(buf, pci_count);
    serial_puts(buf);
    serial_puts("\n");
    int found = 0;
    if (pci_count > 100) pci_count = 100;
    for (int i = 0; i < pci_count; i++) {
        pci_device_t* dev = bus_pci_device_at(i);
        if (dev) {
            serial_puts("PCI dev ");
            str_copy(buf, "");
            str_append_uint(buf, i);
            serial_puts(buf);
            serial_puts(": vendor 0x");
            str_copy(buf, "");
            str_append_uint(buf, dev->manufacturer_id);
            serial_puts(buf);
            serial_puts(" device 0x");
            str_copy(buf, "");
            str_append_uint(buf, dev->product_id);
            serial_puts(buf);
            serial_puts("\n");
            if (dev->manufacturer_id == 0x8086 && (dev->product_id == 0x100E || dev->product_id == 0x100F || dev->product_id == 0x1011 || dev->product_id == 0x1010 || dev->product_id == 0x1004 || dev->product_id == 0x1000)) {
                uint32_t io_base = dev->base_addresses[0] & ~0xF;
                serial_puts("Found e1000, IO base 0x");
                str_copy(buf, "");
                str_append_uint(buf, io_base);
                serial_puts(buf);
                serial_puts("\n");
                if (io_base == 0) continue;
                e1000_init(io_base);
                if ((e1000_dev.mac[0] | e1000_dev.mac[1] | e1000_dev.mac[2] | e1000_dev.mac[3] | e1000_dev.mac[4] | e1000_dev.mac[5]) != 0) {
                    for (int j = 0; j < 6; j++) net_config.mac[j] = e1000_dev.mac[j];
                    net_config.ip = 0xC0A80101;
                    net_config.gateway = 0xC0A80101;
                    net_config.subnet = 0xFFFFFF00;
                    net_config.dns = 0x08080808;
                    found = 1;
                    serial_puts("Network initialized\n");
                    break;
                } else {
                    serial_puts("MAC invalid\n");
                }
            }
        } else {
            serial_puts("PCI dev NULL\n");
        }
    }
    if (!found) {
        serial_puts("No e1000 device found\n");
    }
}
void network_send_packet(void* data, int len, uint8_t* dest_mac, uint16_t ethertype) {
    uint8_t buffer[2048];
    ethernet_header_t* eth = (ethernet_header_t*)buffer;
    for (int i = 0; i < 6; i++) eth->dest_mac[i] = dest_mac[i];
    for (int i = 0; i < 6; i++) eth->src_mac[i] = net_config.mac[i];
    eth->ethertype = ethertype;
    for (int i = 0; i < len; i++) buffer[sizeof(ethernet_header_t) + i] = ((uint8_t*)data)[i];
    e1000_send_packet(buffer, len + sizeof(ethernet_header_t));
}
int network_receive_packet(void* buffer, int max_len) {
    return e1000_receive_packet(buffer, max_len);
}
uint16_t network_checksum(void* data, int len) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)data;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len) sum += *(uint8_t*)ptr;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}
void network_handle_packet(void* packet, int len) {
    ethernet_header_t* eth = (ethernet_header_t*)packet;
    if (eth->ethertype == 0x0806) {
        arp_header_t* arp = (arp_header_t*)(packet + sizeof(ethernet_header_t));
        if (arp->oper == 0x0100) {
            if (arp->tpa == net_config.ip) {
                arp->oper = 0x0200;
                for (int i = 0; i < 6; i++) arp->tha[i] = arp->sha[i];
                arp->tpa = arp->spa;
                for (int i = 0; i < 6; i++) arp->sha[i] = net_config.mac[i];
                arp->spa = net_config.ip;
                network_send_packet(arp, sizeof(arp_header_t), eth->src_mac, 0x0806);
            }
        } else if (arp->oper == 0x0200) {
            for (int i = 0; i < 16; i++) {
                if (!net_config.arp_table[i].valid || net_config.arp_table[i].ip == arp->spa) {
                    net_config.arp_table[i].ip = arp->spa;
                    for (int j = 0; j < 6; j++) net_config.arp_table[i].mac[j] = arp->sha[j];
                    net_config.arp_table[i].valid = 1;
                    break;
                }
            }
        }
    } else if (eth->ethertype == 0x0800) {
        ipv4_header_t* ip = (ipv4_header_t*)(packet + sizeof(ethernet_header_t));
        if (ip->dest_ip == net_config.ip || ip->dest_ip == 0xFFFFFFFF) {
            if (ip->protocol == 1) {
                icmp_header_t* icmp = (icmp_header_t*)((uint8_t*)ip + (ip->version_ihl & 0xF) * 4);
                if (icmp->type == 8) {
                    icmp->type = 0;
                    icmp->checksum = 0;
                    icmp->checksum = network_checksum(icmp, len - sizeof(ethernet_header_t) - (ip->version_ihl & 0xF) * 4);
                    uint32_t temp = ip->src_ip;
                    ip->src_ip = ip->dest_ip;
                    ip->dest_ip = temp;
                    ip->checksum = 0;
                    ip->checksum = network_checksum(ip, (ip->version_ihl & 0xF) * 4);
                    network_send_packet(ip, len - sizeof(ethernet_header_t), eth->src_mac, 0x0800);
                } else if (icmp->type == 0) {
                    terminal_print("ICMP echo reply received\n", TERM_COLOR_SUCCESS);
                }
            }
        }
    }
}
void arp_request(uint32_t target_ip) {
    uint8_t buffer[2048];
    arp_header_t* arp = (arp_header_t*)buffer;
    arp->htype = 0x0100;
    arp->ptype = 0x0008;
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = 0x0100;
    for (int i = 0; i < 6; i++) arp->sha[i] = net_config.mac[i];
    arp->spa = net_config.ip;
    for (int i = 0; i < 6; i++) arp->tha[i] = 0xFF;
    arp->tpa = target_ip;
    network_send_packet(arp, sizeof(arp_header_t), (uint8_t*)"\xff\xff\xff\xff\xff\xff", 0x0806);
}
uint8_t* arp_lookup(uint32_t ip) {
    for (int i = 0; i < 16; i++) {
        if (net_config.arp_table[i].valid && net_config.arp_table[i].ip == ip) {
            return net_config.arp_table[i].mac;
        }
    }
    return NULL;
}
void cmd_ping(const char *s) {
    if (!s || *s == '\0') {
        terminal_print("ping: missing host\n", TERM_COLOR_ERROR);
        return;
    }
    uint32_t target_ip = parse_ip(s);
    if (target_ip == 0) {
        terminal_print("Invalid IP address\n", TERM_COLOR_ERROR);
        return;
    }
    terminal_print("Pinging ", TERM_COLOR_DEFAULT);
    terminal_print(s, TERM_COLOR_DEFAULT);
    terminal_print("...\n", TERM_COLOR_DEFAULT);
    uint8_t* dest_mac = arp_lookup(target_ip);
    if (!dest_mac) {
        arp_request(target_ip);
        for (volatile int i = 0; i < 1000000; i++);
        dest_mac = arp_lookup(target_ip);
        if (!dest_mac) {
            terminal_print("ARP failed, using broadcast\n", TERM_COLOR_DEFAULT);
            dest_mac = (uint8_t*)"\xff\xff\xff\xff\xff\xff";
        }
    }
    uint8_t buffer[2048];
    ipv4_header_t* ip = (ipv4_header_t*)buffer;
    icmp_header_t* icmp = (icmp_header_t*)(buffer + sizeof(ipv4_header_t));
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = sizeof(ipv4_header_t) + sizeof(icmp_header_t) + 32;
    ip->id = 0x1234;
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = 1;
    ip->src_ip = net_config.ip;
    ip->dest_ip = target_ip;
    ip->checksum = 0;
    ip->checksum = network_checksum(ip, sizeof(ipv4_header_t));
    icmp->type = 8;
    icmp->code = 0;
    icmp->id = 0x1234;
    icmp->seq = 0;
    for (int i = 0; i < 32; i++) ((uint8_t*)icmp)[sizeof(icmp_header_t) + i] = i;
    icmp->checksum = 0;
    icmp->checksum = network_checksum(icmp, sizeof(icmp_header_t) + 32);
    network_send_packet(ip, ip->total_len, dest_mac, 0x0800);
    terminal_print("ICMP echo request sent\n", TERM_COLOR_SUCCESS);
}