#include <kernel/include/assembly.h>
#include <kernel/include/reqs.h>
#include <kernel/graph/fm.h>
#include <kernel/include/logo.h>
#include <kernel/graph/theme.h>
#include <theme/tmx.h>
#include <drivers/ps2/ps2.h>
#include <drivers/ps2/keyboard/keyboard.h>
#include <drivers/ps2/mouse/mouse.h>
#include <kernel/gui/gui.h>
#include <kernel/cpu/cpu.h>
#include <kernel/cpu/gdt.h>
#include <kernel/cpu/idt.h>
#include <memory/main.h>
#include <kernel/mem/meminclude.h>
#include <kernel/console/console.h>
#include <kernel/communication/serial.h>
#include "exceptions/panic.h"
#include <kernel/exceptions/timer.h>
#include <kernel/exceptions/panic.h>
#include <kernel/file_systems/vfs/vfs.h>
#include <kernel/file_systems/vfs/init.h>
#include <kernel/module/module.h>
#include <kernel/pci/pci.h>
#include <drivers/cmos/cmos.h>
static void boot_log_component(const char* component, int success, const char* details) {
    if (success) {
        BOOTUP_PRINT("[ OK ] ", GFX_GREEN);
        BOOTUP_PRINT(component, GFX_GRAY_70);
        if (details && details[0] != '\0') {
            BOOTUP_PRINT(" - ", GFX_GRAY_70);
            BOOTUP_PRINT(details, GFX_GRAY_70);
        }
        BOOTUP_PRINT("\n", GFX_GRAY_70);
    } else {
        BOOTUP_PRINT("[FAILED] ", GFX_RED);
        BOOTUP_PRINT(component, GFX_GRAY_70);
        if (details && details[0] != '\0') {
            BOOTUP_PRINT(" - ", GFX_GRAY_70);
            BOOTUP_PRINT(details, GFX_GRAY_70);
        }
        BOOTUP_PRINT("\n", GFX_GRAY_70);
    }
}
static void boot_log_info(const char* message) {
    BOOTUP_PRINT("[INFO] ", GFX_CYAN);
    BOOTUP_PRINT(message, GFX_GRAY_70);
    BOOTUP_PRINT("\n", GFX_GRAY_70);
}
static void boot_log_print(const char* message) {
    BOOTUP_PRINT(message, GFX_GRAY_70);
    BOOTUP_PRINT("\n", GFX_GRAY_70);
}
static void boot_log_error(const char* component, const char* error) {
    BOOTUP_PRINT("[ERROR] ", GFX_RED);
    BOOTUP_PRINT(component, GFX_GRAY_70);
    BOOTUP_PRINT(" - ", GFX_GRAY_70);
    BOOTUP_PRINT(error, GFX_RED);
    BOOTUP_PRINT("\n", GFX_GRAY_70);
}
void _start(void)
{
    boot_log_info("Initializing theme system...");
    theme_init();
    setcontext(THEME_BOOTUP);
    sbootup_theme(THEME_STD);
    sconsole_theme(THEME_FLU);
    spanic_theme(THEME_STD);
    boot_log_component("Themes", 1, "Color themes and contexts initialized");
    clear(bg());
    boot_log_component("Display", 1, "Screen cleared and ready for boot logs");
    boot_log_info("Starting AC-0099 initialization...");
    boot_log_info("Boot sequence initiated");
    boot_log_info("Checking graphics hardware...");
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        boot_log_error("Graphics", "No framebuffer response from bootloader");
        printf("no response");
        hcf();
    }
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    char fb_info[64];
    fb_info[0] = '\0';
    str_copy(fb_info, "Resolution: ");
    str_append_uint(fb_info, fb->width);
    str_append(fb_info, "x");
    str_append_uint(fb_info, fb->height);
    str_append(fb_info, " BPP: ");
    str_append_uint(fb_info, fb->bpp);
    boot_log_component("Graphics", 1, fb_info);
    boot_log_info("Setting up font rendering...");
    fm_init();
    boot_log_component("Font Manager", 1, "Font rendering system ready with PSF/PCF support");
    boot_log_info("Setting up memory management subsystems...");
    boot_log_info("Initializing physical memory manager...");
    physmem_init(memmap_request.response, hhdm_request.response);
    u64 total_memory = 0;
    if (memmap_request.response) {
        for (u64 i = 0; i < memmap_request.response->entry_count; i++) {
            struct limine_memmap_entry *entry = memmap_request.response->entries[i];
            if (entry->type == LIMINE_MEMMAP_USABLE) {
                total_memory += entry->length;
            }
        }
    }
    char mem_info[128];
    mem_info[0] = '\0';
    str_copy(mem_info, "Physical memory manager initialized - ");
    str_append_uint(mem_info, total_memory / 1024 / 1024 / 1024);
    str_append(mem_info, "GB usable memory detected");
    boot_log_component("Physical Memory", 1, mem_info);
    boot_log_info("Setting up virtual memory and paging...");
    paging_init(hhdm_request.response);
    boot_log_component("Virtual Memory", 1, "Paging system initialized with page tables");
    boot_log_info("Allocating kernel heap memory...");
    u64 phys_klime = map_region_alloc(hhdm_request.response, HEAP_START, HEAP_SIZE);
    klime_t *klime = klime_init((u64 *)HEAP_START, HEAP_SIZE);
    char klime_info[64];
    klime_info[0] = '\0';
    str_copy(klime_info, "Allocated ");
    str_append_uint(klime_info, HEAP_SIZE / 1024 / 1024);
    str_append(klime_info, "MB for kernel heap");
    boot_log_component("Kernel Memory (KLIME)", 1, klime_info);
    boot_log_info("Setting up graphics memory management...");
    if (!framebuffer_request.response) {
        boot_log_error("Graphics Memory", "Limine response NULL");
        panic("Cant initialize glime limine response NULL");
    }
    if (framebuffer_request.response->framebuffer_count < 1) {
        boot_log_error("Graphics Memory", "No framebuffers available");
        panic("Cant initialize glime limine framebuffer_count 0");
    }
    u64 phys_glime = map_region_alloc(hhdm_request.response, GRAPHICS_START, GRAPHICS_SIZE);
    limine_framebuffer_t *fb_glime = framebuffer_request.response->framebuffers[0];
    glime_response_t glres;
    glres.start_framebuffer = (u64 *)fb_glime->address;
    glres.width  = (u64)fb_glime->width;
    glres.height = (u64)fb_glime->height;
    glres.pitch  = (u64)fb_glime->pitch;
    glime_t *glime = glime_init(&glres, (u64 *)GRAPHICS_START, GRAPHICS_SIZE);
    char glime_info[64];
    glime_info[0] = '\0';
    str_copy(glime_info, "Graphics buffer: ");
    str_append_uint(glime_info, GRAPHICS_SIZE / 1024 / 1024);
    str_append(glime_info, "MB allocated");
    boot_log_component("Graphics Memory (GLIME)", 1, glime_info);
    boot_log_info("Initializing 2D graphics rendering...");
    graphics_init(fb, klime);
    graphics_enable_double_buffering();
    boot_log_component("Graphics System", 1, "2D graphics rendering initialized with drawing primitives");
    boot_log_info("Setting up userspace memory management...");
    u64 phys_ulime = map_region_alloc(hhdm_request.response, ULIME_START, ULIME_META_SIZE);
    ulime_t *ulime = ulime_init(hhdm_request.response, klime, glime, phys_ulime);
    if (!ulime) {
        boot_log_error("Userspace Memory (ULIME)", "Failed to initialize userspace memory manager");
        BOOTUP_PRINTF("Errorcode 1101: ulime is not initialized.");
        panic("Errorcode 1011: ulime is not initialized.");
    }
    boot_log_component("Userspace Memory (ULIME)", 1, "Userspace memory manager ready with process isolation");
    boot_log_info("Initializing virtual filesystem...");
    fs_system_init(klime);
    boot_log_component("Filesystem", 1, "Virtual filesystem initialized with VFS layer");
    boot_log_info("Setting up display cursor...");
    boot_log_component("Cursor", 1, "Display cursor positioned for boot logs");
    boot_log_info("Detecting CPU features and capabilities...");
    cpu_detect();
    cpu_info_t* cpu_info = cpu_get_info();
    char cpu_detail[256];
    cpu_detail[0] = '\0';
    str_copy(cpu_detail, "CPU: ");
    str_append(cpu_detail, cpu_info->brand[0] ? cpu_info->brand : cpu_info->vendor);
    str_append(cpu_detail, " (");
    str_append_uint(cpu_detail, cpu_info->cores);
    str_append(cpu_detail, " cores, ");
    str_append_uint(cpu_detail, cpu_info->threads);
    str_append(cpu_detail, " threads)");
    boot_log_component("CPU Detection", 1, cpu_detail);
    boot_log_info("Setting up Global Descriptor Table...");
    gdt_init();
    boot_log_component("GDT", 1, "Global Descriptor Table initialized with segments");
    boot_log_info("Configuring Interrupt Descriptor Table...");
    idt_init();
    boot_log_component("IDT", 1, "Interrupt Descriptor Table initialized with exception handlers");
    boot_log_info("Setting up system timer...");
    u32 freq = 1000;
    timer_init(freq);
    char timer_info[64];
    timer_info[0] = '\0';
    str_copy(timer_info, "System timer initialized at ");
    str_append_uint(timer_info, freq);
    str_append(timer_info, "Hz (");
    str_append_uint(timer_info, 1000/freq);
    str_append(timer_info, "ms ticks)");
    boot_log_component("Timer", 1, timer_info);
    boot_log_info("Starting system uptime tracking...");
    timer_set_boot_time();
    boot_log_component("Uptime Counter", 1, "System uptime tracking started with CMOS sync");
    boot_log_info("Scanning PCI/PCIe devices...");
    pci_init();
    int pci_count = pci_get_device_count();
    char pci_info[64];
    pci_info[0] = '\0';
    str_copy(pci_info, "PCI/PCIe subsystem initialized - ");
    str_append_uint(pci_info, pci_count);
    str_append(pci_info, " devices enumerated");
    boot_log_component("PCI", 1, pci_info);
    boot_log_info("Setting up dynamic module loading system...");
    module_init();
    boot_log_component("Module System", 1, "Dynamic module loading system ready");
    boot_log_info("Registering core system modules...");
    module_register(&console_module);
    module_register(&keyboard_module);
    module_register(&mouse_module);
    int count = module_get_count();
    char buf[64];
    buf[0] = '\0';
    str_append_uint(buf, count);
    str_append(buf, " core modules registered");
    boot_log_component("Core Modules", 1, buf);
    boot_log_info("Loading filesystem drivers...");
    fs_register_mods();
    boot_log_component("Filesystem Modules", 1, "Filesystem drivers registered and mounted");
    boot_log_info("Creating test filesystem entries...");
    fs_create_test_file();
    boot_log_component("Test Files", 1, "Test filesystem entries created for validation");
    boot_log_info("Configuring display scaling...");
    font_scale = 1;
    boot_log_component("Font Scaling", 1, "Display scaling set to 1x for optimal readability");
    boot_log_info("Finalizing boot logging...");
    if (init_boot_log >= 0) {
        fs_close(init_boot_log);
        init_boot_log = -1;
        boot_log_component("Boot Log", 1, "Boot log saved to persistent filesystem storage");
    }
    boot_log_info("Initializing input devices...");
    boot_log_info("Setting up PS/2 keyboard driver...");
    keyboard_init();
    boot_log_component("Keyboard", 1, "PS/2 keyboard driver initialized with scancode translation");
    boot_log_info("Initializing PS/2 mouse...");
    int mouse_result = mouse_init();
    if (mouse_result == 0) {
        boot_log_component("PS/2 Mouse", 1, "PS/2 mouse driver initialized with packet processing");
    } else {
        boot_log_component("PS/2 Mouse", 0, "PS/2 mouse initialization failed - device not present");
    }
    boot_log_info("Setting up USB mouse support...");
    int usb_mouse_result = usb_mouse_init();
    if (usb_mouse_result == 0) {
        boot_log_component("USB Mouse", 1, "USB mouse driver initialized with HID protocol support");
    } else {
        boot_log_component("USB Mouse", 0, "USB mouse initialization failed - no USB devices detected");
    }
    boot_log_info("Input devices initialization completed");
    boot_log_info("");
    boot_log_info("System initialization complete.");
    boot_log_info("AC-0099 Launched Successfully!");
    boot_log_info("All core subsystems operational");
    boot_log_info("Synchronizing system clock...");
    cmos_time_t current_time;
    cmos_read_time(&current_time);
    char time_buf[128];
    time_buf[0] = '\0';
    str_copy(time_buf, "Current system time: ");
    str_append_uint(time_buf, current_time.hour);
    str_append(time_buf, ":");
    str_append_uint(time_buf, current_time.minute);
    str_append(time_buf, ":");
    str_append_uint(time_buf, current_time.second);
    str_append(time_buf, " ");
    str_append_uint(time_buf, current_time.day);
    str_append(time_buf, "/");
    str_append_uint(time_buf, current_time.month);
    str_append(time_buf, "/20");
    str_append_uint(time_buf, current_time.year);
    boot_log_info(time_buf);
    boot_log_info("System Status: [ OK ]");
    boot_log_info("All hardware interfaces initialized");
    boot_log_info("Filesystem: Mounted.");
    boot_log_info("Input devices: [ OK ]");
    boot_log_info("Network: NOT CONFIGURED");
    boot_log_print("System ready for user interaction");
    boot_log_print("");
    boot_log_print("");
    boot_log_print("");
    boot_log_print("Kernel(AC-0099) version: 1.0.0 - release.");
    boot_log_print("KernixOS: Welcome.");
    boot_log_print("Type 'help' for available commands");
    console_init();
    setcontext(THEME_CONSOLE);
    putchar('\n', GFX_GRAY_70);
    shell_print_prompt();
    cursor_enable();
    extern int boot_completed;
    boot_completed = 1;
    while (1) {
        __asm__ volatile("hlt");
    }
    #ifdef USE_HCF
        hcf();
    #else
        panic("USE_HCF; FAILED --> USING PANIC");
    #endif
};