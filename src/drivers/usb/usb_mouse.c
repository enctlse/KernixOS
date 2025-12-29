#include "usb_mouse.h"
#include <kernel/include/ports.h>
#include <kernel/exceptions/irq.h>
#include <kernel/graph/graphics.h>
#include <types.h>
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;
} cursor_image_t;
static cursor_image_t* usb_mouse_cursor_image = NULL;
static int32_t usb_mouse_x = 400;
static int32_t usb_mouse_y = 300;
static uint8_t usb_mouse_buttons = 0;
static int usb_mouse_initialized = 0;
static int usb_mouse_interrupt_count = 0;
static int usb_mouse_test_mode = 0;
static const uint32_t usb_embedded_cursor_pixels[256] = {
0b0000000000000000,
0b0000000100000000,
0b0000000110000000,
0b0000000111000000,
0b0000000111100000,
0b0000000111110000,
0b0000000111111000,
0b0000000111111100,
0b0000000111111110,
0b0000000111111000,
0b0000000110110000,
0b0000000100110000,
0b0000000000110000,
0b0000000000110000,
0b0000000000000000,
0b0000000000000000
};
static cursor_image_t usb_embedded_cursor = {
    .width = 16,
    .height = 16,
    .pixels = (uint32_t*)usb_embedded_cursor_pixels
};
static int usb_mouse_test_counter = 0;
#define USB_MOUSE_CURSOR_WIDTH  16
#define USB_MOUSE_CURSOR_HEIGHT 16
static usb_mouse_callback_t usb_mouse_callback = NULL;
static void usb_mouse_interrupt_handler(uint8_t* report, int report_size) {
    usb_mouse_interrupt_count++;
    if (report_size < 4) return;
    usb_mouse_buttons = report[0] & 0x07;
    int16_t delta_x = (int8_t)report[1];
    if (report_size >= 5) {
        delta_x = (int16_t)(report[1] | (report[2] << 8));
    }
    int16_t delta_y = (int8_t)report[2];
    if (report_size >= 5) {
        delta_y = (int16_t)(report[3] | (report[4] << 8));
    }
    usb_mouse_x += delta_x;
    usb_mouse_y += delta_y;
    uint32_t fb_width = get_fb_width();
    uint32_t fb_height = get_fb_height();
    if (fb_width == 0) fb_width = 800;
    if (fb_height == 0) fb_height = 600;
    if (usb_mouse_x < 0) usb_mouse_x = 0;
    if (usb_mouse_y < 0) usb_mouse_y = 0;
    if (usb_mouse_x >= (int32_t)(fb_width - USB_MOUSE_CURSOR_WIDTH))
        usb_mouse_x = fb_width - USB_MOUSE_CURSOR_WIDTH - 1;
    if (usb_mouse_y >= (int32_t)(fb_height - USB_MOUSE_CURSOR_HEIGHT))
        usb_mouse_y = fb_height - USB_MOUSE_CURSOR_HEIGHT - 1;
    if (usb_mouse_callback) {
        usb_mouse_callback(usb_mouse_x, usb_mouse_y, usb_mouse_buttons);
    }
}
void usb_mouse_simulate_interrupt() {
    static int counter = 0;
    counter++;
    if (usb_mouse_test_mode) {
        if (counter % 50 == 0) {
            usb_mouse_x += 5;
            if (usb_mouse_x > 700) usb_mouse_x = 50;
            usb_mouse_y += 3;
            if (usb_mouse_y > 500) usb_mouse_y = 50;
            print("USB Test: X=", GFX_YELLOW);
            char buf[16];
            str_copy(buf, "");
            str_append_uint(buf, (uint32_t)usb_mouse_x);
            print(buf, GFX_CYAN);
            print(" Y=", GFX_YELLOW);
            str_copy(buf, "");
            str_append_uint(buf, (uint32_t)usb_mouse_y);
            print(buf, GFX_CYAN);
            print("\n", GFX_YELLOW);
            if (usb_mouse_callback) {
                usb_mouse_callback(usb_mouse_x, usb_mouse_y, usb_mouse_buttons);
            }
        }
    }
}
int usb_mouse_init() {
    if (usb_mouse_initialized) return 0;
    if (!usb_mouse_cursor_image) {
        usb_mouse_cursor_image = &usb_embedded_cursor;
    }
    uint32_t fb_width = get_fb_width();
    uint32_t fb_height = get_fb_height();
    if (fb_width > 0 && fb_height > 0) {
        usb_mouse_x = fb_width / 2;
        usb_mouse_y = fb_height / 2;
    }
    usb_mouse_initialized = 1;
    return 0;
}
void usb_mouse_get_position(int32_t *x, int32_t *y) {
    if (x) *x = usb_mouse_x;
    if (y) *y = usb_mouse_y;
}
void usb_mouse_set_position(int32_t x, int32_t y) {
    usb_mouse_x = x;
    usb_mouse_y = y;
}
uint8_t usb_mouse_get_buttons() {
    return usb_mouse_buttons;
}
void usb_mouse_set_callback(usb_mouse_callback_t callback) {
    usb_mouse_callback = callback;
}
void usb_mouse_unregister_callback() {
    usb_mouse_callback = NULL;
}
void usb_mouse_draw_cursor() {
    if (!usb_mouse_initialized) return;
    uint32_t fb_width = get_fb_width();
    uint32_t fb_height = get_fb_height();
    if (usb_mouse_cursor_image) {
        for (uint32_t y = 0; y < usb_mouse_cursor_image->height; y++) {
            for (uint32_t x = 0; x < usb_mouse_cursor_image->width; x++) {
                int screen_x = usb_mouse_x + x;
                int screen_y = usb_mouse_y + y;
                if (screen_x >= 0 && screen_x < (int)fb_width &&
                    screen_y >= 0 && screen_y < (int)fb_height) {
                    uint32_t pixel = usb_mouse_cursor_image->pixels[y * usb_mouse_cursor_image->width + x];
                    uint32_t alpha = (pixel >> 24) & 0xFF;
                    if (alpha > 0) {
                        putpixel(screen_x, screen_y, pixel & 0x00FFFFFF);
                    }
                }
            }
        }
    }
}
int usb_mouse_is_initialized() {
    return usb_mouse_initialized;
}
int usb_mouse_get_interrupt_count() {
    return usb_mouse_interrupt_count;
}
int usb_mouse_cursor_needs_update() {
    return 0;
}
void usb_mouse_force_update() {
    if (!usb_mouse_initialized) return;
    usb_mouse_draw_cursor();
    print("USB Mouse pos: ", GFX_YELLOW);
    char buf[64];
    str_copy(buf, "");
    str_append_uint(buf, (uint32_t)usb_mouse_x);
    str_append(buf, ", ");
    str_append_uint(buf, (uint32_t)usb_mouse_y);
    str_append(buf, " buttons: ");
    str_append_uint(buf, usb_mouse_buttons);
    str_append(buf, " ints: ");
    str_append_uint(buf, usb_mouse_interrupt_count);
    print(buf, GFX_YELLOW);
    print("\n", GFX_YELLOW);
}
void usb_mouse_enable_test_mode() {
    usb_mouse_test_mode = 1;
    print("USB mouse test mode enabled\n", GFX_CYAN);
}