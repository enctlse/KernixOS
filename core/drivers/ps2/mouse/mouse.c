#include "mouse.h"
#include <kernel/include/io.h>
#include <kernel/interrupts/handlers/irq.h>
#include <kernel/display/visual.h>
#include <fs/vfs/vfs.h>
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <outputs/types.h>
#define MOUSE_PORT_DATA    0x60
#define MOUSE_PORT_STATUS  0x64
#define MOUSE_PORT_CMD     0x64
static cursor_image_t* mouse_cursor_image = NULL;
static int32_t mouse_x_accum = 40000; 
static int32_t mouse_y_accum = 30000;
static int prev_mouse_x = -1;
static int prev_mouse_y = -1;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[4];
static int32_t mouse_x = 400;
static int32_t mouse_y = 300;
static uint8_t mouse_buttons = 0;
static int mouse_initialized = 0;
static int mouse_interrupt_count = 0;
static int mouse_cursor_needs_redraw = 1; 
static uint32_t cursor_bg[256];
static int cursor_bg_valid = 0;
static int mouse_sensitivity_factor = 120; 
#define MOUSE_CURSOR_WIDTH  16
#define MOUSE_CURSOR_HEIGHT 16
static const uint32_t embedded_cursor_pixels[256] = {
     0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};
static cursor_image_t embedded_cursor = {
    .width = 16,
    .height = 16,
    .pixels = (uint32_t*)embedded_cursor_pixels
};
typedef struct {
    uint16_t type;      
    uint32_t size;      
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;    
} __attribute__((packed)) bmp_file_header_t;
typedef struct {
    uint32_t size;              
    int32_t width;              
    int32_t height;             
    uint16_t planes;            
    uint16_t bpp;               
    uint32_t compression;       
    uint32_t image_size;        
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t important_colors;
} __attribute__((packed)) bmp_info_header_t;
cursor_image_t* load_cursor_bmp(const char* path) {
    int fd = fs_open(path, O_RDONLY);
    if (fd < 0) {
        print("Failed to open cursor file: ", red);
        print(path, red);
        print("\n", red);
        return NULL;
    }
    bmp_file_header_t file_header;
    bmp_info_header_t info_header;
    ssize_t read_bytes = fs_read(fd, &file_header, sizeof(bmp_file_header_t));
    if (read_bytes != sizeof(bmp_file_header_t) || file_header.type != 0x4D42) {
        print("Invalid BMP file header\n", red);
        fs_close(fd);
        return NULL;
    }
    read_bytes = fs_read(fd, &info_header, sizeof(bmp_info_header_t));
    if (read_bytes != sizeof(bmp_info_header_t)) {
        print("Failed to read BMP info header\n", red);
        fs_close(fd);
        return NULL;
    }
    if (info_header.width != 16 || info_header.height != 16 || info_header.bpp != 32) {
        print("Unsupported BMP format. Need 16x16 32-bit RGBA\n", red);
        fs_close(fd);
        return NULL;
    }
    extern void* fs_kernel_memory;
    cursor_image_t* cursor = (cursor_image_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(cursor_image_t), 1);
    if (!cursor) {
        print("Failed to allocate cursor image\n", red);
        fs_close(fd);
        return NULL;
    }
    cursor->width = 16;
    cursor->height = 16;
    cursor->pixels = (uint32_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(uint32_t), 16 * 16);
    if (!cursor->pixels) {
        print("Failed to allocate cursor pixels\n", red);
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)cursor);
        fs_close(fd);
        return NULL;
    }
    fs_close(fd); 
    fd = fs_open(path, O_RDONLY);
    if (fd < 0) return NULL;
    uint32_t data_offset = file_header.offset;
    uint8_t skip_buf[1024];
    while (data_offset > 0) {
        size_t to_skip = data_offset > 1024 ? 1024 : data_offset;
        read_bytes = fs_read(fd, skip_buf, to_skip);
        if (read_bytes <= 0) break;
        data_offset -= read_bytes;
    }
    uint32_t bmp_pixels[256];
    read_bytes = fs_read(fd, bmp_pixels, 16 * 16 * 4);
    fs_close(fd);
    if (read_bytes != 16 * 16 * 4) {
        print("Failed to read BMP pixel data\n", red);
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)cursor->pixels);
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)cursor);
        return NULL;
    }
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint32_t bmp_pixel = bmp_pixels[(15 - y) * 16 + x];
            uint8_t b = (bmp_pixel >> 0) & 0xFF;
            uint8_t g = (bmp_pixel >> 8) & 0xFF;
            uint8_t r = (bmp_pixel >> 16) & 0xFF;
            uint8_t a = (bmp_pixel >> 24) & 0xFF;
            cursor->pixels[y * 16 + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    print("Loaded BMP cursor: ", green);
    print(path, green);
    print("\n", green);
    return cursor;
}
cursor_image_t* load_cursor_from_file(const char* path) {
    extern cursor_image_t embedded_cursor;
    cursor_image_t* cursor = load_cursor_bmp(path);
    if (cursor) {
        if (mouse_cursor_image && mouse_cursor_image != &embedded_cursor) {
            extern void* fs_kernel_memory;
            if (mouse_cursor_image->pixels) {
                kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)mouse_cursor_image->pixels);
            }
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)mouse_cursor_image);
        }
        mouse_cursor_image = cursor;
        return cursor;
    }
    return NULL;
}
static cursor_image_t* load_embedded_cursor() {
    print("Loading embedded cursor image\n", cyan);
    return &embedded_cursor;
}
static mouse_callback_t mouse_callback = NULL;
static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(MOUSE_PORT_STATUS) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(MOUSE_PORT_STATUS) & 2) == 0) return;
        }
    }
}
static void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(MOUSE_PORT_CMD, 0xD4);
    mouse_wait(1);
    outb(MOUSE_PORT_DATA, write);
}
static uint8_t mouse_read() {
    mouse_wait(0);
    return inb(MOUSE_PORT_DATA);
}
void mouse_interrupt_handler(cpu_state_t* state __attribute__((unused))) {
    mouse_interrupt_count++;
    uint8_t status = inb(MOUSE_PORT_CMD);
    if (!(status & 0x20)) return;
    uint8_t b = inb(MOUSE_PORT_DATA);
    if (mouse_cycle == 0 && !(b & 0x08)) return;
    mouse_packet[mouse_cycle] = b;
    mouse_cycle++;
    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        uint8_t state = mouse_packet[0];
        int rel_x = (int8_t)mouse_packet[1];
        int rel_y = (int8_t)mouse_packet[2];
        int32_t delta_x_scaled = rel_x * mouse_sensitivity_factor;
        int32_t delta_y_scaled = rel_y * mouse_sensitivity_factor;
        if ((rel_x > 5 || rel_x < -5) || (rel_y > 5 || rel_y < -5)) {
            delta_x_scaled = (delta_x_scaled * 14) / 10;
            delta_y_scaled = (delta_y_scaled * 14) / 10;
        }
        mouse_x_accum += delta_x_scaled;
        mouse_y_accum -= delta_y_scaled;
        int32_t new_x = mouse_x_accum / 100;
        int32_t new_y = mouse_y_accum / 100;
        uint32_t fb_width = get_fb_width();
        uint32_t fb_height = get_fb_height();
        if (fb_width > 0 && fb_height > 0) {
            uint32_t cursor_width = mouse_cursor_image ? mouse_cursor_image->width : MOUSE_CURSOR_WIDTH;
            uint32_t cursor_height = mouse_cursor_image ? mouse_cursor_image->height : MOUSE_CURSOR_HEIGHT;
            if (new_x < 0) { 
                new_x = 0; 
                mouse_x_accum = 0; 
            }
            if (new_x >= (int32_t)(fb_width - cursor_width)) { 
                new_x = fb_width - cursor_width - 1; 
                mouse_x_accum = new_x * 100;
            }
            if (new_y < 0) { 
                new_y = 0; 
                mouse_y_accum = 0; 
            }
            if (new_y >= (int32_t)(fb_height - cursor_height)) { 
                new_y = fb_height - cursor_height - 1; 
                mouse_y_accum = new_y * 100;
            }
        }
        prev_mouse_x = mouse_x;
        prev_mouse_y = mouse_y;
        mouse_x = new_x;
        mouse_y = new_y;
        mouse_buttons = state & 0x07;
        mouse_cursor_needs_redraw = 1;
        if (mouse_callback) {
            mouse_callback(mouse_x, mouse_y, mouse_buttons);
        }
    }
    irq_ack(12);
}
int mouse_init() {
    if (mouse_initialized) return 0;
    print("PS/2: Initializing Mouse...\n", cyan);
    mouse_wait(1);
    outb(MOUSE_PORT_CMD, 0xA8);
    mouse_wait(1);
    outb(MOUSE_PORT_CMD, 0x20);
    uint8_t status = mouse_read();
    status |= 2;
    status &= ~0x20;
    mouse_wait(1);
    outb(MOUSE_PORT_CMD, 0x60);
    mouse_wait(1);
    outb(MOUSE_PORT_DATA, status);
    mouse_write(0xF6);
    mouse_read();
    print("PS/2: Setting Sample Rate to 200Hz...\n", gray_70);
    mouse_write(0xF3);
    mouse_read();
    mouse_write(200);
    mouse_read();
    mouse_write(0xE8); 
    mouse_read();
    mouse_write(0x03);
    mouse_read();
    mouse_write(0xF4);
    mouse_read(); 
    irq_set_mask(2, 0);
    irq_set_mask(12, 0);
    if (!mouse_cursor_image) {
        mouse_cursor_image = load_embedded_cursor();
    }
    uint32_t fb_width = get_fb_width();
    uint32_t fb_height = get_fb_height();
    if (fb_width > 0 && fb_height > 0) {
        mouse_x = fb_width / 2;
        mouse_y = fb_height / 2;
        mouse_x_accum = mouse_x * 100;
        mouse_y_accum = mouse_y * 100;
    }
    prev_mouse_x = mouse_x;
    prev_mouse_y = mouse_y;
    cursor_bg_valid = 0;
    mouse_initialized = 1;
    print("PS/2 mouse initialized successfully (200Hz)\n", green);
    return 0;
}
void mouse_get_position(int32_t *x, int32_t *y) {
    if (x) *x = mouse_x;
    if (y) *y = mouse_y;
}
void mouse_set_position(int32_t x, int32_t y) {
    mouse_x = x;
    mouse_y = y;
    mouse_x_accum = x * 100;
    mouse_y_accum = y * 100;
}
uint8_t mouse_get_buttons() {
    return mouse_buttons;
}
void mouse_set_callback(mouse_callback_t callback) {
    mouse_callback = callback;
}
void mouse_unregister_callback() {
    mouse_callback = NULL;
}
void mouse_set_buttons(uint8_t buttons) {
    mouse_buttons = buttons;
}
int mouse_is_initialized() {
    return mouse_initialized;
}
void mouse_draw_cursor() {
    uint32_t fb_width = get_fb_width();
    uint32_t fb_height = get_fb_height();
    const int CURSOR_WIDTH = 8;
    const int CURSOR_HEIGHT = 19;
    static const uint8_t cursor_bitmap[19] = {
        0b00000001, 0b00000011, 0b00000110, 0b00001100,
        0b00011000, 0b00110000, 0b01100000, 0b11000000,
        0b10000000, 0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000000, 0b00000000,
    };
    uint32_t cursor_width = mouse_cursor_image ? mouse_cursor_image->width : 8;
    uint32_t cursor_height = mouse_cursor_image ? mouse_cursor_image->height : 19;
    if (cursor_bg_valid && (prev_mouse_x != mouse_x || prev_mouse_y != mouse_y)) {
        int px = prev_mouse_x;
        int py = prev_mouse_y;
        for (int y = 0; y < (int)cursor_height; y++) {
            for (int x = 0; x < (int)cursor_width; x++) {
                if (px + x >= 0 && px + x < (int)fb_width && py + y >= 0 && py + y < (int)fb_height) {
                    putpixel(px + x, py + y, cursor_bg[y * cursor_width + x]);
                }
            }
        }
    }
    int cx = mouse_x;
    int cy = mouse_y;
    for (int y = 0; y < (int)cursor_height; y++) {
        for (int x = 0; x < (int)cursor_width; x++) {
            if (cx + x >= 0 && cx + x < (int)fb_width && cy + y >= 0 && cy + y < (int)fb_height) {
                cursor_bg[y * cursor_width + x] = getpixel(cx + x, cy + y);
            } else {
                cursor_bg[y * cursor_width + x] = 0;
            }
        }
    }
    cursor_bg_valid = 1;
    prev_mouse_x = mouse_x;
    prev_mouse_y = mouse_y;
    if (mouse_cursor_image) {
        int start_x = mouse_x;
        int start_y = mouse_y;
        int end_x = mouse_x + mouse_cursor_image->width;
        int end_y = mouse_y + mouse_cursor_image->height;
        if (start_x < 0) start_x = 0;
        if (start_y < 0) start_y = 0;
        if (end_x > (int)fb_width) end_x = fb_width;
        if (end_y > (int)fb_height) end_y = fb_height;
        for (int screen_y = start_y; screen_y < end_y; screen_y++) {
            int cursor_y = screen_y - mouse_y;
            for (int screen_x = start_x; screen_x < end_x; screen_x++) {
                int cursor_x = screen_x - mouse_x;
                uint32_t pixel = mouse_cursor_image->pixels[cursor_y * mouse_cursor_image->width + cursor_x];
                uint32_t alpha = (pixel >> 24) & 0xFF;
                if (alpha > 0) {
                    putpixel(screen_x, screen_y, pixel & 0x00FFFFFF);
                }
            }
        }
    } else {
        int start_x = mouse_x;
        int start_y = mouse_y;
        int end_x = mouse_x + CURSOR_WIDTH;
        int end_y = mouse_y + CURSOR_HEIGHT;
        if (start_x < 0) start_x = 0;
        if (start_y < 0) start_y = 0;
        if (end_x > (int)fb_width) end_x = fb_width;
        if (end_y > (int)fb_height) end_y = fb_height;
        for (int screen_y = start_y; screen_y < end_y; screen_y++) {
            int cursor_y = screen_y - mouse_y;
            uint8_t row = cursor_bitmap[cursor_y];
            for (int screen_x = start_x; screen_x < end_x; screen_x++) {
                int cursor_x = screen_x - mouse_x;
                if (row & (0x80 >> cursor_x)) {
                    putpixel(screen_x, screen_y, 0xFF000000);
                }
            }
        }
    }
}
void mouse_force_update() {
    mouse_draw_cursor();
    print("Mouse pos: ", cyan);
    char buf[64];
    str_copy(buf, "");
    str_append_uint(buf, (uint32_t)mouse_x);
    str_append(buf, ", ");
    str_append_uint(buf, (uint32_t)mouse_y);
    str_append(buf, " buttons: ");
    str_append_uint(buf, mouse_buttons);
    str_append(buf, " ints: ");
    str_append_uint(buf, mouse_interrupt_count);
    print(buf, cyan);
    print("\n", cyan);
}
void mouse_reload_cursor() {
    print("Reloading cursor to embedded arrow...\n", cyan);
    mouse_cursor_image = load_embedded_cursor();
    print("Cursor reloaded successfully\n", green);
}
int mouse_has_scroll_wheel() { return 0; }
int8_t mouse_get_scroll_delta() { return 0; }
void mouse_enable_test_mode() {}
int mouse_get_interrupt_count() { return mouse_interrupt_count; }
int mouse_cursor_needs_update() { return mouse_cursor_needs_redraw; }
void mouse_clear_cursor_redraw_flag() { mouse_cursor_needs_redraw = 0; }
void mouse_set_sensitivity(int factor) {
    if (factor < 10) factor = 10;
    if (factor > 500) factor = 500;
    mouse_sensitivity_factor = factor;
}
int mouse_get_sensitivity() {
    return mouse_sensitivity_factor;
}
static int mouse_handler_startup(void) {
    return mouse_init();
}
static void mouse_handler_shutdown(void) {
}
struct component_handler mouse_handler = {
    .identifier = "ps2_mouse",
    .attachment_point = "/dev/mouse",
    .build_number = BUILD_VERSION(0, 1, 0, 0),
    .startup = mouse_handler_startup,
    .shutdown = mouse_handler_shutdown,
    .access = NULL,
    .retrieve = NULL,
    .store = NULL,
};