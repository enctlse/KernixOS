#include <kernel/console/console.h>
#include <memory/main.h>
#include <drivers/cmos/cmos.h>
#include <kernel/module/module.h>
#include <kernel/exceptions/timer.h>
#include <kernel/cpu/cpu.h>
#include <network/network.h>
#include <drivers/ethernet/e1000.h>
#include <string/string.h>
static u8 my_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static u32 my_ip = (192 << 24) | (168 << 16) | (1 << 8) | 1;
FHDR(cmd_modules)
{
    (void)s;
    print("Loaded Modules:\n", GFX_WHITE);
    int count = module_get_count();
    if (count == 0) {
        print("No modules loaded\n", GFX_RED);
        return;
    }
    char buf[128];
    for (int i = 0; i < count; i++) {
        driver_module *mod = module_get_by_index(i);
        if (mod) {
            print(mod->mount, GFX_CYAN);
            u32 ver = mod->version;
            u32 major = (ver >> 24) & 0xFF;
            u32 minor = (ver >> 16) & 0xFF;
            u32 patch = (ver >> 8) & 0xFF;
            str_copy(buf, " [v");
            str_append_uint(buf, major);
            str_append(buf, ".");
            str_append_uint(buf, minor);
            str_append(buf, ".");
            str_append_uint(buf, patch);
            str_append(buf, "]");
            print(buf, GFX_GRAY_50);
            print("\n", GFX_WHITE);
        }
    }
    str_copy(buf, "\nTotal: ");
    str_append_uint(buf, count);
    str_append(buf, " module(s)");
    print(buf, GFX_WHITE);
}
void print_res() {
    char buf[128];
    str_copy(buf, "");
    str_append_uint(buf, get_fb_width());
    str_append(buf, "x");
    str_append_uint(buf, get_fb_height());
    str_append(buf, "\n");
    print(buf, GFX_WHITE);
}
void ShowCPUName(){
    const char *cpu_name = cpu_get_brand();
    if (cpu_name[0]) {
        print(cpu_name, GFX_WHITE);
    } else {
        print("Unknown CPU", GFX_WHITE);
    }
}
u32 parse_ip(const char *str) {
    u32 ip = 0;
    int part = 0;
    int shift = 24;
    const char *p = str;
    while (*p && shift >= 0) {
        if (*p >= '0' && *p <= '9') {
            part = part * 10 + (*p - '0');
        } else if (*p == '.') {
            ip |= (part << shift);
            part = 0;
            shift -= 8;
        } else {
            return 0;
        }
        p++;
    }
    ip |= (part << shift);
    return ip;
}
u16 checksum(const void *data, u32 len) {
    u32 sum = 0;
    const u16 *ptr = data;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len) sum += *(u8*)ptr;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}
