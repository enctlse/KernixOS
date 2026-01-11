#ifndef USB_MOUSE_H
#define USB_MOUSE_H
#include <outputs/types.h>
#define USB_MOUSE_BUTTON_LEFT   0x01
#define USB_MOUSE_BUTTON_RIGHT  0x02
#define USB_MOUSE_BUTTON_MIDDLE 0x04
typedef void (*usb_mouse_callback_t)(int32_t x, int32_t y, uint8_t buttons);
int usb_mouse_init();
void usb_mouse_get_position(int32_t *x, int32_t *y);
void usb_mouse_set_position(int32_t x, int32_t y);
uint8_t usb_mouse_get_buttons();
void usb_mouse_set_callback(usb_mouse_callback_t callback);
void usb_mouse_unregister_callback();
void usb_mouse_draw_cursor();
int usb_mouse_is_initialized();
int usb_mouse_get_interrupt_count();
int usb_mouse_cursor_needs_update();
void usb_mouse_force_update();
void usb_mouse_enable_test_mode();
void usb_mouse_simulate_interrupt();
#endif