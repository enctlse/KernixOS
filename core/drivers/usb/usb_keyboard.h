#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H
#include <outputs/types.h>
typedef struct __attribute__((packed)) {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} usb_kb_report_t;
typedef void (*usb_kb_callback_t)(int ascii);
int usb_keyboard_init(void);
void usb_keyboard_set_callback(usb_kb_callback_t cb);
int usb_keyboard_is_initialized(void);
void usb_keyboard_handle_report(const usb_kb_report_t *report);
#endif