void network_send_icmp_echo(u32 target_ip) {
    u8 buffer[sizeof(ethernet_frame_t) + sizeof(ip_packet_t) + sizeof(icmp_packet_t) + 32];
    ethernet_frame_t *eth = (ethernet_frame_t*)buffer;
    ip_packet_t *ip = (ip_packet_t*)eth->payload;
    icmp_packet_t *icmp = (icmp_packet_t*)ip->payload;
    memcpy(eth->dest_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);
    memcpy(eth->src_mac, my_mac, 6);
    eth->ethertype = 0x0800;
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = sizeof(ip_packet_t) + sizeof(icmp_packet_t) + 32;
    ip->id = 0;
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = 1;
    ip->checksum = 0;
    ip->src_ip = my_ip;
    ip->dest_ip = target_ip;
    ip->checksum = checksum(ip, sizeof(ip_packet_t));
    icmp->type = 8;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = 1;
    icmp->seq = 1;
    memset(icmp->data, 'A', 32);
    icmp->checksum = checksum(icmp, sizeof(icmp_packet_t) + 32);
    e1000_send_packet(eth, sizeof(ethernet_frame_t) + ip->total_len);
}
int ping(const char *ip_str) {
    u32 target_ip = parse_ip(ip_str);
    if (target_ip == 0) {
        print("Invalid IP address\n", GFX_RED);
        return -1;
    }
    network_send_icmp_echo(target_ip);
    return 0;
}
FHDR(cmd_sysinfo)
{
    (void)s;
    print(" 			   ..,oONMMMMMMMMMMN0o,..\n", GFX_RED);
    print("			  ..lKMMMMMMMMMMMMMMMMMMKl..        ", GFX_RED);        print(PC_NAME, GFX_GREEN); print("@", GFX_GREEN); print(USER_NAME, GFX_GREEN); print("\n", GFX_GREEN);
    print("		     .lNMMMMMMMMMMMMMMMMMMMMMMNl..      ", GFX_RED);           print("OS: ", GFX_GREEN); print("KernixOS-64bit\n", GFX_CYAN);
    print("		   ..xMMMMMMMMMMMMMMMMMMMMMMMMMMk..     ", GFX_RED);           print("Kernel: ", GFX_GREEN); print("AC-0099\n", GFX_BLUE);
    print("	       .oMMMMMMMMMMMMMMMMMMMMMMMMMMMMo.     ",GFX_RED);               print("Resolution: ", GFX_GREEN); print_res();
    print("	      ..NMMMMMMMMMMMMMMMMMMMMMMMMMMMMN..    ",GFX_RED);               print("Bootloader: ", GFX_GREEN); print("Limine \n", GFX_WHITE);
    print("          .,MMMMMMMMMMMMMMMMMMMMMMMMMMMMMM,.    ",GFX_RED);               print("CPU: ", GFX_GREEN); ShowCPUName(); print("\n", GFX_WHITE);
    print("          ..KMMMMMMMMMMMMMMMMMMMMMMMMMMMM0..    ",GFX_RED);               print("Date: ", GFX_GREEN); GetCMOSDate(); print("\n", GFX_WHITE);
    print("           .;WMMMMMMMMMMMMMMMMMMMMMMMMMMN,.     ", GFX_RED);                 print("Uptime: ", GFX_GREEN); timer_print_uptime(); print("\n", GFX_WHITE);
    print("            .:NMMMMMMMMMMMMMMMMMMMMMMMMN;.\n", GFX_RED);
    print("             .'kWMMMMMMMMMMMMMMMMMMMMWx..\n", GFX_RED);
    print("               .'dNMMMMMMMMMMMMMMMMXd..\n", GFX_RED);
    print("                 ..,lkKXWMMMMWNKkl,..\n", GFX_RED);
        print("\x09 ", GFX_WHITE);
        print("\x09 ", GFX_RED);
        print("\x09 ", GFX_GREEN);
        print("\x09 ", GFX_YELLOW);
        print("\x09 ", GFX_BLUE);
        print("\x09 ", GFX_PURPLE);
        print("\x09 ", GFX_CYAN);
        print("\x09 \n", GFX_BG); 
    print("                        ", GFX_GREEN);
}
FHDR(cmd_cpuinfo)
{
    (void)s;
    cpu_info_t *info = cpu_get_info();
    char buf[128];
    print("CPU Information:\n", GFX_CYAN);
    print("Vendor: ", GFX_GREEN);
    print(info->vendor, GFX_WHITE);
    print("\n", GFX_WHITE);
    print("Brand: ", GFX_GREEN);
    print(info->brand[0] ? info->brand : "Unknown", GFX_WHITE);
    print("\n", GFX_WHITE);
    print("Family: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->family);
    print(buf, GFX_WHITE);
    print("\n", GFX_WHITE);
    print("Model: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->model);
    print(buf, GFX_WHITE);
    print("\n", GFX_WHITE);
    print("Stepping: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->stepping);
    print(buf, GFX_WHITE);
    print("\n", GFX_WHITE);
    print("Cores: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->cores);
    print(buf, GFX_WHITE);
    print("\n", GFX_WHITE);
    print("Threads: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->threads);
    print(buf, GFX_WHITE);
    print("\n", GFX_WHITE);
    print("Cache Line Size: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->cache_line_size);
    str_append(buf, " bytes");
    print(buf, GFX_WHITE);
    print("\n", GFX_WHITE);
    if (info->cache_l1d) {
        print("L1 Data Cache: ", GFX_GREEN);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l1d);
        str_append(buf, " KB");
        print(buf, GFX_WHITE);
        print("\n", GFX_WHITE);
    }
    if (info->cache_l1i) {
        print("L1 Instruction Cache: ", GFX_GREEN);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l1i);
        str_append(buf, " KB");
        print(buf, GFX_WHITE);
        print("\n", GFX_WHITE);
    }
    if (info->cache_l2) {
        print("L2 Cache: ", GFX_GREEN);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l2);
        str_append(buf, " KB");
        print(buf, GFX_WHITE);
        print("\n", GFX_WHITE);
    }
    if (info->cache_l3) {
        print("L3 Cache: ", GFX_GREEN);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l3);
        str_append(buf, " KB");
        print(buf, GFX_WHITE);
        print("\n", GFX_WHITE);
    }
    print("Features (EDX): ", GFX_GREEN);
print_hex(info->features_edx, GFX_WHITE);
print("\n", GFX_WHITE);
print("Features (ECX): ", GFX_GREEN);
print_hex(info->features_ecx, GFX_WHITE);
print("\n", GFX_WHITE);
print("Extended Features (EBX): ", GFX_GREEN);
print_hex(info->extended_features_ebx, GFX_WHITE);
print("\n", GFX_WHITE);
print("Extended Features (ECX): ", GFX_GREEN);
print_hex(info->extended_features_ecx, GFX_WHITE);
print("\n", GFX_WHITE);
}
FHDR(cmd_ping)
{
    if (!s || !*s) {
        print("Usage: ping <ip>\n", GFX_RED);
        return;
    }
    u32 target_ip = parse_ip(s);
    if (target_ip == 0) {
        print("Invalid IP address\n", GFX_RED);
        return;
    }
    print("Pinging ", GFX_WHITE);
    print(s, GFX_CYAN);
    print("...\n", GFX_WHITE);
    network_send_icmp_echo(target_ip);
    u32 timeout = 5000;
    u32 start_time = 0;
    while (timeout > 0) {
        u8 buffer[2048];
        u32 len;
        e1000_receive_packet(buffer, &len);
        if (len > 0) {
            ethernet_frame_t *eth = (ethernet_frame_t*)buffer;
            if (eth->ethertype == 0x0800) {
                ip_packet_t *ip = (ip_packet_t*)eth->payload;
                if (ip->protocol == 1 && ip->dest_ip == my_ip) {
                    icmp_packet_t *icmp = (icmp_packet_t*)ip->payload;
                    if (icmp->type == 0 && icmp->id == 1) {
                        print("Reply from ", GFX_GREEN);
                        print(s, GFX_CYAN);
                        print(": bytes=32\n", GFX_WHITE);
                        return;
                    }
                }
            }
        }
        for (volatile int i = 0; i < 100000; i++);
        timeout--;
    }
    print("Request timeout\n", GFX_RED);
}