#include <kernel/console/console.h>
#include <memory/main.h>
#include <drivers/cmos/cmos.h>
#include <kernel/module/module.h>
#include <kernel/exceptions/timer.h>
#include <kernel/cpu/cpu.h>
#include <string/string.h>
static u8 my_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static u32 my_ip = (192 << 24) | (168 << 16) | (1 << 8) | 1;
FHDR(cmd_modules)
{
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
}
    print("Loaded Modules:\n", GFX_GRAY_70);
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
            print("\n", GFX_GRAY_70);
        }
    }
    str_copy(buf, "\nTotal: ");
    str_append_uint(buf, count);
    str_append(buf, " module(s)");
    print(buf, GFX_GRAY_70);
}
void print_res() {
    char buf[128];
    str_copy(buf, "");
    str_append_uint(buf, get_fb_width());
    str_append(buf, "x");
    str_append_uint(buf, get_fb_height());
    str_append(buf, "\n");
    print(buf, GFX_GRAY_70);
}
void ShowCPUName(){
    const char *cpu_name = cpu_get_brand();
    if (cpu_name[0]) {
        print(cpu_name, GFX_GRAY_70);
    } else {
        print("Unknown CPU", GFX_GRAY_70);
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
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
    }
    print(" 			   ..,oONMMMMMMMMMMN0o,..\n", GFX_RED);
    print("			  ..lKMMMMMMMMMMMMMMMMMMKl..        ", GFX_RED);        print(PC_NAME, GFX_GREEN); print("@", GFX_GREEN); print(USER_NAME, GFX_GREEN); print("\n", GFX_GREEN);
    print("		     .lNMMMMMMMMMMMMMMMMMMMMMMNl..      ", GFX_RED);           print("OS: ", GFX_GREEN); print("KernixOS-64bit\n", GFX_GRAY_70);
    print("		   ..xMMMMMMMMMMMMMMMMMMMMMMMMMMk..     ", GFX_RED);           print("Kernel: ", GFX_GREEN); print("AC-0099\n", GFX_GRAY_70);
    print("	       .oMMMMMMMMMMMMMMMMMMMMMMMMMMMMo.     ",GFX_RED);                 print("Shell: ", GFX_GREEN); print("acsh 1.0.0.0\n", GFX_GRAY_70);
    print("	      ..NMMMMMMMMMMMMMMMMMMMMMMMMMMMMN..    ",GFX_RED);                 print("Resolution: ", GFX_GREEN); print_res();
    print("          .,MMMMMMMMMMMMMMMMMMMMMMMMMMMMMM,.    ",GFX_RED);              print("Bootloader: ", GFX_GREEN); print("Limine \n", GFX_GRAY_70); 
    print("          ..KMMMMMMMMMMMMMMMMMMMMMMMMMMMM0..    ",GFX_RED);              print("CPU: ", GFX_GREEN); ShowCPUName(); print("\n", GFX_GRAY_70); 
    print("           .;WMMMMMMMMMMMMMMMMMMMMMMMMMMN,.     ", GFX_RED);             print("Date: ", GFX_GREEN); GetCMOSDate(); print("\n", GFX_GRAY_70);    
    print("            .:NMMMMMMMMMMMMMMMMMMMMMMMMN;.      ", GFX_RED);                 print("Uptime: ", GFX_GREEN); timer_print_uptime(); print("\n", GFX_GRAY_70);
    print("             .'kWMMMMMMMMMMMMMMMMMMMMWx..\n", GFX_RED);
    print("               .'dNMMMMMMMMMMMMMMMMXd..\n", GFX_RED);
    print("                 ..,lkKXWMMMMWNKkl,..\n", GFX_RED);
        print("R ", GFX_RED);
        print("G ", GFX_GREEN);
        print("B ", GFX_BLUE);
        print("Y ", GFX_YELLOW);
        print("M ", GFX_PURPLE);
        print("C ", GFX_CYAN);
        print("W ", GFX_GRAY_70);
        print("K \n", GFX_BLACK);
    print("                        ", GFX_GREEN);
}
FHDR(cmd_cpuinfo)
{
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
    }
    cpu_info_t *info = cpu_get_info();
    char buf[128];
    print("CPU Information:\n", GFX_CYAN);
    print("Vendor: ", GFX_GREEN);
    print(info->vendor, GFX_GRAY_70);
    print("\n", GFX_GRAY_70);
    print("Brand: ", GFX_GREEN);
    print(info->brand[0] ? info->brand : "Unknown", GFX_GRAY_70);
    print("\n", GFX_GRAY_70);
    print("Family: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->family);
    print(buf, GFX_GRAY_70);
    print("\n", GFX_GRAY_70);
    print("Model: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->model);
    print(buf, GFX_GRAY_70);
    print("\n", GFX_GRAY_70);
    print("Stepping: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->stepping);
    print(buf, GFX_GRAY_70);
    print("\n", GFX_GRAY_70);
    print("Cores: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->cores);
    print(buf, GFX_GRAY_70);
    print("\n", GFX_GRAY_70);
    print("Threads: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->threads);
    print(buf, GFX_GRAY_70);
    print("\n", GFX_GRAY_70);
    print("Cache Line Size: ", GFX_GREEN);
    str_copy(buf, "");
    str_append_uint(buf, info->cache_line_size);
    str_append(buf, " bytes");
    print(buf, GFX_GRAY_70);
    print("\n", GFX_GRAY_70);
    if (info->cache_l1d) {
        print("L1 Data Cache: ", GFX_GREEN);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l1d);
        str_append(buf, " KB");
        print(buf, GFX_GRAY_70);
        print("\n", GFX_GRAY_70);
    }
    if (info->cache_l1i) {
        print("L1 Instruction Cache: ", GFX_GREEN);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l1i);
        str_append(buf, " KB");
        print(buf, GFX_GRAY_70);
        print("\n", GFX_GRAY_70);
    }
    if (info->cache_l2) {
        print("L2 Cache: ", GFX_GREEN);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l2);
        str_append(buf, " KB");
        print(buf, GFX_GRAY_70);
        print("\n", GFX_GRAY_70);
    }
    if (info->cache_l3) {
        print("L3 Cache: ", GFX_GREEN);
        str_copy(buf, "");
        str_append_uint(buf, info->cache_l3);
        str_append(buf, " KB");
        print(buf, GFX_GRAY_70);
        print("\n", GFX_GRAY_70);
    }
    print("Features (EDX): ", GFX_GREEN);
print_hex(info->features_edx, GFX_GRAY_70);
print("\n", GFX_GRAY_70);
print("Features (ECX): ", GFX_GREEN);
print_hex(info->features_ecx, GFX_GRAY_70);
print("\n", GFX_GRAY_70);
print("Extended Features (EBX): ", GFX_GREEN);
print_hex(info->extended_features_ebx, GFX_GRAY_70);
print("\n", GFX_GRAY_70);
print("Extended Features (ECX): ", GFX_GREEN);
print_hex(info->extended_features_ecx, GFX_GRAY_70);
print("\n", GFX_GRAY_70);
}