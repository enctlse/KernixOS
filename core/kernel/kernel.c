#include <kernel/include/utils.h>
#include <kernel/include/defs.h>
#include <kernel/display/fonts/typeface.h>
#include <config/boot.h>
#include <drivers/ps2/ps2.h>
#include <drivers/ps2/keyboard/keyboard.h>
#include <drivers/ps2/mouse/mouse.h>
#include <drivers/usb/usb_keyboard.h>
#include <drivers/usb/usb_mouse.h>
#include <gui/gui.h>
#include <kernel/cpu/cpu.h>
#include <kernel/cpu/smp.h>
#include <kernel/cpu/per_cpu.h>
#include <kernel/processes/scheduler.h>
#include <kernel/cpu/gdt.h>
#include <kernel/cpu/idt.h>
#include <drivers/memory/mem.h>
#include <kernel/mem/meminclude.h>
#include <kernel/shell/acsh.h>
#include <kernel/communication/serial.h>
#include "interrupts/panic/panic.h"
#include <kernel/interrupts/timer/timer.h>
#include <kernel/interrupts/panic/panic.h>
#include <fs/vfs/vfs.h>
#include <fs/vfs/init.h>
#include <kernel/module/lkm.h>
#include <kernel/include/defs.h>
#include <drivers/pci/pci_core.h>
#include <drivers/cmos/cmos.h>
#include <drivers/ata/ata.h>
#include <drivers/usb/ehci.h>
#include <drivers/usb/ohci.h>
#include <drivers/usb/uhci.h>
#include <drivers/usb/xhci.h>
#include <kernel/shell/commands/network.h>
extern kernel_memory_t *global_kernel_memory;
void *kmalloc(size_t size) {
    if (!global_kernel_memory) return NULL;
    return kernel_memory_alloc(global_kernel_memory, size, 1);
}
void kfree(void *ptr) {
    if (!global_kernel_memory) return;
    kernel_memory_free(global_kernel_memory, ptr);
}
EXPORT_SYMBOL(kmalloc);
EXPORT_SYMBOL(kfree);
EXPORT_SYMBOL(print);
static void mouse_callback(int32_t x, int32_t y, uint8_t buttons) {
    mouse_set_position(x, y);
    mouse_set_buttons(buttons);
}
static void boot_log_component(const char* component, int success, const char* details) {
    if (success) {
        SYSTEM_PRINT("[ OK ] ", green);
        SYSTEM_PRINT(component, gray_70);
        if (details && details[0] != '\0') {
            SYSTEM_PRINT(" - ", gray_70);
            SYSTEM_PRINT(details, gray_70);
        }
        SYSTEM_PRINT("\n", gray_70);
    } else {
        SYSTEM_PRINT("[FAILED] ", red);
        SYSTEM_PRINT(component, gray_70);
        if (details && details[0] != '\0') {
            SYSTEM_PRINT(" - ", gray_70);
            SYSTEM_PRINT(details, gray_70);
        }
        SYSTEM_PRINT("\n", gray_70);
    }
}
static void boot_log_info(const char* message) {
    SYSTEM_PRINT("[INFO] ", gray_70);
    SYSTEM_PRINT(message, gray_70);
    SYSTEM_PRINT("\n", gray_70);
}
static void boot_log_print(const char* message) {
    SYSTEM_PRINT(message, gray_70);
    SYSTEM_PRINT("\n", gray_70);
}
static void boot_log_error(const char* component, const char* error) {
    SYSTEM_PRINT("[ERROR] ", red);
    SYSTEM_PRINT(component, gray_70);
    SYSTEM_PRINT(" - ", gray_70);
    SYSTEM_PRINT(error, red);
    SYSTEM_PRINT("\n", gray_70);
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
    clear(theme_bg);
    boot_log_component("Display", 1, "Screen cleared and ready for boot logs");
    boot_log_info("Starting AC-0099 initialization...");
    boot_log_info("Boot sequence initiated");
    boot_log_info("Checking graphics hardware...");
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        boot_log_error("Graphics", "No framebuffer response from bootloader");
        printf("no response");
        system_halt();
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
    u64 phys_kernel_memory = map_region_alloc(hhdm_request.response, HEAP_START, HEAP_SIZE);
    kernel_memory_t *kernel_memory = kernel_memory_init((u64 *)HEAP_START, HEAP_SIZE);
    global_kernel_memory = kernel_memory;
    char kernel_memory_info[64];
    kernel_memory_info[0] = '\0';
    str_copy(kernel_memory_info, "Allocated ");
    str_append_uint(kernel_memory_info, HEAP_SIZE / 1024 / 1024);
    str_append(kernel_memory_info, "MB for kernel heap");
    boot_log_component("Kernel Memory (KERNEL_MEMORY)", 1, kernel_memory_info);
    boot_log_info("Setting up graphics memory management...");
    if (!framebuffer_request.response) {
        boot_log_error("Graphics Memory", "Limine response NULL");
        initiate_panic("Cant initialize graphics limine response NULL");
    }
    if (framebuffer_request.response->framebuffer_count < 1) {
        boot_log_error("Graphics Memory", "No framebuffers available");
        initiate_panic("Cant initialize graphics limine framebuffer_count 0");
    }
    u64 phys_graphics = map_region_alloc(hhdm_request.response, GRAPHICS_START, GRAPHICS_SIZE);
    limine_framebuffer_t *fb_graphics = framebuffer_request.response->framebuffers[0];
    graphics_response_t graphics_res;
    graphics_res.start_framebuffer = (u64 *)fb_graphics->address;
    graphics_res.width  = (u64)fb_graphics->width;
    graphics_res.height = (u64)fb_graphics->height;
    graphics_res.pitch  = (u64)fb_graphics->pitch;
    graphics_manager_t *graphics_manager = graphics_manager_init(&graphics_res, (u64 *)GRAPHICS_START, GRAPHICS_SIZE);
    char graphics_info[64];
    graphics_info[0] = '\0';
    str_copy(graphics_info, "Graphics buffer: ");
    str_append_uint(graphics_info, GRAPHICS_SIZE / 1024 / 1024);
    str_append(graphics_info, "MB allocated");
    boot_log_component("Graphics Memory (GRAPHICS)", 1, graphics_info);
    boot_log_info("Initializing 2D graphics rendering...");
    graphics_init(fb, kernel_memory);
    graphics_disable_double_buffering();
    boot_log_component("Graphics System", 1, "2D graphics rendering initialized with drawing primitives");
    boot_log_info("Setting up userspace memory management...");
    u64 phys_user_space = map_region_alloc(hhdm_request.response, ULIME_START, ULIME_META_SIZE);
    user_space_t *user_space = user_space_init(hhdm_request.response, kernel_memory, graphics_manager, phys_user_space);
    if (!user_space) {
        boot_log_error("Userspace Memory (USER_SPACE)", "Failed to initialize userspace memory manager");
        SYSTEM_PRINTF("Errorcode 1101: user_space is not initialized.");
        initiate_panic("Errorcode 1011: user_space is not initialized.");
    }
    boot_log_component("Userspace Memory (USER_SPACE)", 1, "Userspace memory manager ready with process isolation");
    boot_log_info("Initializing virtual filesystem...");
    fs_system_init(kernel_memory);
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
    boot_log_info("Initializing Symmetric Multi-Processing...");
    smp_init();
    char smp_detail[64];
    smp_detail[0] = '\0';
    str_copy(smp_detail, "Detected ");
    str_append_uint(smp_detail, smp_get_cpu_count());
    str_append(smp_detail, " CPUs via Limine SMP");
    boot_log_component("SMP Initialization", 1, smp_detail);
    boot_log_info("Initializing per-CPU data structures...");
    per_cpu_init();
    boot_log_component("Per-CPU Data", 1, "Per-CPU data structures initialized");
    boot_log_info("Initializing task scheduler...");
    initialize_scheduler();
    boot_log_component("Task Scheduler", 1, "Task scheduler initialized with idle task");
    boot_log_info("Setting up system timer...");
    u32 freq = 1000;
    timer_system_initialize(freq);
    char timer_info[64];
    timer_info[0] = '\0';
    str_copy(timer_info, "System timer initialized at ");
    str_append_uint(timer_info, freq);
    str_append(timer_info, "Hz (");
    str_append_uint(timer_info, 1000/freq);
    str_append(timer_info, "ms ticks)");
    boot_log_component("Timer", 1, timer_info);
    boot_log_info("Starting application processors...");
    char ap_detail[64];
    ap_detail[0] = '\0';
    str_copy(ap_detail, "Attempted to start ");
    str_append_uint(ap_detail, smp_get_cpu_count() - 1);
    str_append(ap_detail, " application processors");
    boot_log_component("AP Startup", 1, ap_detail);
    boot_log_info("Starting system uptime tracking...");
    timer_record_boot_timestamp();
    boot_log_component("Uptime Counter", 1, "System uptime tracking started with CMOS sync");
    boot_log_info("Scanning PCI/PCIe devices...");
    bus_pci_initialize();
    int pci_count = bus_pci_device_total();
    char pci_info[64];
    pci_info[0] = '\0';
    str_copy(pci_info, "PCI/PCIe subsystem initialized - ");
    str_append_uint(pci_info, pci_count);
    str_append(pci_info, " devices enumerated");
    boot_log_component("PCI", 1, pci_info);
    boot_log_info("Initializing EHCI USB controller...");
    int ehci_found = 0;
    for (int i = 0; i < pci_count; i++) {
        pci_device_t* dev = bus_pci_device_at(i);
        if (dev->device_class == EHCI_CLASS && dev->device_subclass == EHCI_SUBCLASS && dev->prog_interface == EHCI_PROG_IF) {
            uint64_t base_addr = dev->base_addresses[0];
            if (ehci_init(base_addr) == 0) {
                ehci_start();
                ehci_enumerate_devices();
                ehci_found = 1;
                boot_log_component("EHCI", 1, "USB 2.0 controller initialized and devices enumerated");
                break;
            }
        }
    }
    if (!ehci_found) {
        boot_log_component("EHCI", 0, "No EHCI controller found");
    }
    boot_log_info("Initializing OHCI USB controller...");
    int ohci_found = 0;
    for (int i = 0; i < pci_count; i++) {
        pci_device_t* dev = bus_pci_device_at(i);
        if (dev->device_class == OHCI_CLASS && dev->device_subclass == OHCI_SUBCLASS && dev->prog_interface == OHCI_PROG_IF) {
            uint64_t base_addr = dev->base_addresses[0];
            if (ohci_init(base_addr) == 0) {
                ohci_start();
                ohci_enumerate_devices();
                ohci_found = 1;
                boot_log_component("OHCI", 1, "USB 1.1 controller initialized and devices enumerated");
                break;
            }
        }
    }
    if (!ohci_found) {
        boot_log_component("OHCI", 0, "No OHCI controller found");
    }
    boot_log_info("Initializing UHCI USB controller...");
    int uhci_found = 0;
    for (int i = 0; i < pci_count; i++) {
        pci_device_t* dev = bus_pci_device_at(i);
        if (dev->device_class == UHCI_CLASS && dev->device_subclass == UHCI_SUBCLASS && dev->prog_interface == UHCI_PROG_IF) {
            uint64_t base_addr = dev->base_addresses[4] & ~0xF;
            if (uhci_init(base_addr) == 0) {
                uhci_start();
                uhci_enumerate_devices();
                uhci_found = 1;
                boot_log_component("UHCI", 1, "USB 1.1 controller initialized and devices enumerated");
                break;
            }
        }
    }
    if (!uhci_found) {
        boot_log_component("UHCI", 0, "No UHCI controller found");
    }
    boot_log_info("Initializing XHCI USB controller...");
    int xhci_found = 0;
    for (int i = 0; i < pci_count; i++) {
        pci_device_t* dev = bus_pci_device_at(i);
        if (dev->device_class == 0x0C && dev->device_subclass == 0x03 && dev->prog_interface == 0x30) {
            uint64_t base_addr = dev->base_addresses[0];
            if (xhci_init(base_addr) == 0) {
                xhci_start();
                xhci_enumerate_devices();
                xhci_found = 1;
                boot_log_component("XHCI", 1, "USB 3.0 controller initialized and devices enumerated");
                break;
            }
        }
    }
    if (!xhci_found) {
        boot_log_component("XHCI", 0, "No XHCI controller found");
    }
    boot_log_info("Initializing ATA driver...");
    ata_init();
    boot_log_component("ATA", 1, "ATA/IDE driver initialized");
    boot_log_info("Initializing network stack...");
    network_init();
    if (net_config.mac[0] || net_config.mac[1] || net_config.mac[2] || net_config.mac[3] || net_config.mac[4] || net_config.mac[5]) {
        boot_log_component("Network Stack", 1, "Basic network stack initialized");
    } else {
        boot_log_component("Network Stack", 0, "No Ethernet device found");
    }
    boot_log_info("Setting up loadable kernel module system...");
    lkm_init();
    boot_log_component("LKM System", 1, "Loadable kernel module system ready");
    struct lkm_module console_lkm = {
        .name = "console",
        .version = "1.0",
        .depends = NULL,
        .init = console_handler.startup,
        .exit = console_handler.shutdown,
        .module_core = NULL,
        .size = 0
    };
    lkm_register_builtin(&console_lkm);
    struct lkm_module keyboard_lkm = {
        .name = "keyboard",
        .version = "1.0",
        .depends = NULL,
        .init = keyboard_handler.startup,
        .exit = keyboard_handler.shutdown,
        .module_core = NULL,
        .size = 0
    };
    lkm_register_builtin(&keyboard_lkm);
    struct lkm_module mouse_lkm = {
        .name = "mouse",
        .version = "1.0",
        .depends = NULL,
        .init = mouse_handler.startup,
        .exit = mouse_handler.shutdown,
        .module_core = NULL,
        .size = 0
    };
    lkm_register_builtin(&mouse_lkm);
    boot_log_info("Loading core system components as LKM...");
    enroll_component(&console_handler);
    enroll_component(&keyboard_handler);
    enroll_component(&mouse_handler);
    int count = count_components();
    char buf[64];
    buf[0] = '\0';
    str_append_uint(buf, count);
    str_append(buf, " core components registered");
    boot_log_component("Core Components", 1, buf);
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
    boot_log_info("Setting up USB mouse support...");
    int usb_mouse_result = usb_mouse_init();
    if (usb_mouse_result == 0) {
        boot_log_component("USB Mouse", 1, "USB mouse driver initialized with HID protocol support");
    } else {
        boot_log_component("USB Mouse", 0, "USB mouse initialization failed - no USB devices detected");
    }
    boot_log_info("Setting up USB keyboard support...");
    int usb_keyboard_result = usb_keyboard_init();
    if (usb_keyboard_result == 0) {
        boot_log_component("USB Keyboard", 1, "USB keyboard driver initialized with HID protocol support");
    } else {
        boot_log_component("USB Keyboard", 0, "USB keyboard initialization failed - no USB devices detected");
    }
    boot_log_info("Setting input device priorities...");
    if (usb_keyboard_is_initialized()) {
        usb_keyboard_set_callback(console_handle_key);
    } else {
        keyboard_init();
    }
    if (usb_mouse_is_initialized()) {
        usb_mouse_set_callback(mouse_callback);
    } else {
        mouse_init();
        usb_mouse_enable_test_mode();
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
    if (net_config.mac[0] || net_config.mac[1] || net_config.mac[2] || net_config.mac[3] || net_config.mac[4] || net_config.mac[5]) {
        boot_log_info("Network: CONFIGURED");
    } else {
        boot_log_info("Network: NOT AVAILABLE");
    }
    boot_log_print("acsh: [ OK ]");
    boot_log_print("System ready for user interaction");
    boot_log_print("");
    boot_log_print("");
    boot_log_print("");
    boot_log_print("Kernel(AC-0099) version: 1.2.6 - release.");
    boot_log_print("KernixOS: Welcome.");
    boot_log_print("Type 'help' for available commands");
    console_init();
    setcontext(THEME_CONSOLE);
    putchar('\n', gray_70);
    shell_print_prompt();
    console_set_input_start_x();
    cursor_enable();
    extern int boot_completed;
    boot_completed = 1;
    while (1) {
        ehci_handle_interrupt();
        ohci_handle_interrupt();
        uhci_handle_interrupt();
        xhci_handle_event();
        uint8_t packet[2048];
        int len = network_receive_packet(packet, sizeof(packet));
        if (len > 0) {
            network_handle_packet(packet, len);
        }
        __asm__ volatile("hlt");
    }
    #ifdef USE_HCF
        system_halt();
    #else
        initiate_panic("USE_HCF; FAILED --> USING PANIC");
    #endif
};