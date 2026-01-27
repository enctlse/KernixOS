#include <kernel/shell/acsh.h>
#include <drivers/memory/mem.h>
#include <drivers/cmos/cmos.h>
#include <kernel/module/module.h>
#include <kernel/interrupts/timer/timer.h>
#include <kernel/cpu/cpu.h>
#include <string/string.h>
#include <gui/programs/terminal.h>
static u8 my_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static u32 my_ip = (192 << 24) | (168 << 16) | (1 << 8) | 1;
FHDR(cmd_modules)
{
    (void)s;
    if (*s != '\0') {
    terminal_print("Error: invalid option: ", TERM_COLOR_ERROR);
    terminal_print(s, TERM_COLOR_ERROR);
    terminal_print("\n", TERM_COLOR_ERROR);
    return;
}
    terminal_print("Loaded Modules:\n", TERM_COLOR_INFO);
    int count = count_components();
    if (count == 0) {
        terminal_print("No modules loaded\n", TERM_COLOR_ERROR);
        return;
    }
    char buf[128];
    for (int i = 0; i < count; i++) {
        struct component_handler *handler = component_at_position(i);
        if (handler) {
            terminal_print(handler->attachment_point, TERM_COLOR_INFO);
            u32 ver = handler->build_number;
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
            terminal_print(buf, TERM_COLOR_INFO);
            terminal_print("\n", TERM_COLOR_INFO);
        }
    }
    str_copy(buf, "\nTotal: ");
    str_append_uint(buf, count);
    str_append(buf, " module(s)");
    terminal_print(buf, TERM_COLOR_INFO);
}
void print_res() {
    char buf[128];
    str_copy(buf, "");
    str_append_uint(buf, get_fb_width());
    str_append(buf, "x");
    str_append_uint(buf, get_fb_height());
    str_append(buf, "\n");
    print(buf, gray_70);
}
void ShowCPUName(){
    const char *cpu_name = cpu_get_brand();
    if (cpu_name[0]) {
        print(cpu_name, gray_70);
    } else {
        print("Unknown CPU", gray_70);
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
FHDR(cmd_sysinfo)
{
    (void)s;
     if (*s != '\0') {
    print("Error: invalid option: ", red);
    print(s, red);
    print("\n", red);
    return;
    }
    print(" 			   ..,oONMMMMMMMMMMN0o,..\n", red);
    print("			  ..lKMMMMMMMMMMMMMMMMMMKl..        ", red);        print(Host, green); print("@", green); print(User, green); print("\n", green);
    print("		     .lNMMMMMMMMMMMMMMMMMMMMMMNl..      ", red);           print("OS: ", green); print("KernixOS-64bit\n", gray_70);
    print("		   ..xMMMMMMMMMMMMMMMMMMMMMMMMMMk..     ", red);           print("Kernel: ", green); print("AC-0099\n", gray_70);
    print("	       .oMMMMMMMMMMMMMMMMMMMMMMMMMMMMo.     ",red);                 print("Shell: ", green); print("acsh 1.0.0.0\n", gray_70);
    print("	      ..NMMMMMMMMMMMMMMMMMMMMMMMMMMMMN..    ",red);                 print("Resolution: ", green); print_res();
    print("          .,MMMMMMMMMMMMMMMMMMMMMMMMMMMMMM,.    ",red);              print("Bootloader: ", green); print("Limine \n", gray_70); 
    print("          ..KMMMMMMMMMMMMMMMMMMMMMMMMMMMM0..    ",red);              print("CPU: ", green); ShowCPUName(); print("\n", gray_70); 
    print("           .;WMMMMMMMMMMMMMMMMMMMMMMMMMMN,.     ", red);             print("Date: ", green); GetCMOSDate(); print("\n", gray_70);    
    print("            .:NMMMMMMMMMMMMMMMMMMMMMMMMN;.      ", red);                 print("Uptime: ", green); timer_display_uptime_information(); print("\n", gray_70);
    print("             .'kWMMMMMMMMMMMMMMMMMMMMWx..\n", red);
    print("               .'dNMMMMMMMMMMMMMMMMXd..\n", red);
    print("                 ..,lkKXWMMMMWNKkl,..\n", red);
        print("R ", red);
        print("G ", green);
        print("B ", blue);
        print("Y ", yellow);
        print("M ", purple);
        print("C ", cyan);
        print("W ", gray_70);
        print("K \n", black);
    print("                        ", green);
}
FHDR(cmd_cpuinfo)
{
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", red);
    print(s, red);
    print("\n", red);
    return;
    }
    cpu_info_t *info = cpu_get_info();
    char buf[128];
    print("CPU Information:\n", cyan);
    print("Vendor: ", green);
    print(info->vendor, gray_70);
    print("\n", gray_70);
    print("Brand: ", green);
    print(info->brand[0] ? info->brand : "Unknown", gray_70);
    print("\n", gray_70);
    print("Family: ", green);
    str_copy(buf, "");
    str_append_uint(buf, info->family);
    print(buf, gray_70);
    print("\n", gray_70);
    print("Model: ", green);
    str_copy(buf, "");
    str_append_uint(buf, info->model);
    print(buf, gray_70);
    print("\n", gray_70);
    print("Stepping: ", green);
    str_copy(buf, "");
    str_append_uint(buf, info->stepping);
    print(buf, gray_70);
    print("\n", gray_70);
    print("Cores: ", green);
    str_copy(buf, "");
    str_append_uint(buf, info->cores);
    print(buf, gray_70);
    print("\n", gray_70);
    print("Threads: ", green);
    str_copy(buf, "");
    str_append_uint(buf, info->threads);
    print(buf, gray_70);
    print("\n", gray_70);
    print("Cache Line Size: ", green);
    str_copy(buf, "");
    str_append_uint(buf, info->cache_line_size);
    str_append(buf, " bytes");
    print(buf, gray_70);
    print("\n", gray_70);
    if (info->cache_l1d) {
        print("L1 Data Cache: ", green);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l1d);
        str_append(buf, " KB");
        print(buf, gray_70);
        print("\n", gray_70);
    }
    if (info->cache_l1i) {
        print("L1 Instruction Cache: ", green);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l1i);
        str_append(buf, " KB");
        print(buf, gray_70);
        print("\n", gray_70);
    }
    if (info->cache_l2) {
        print("L2 Cache: ", green);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l2);
        str_append(buf, " KB");
        print(buf, gray_70);
        print("\n", gray_70);
    }
    if (info->cache_l3) {
        print("L3 Cache: ", green);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l3);
        str_append(buf, " KB");
        print(buf, gray_70);
        print("\n", gray_70);
    }
    print("Features (EDX): ", green);
print_hex(info->features_edx, gray_70);
print("\n", gray_70);
print("Features (ECX): ", green);
print_hex(info->features_ecx, gray_70);
print("\n", gray_70);
print("Extended Features (EBX): ", green);
print_hex(info->extended_features_ebx, gray_70);
print("\n", gray_70);
print("Extended Features (ECX): ", green);
print_hex(info->extended_features_ecx, gray_70);
print("\n", gray_70);
}