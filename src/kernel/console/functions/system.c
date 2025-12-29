#include <kernel/console/console.h>
#include <memory/main.h>
#include <drivers/cmos/cmos.h>
#include <kernel/module/module.h>
#include <kernel/exceptions/timer.h>
#include <kernel/cpu/cpu.h>
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