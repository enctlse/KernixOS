#include "usb_keyboard.h"
#include <kernel/console/console.h>
static const char hid_to_ascii[256] = {
    [0x04] = 'a', [0x05] = 'b', [0x06] = 'c', [0x07] = 'd', [0x08] = 'e',
    [0x09] = 'f', [0x0A] = 'g', [0x0B] = 'h', [0x0C] = 'i', [0x0D] = 'j',
    [0x0E] = 'k', [0x0F] = 'l', [0x10] = 'm', [0x11] = 'n', [0x12] = 'o',
    [0x13] = 'p', [0x14] = 'q', [0x15] = 'r', [0x16] = 's', [0x17] = 't',
    [0x18] = 'u', [0x19] = 'v', [0x1A] = 'w', [0x1B] = 'x', [0x1C] = 'y',
    [0x1D] = 'z', [0x1E] = '1', [0x1F] = '2', [0x20] = '3', [0x21] = '4',
    [0x22] = '5', [0x23] = '6', [0x24] = '7', [0x25] = '8', [0x26] = '9',
    [0x27] = '0', [0x28] = '\n', [0x29] = '\x1B', [0x2A] = '\b', [0x2B] = '\t',
    [0x2C] = ' ', [0x2D] = '-', [0x2E] = '=', [0x2F] = '[', [0x30] = ']',
    [0x31] = '\\', [0x33] = ';', [0x34] = '\'', [0x35] = '`', [0x36] = ',',
    [0x37] = '.', [0x38] = '/'
};
static usb_kb_callback_t usb_kb_callback = NULL;
static int usb_kb_initialized = 0;
static uint8_t prev_keys[6] = {0};
static uint8_t prev_modifiers = 0;
int usb_keyboard_init(void) {
    usb_kb_initialized = 0;
    usb_kb_initialized = 1;
    print("USB Keyboard: Initialized\n", green);
    return 0;
}
void usb_keyboard_set_callback(usb_kb_callback_t cb) {
    usb_kb_callback = cb;
    if (cb) {
        print("USB Keyboard: Callback set\n", cyan);
    }
}
int usb_keyboard_is_initialized(void) {
    return usb_kb_initialized;
}
void usb_keyboard_handle_report(const usb_kb_report_t *report) {
    if (!report || !usb_kb_callback) return;
    int shift = (report->modifiers & 0x22) != 0;
    int ctrl = (report->modifiers & 0x11) != 0;
    int alt = (report->modifiers & 0x44) != 0;
    if (report->modifiers != prev_modifiers) {
        if (ctrl && (report->modifiers != prev_modifiers)) {
        }
        prev_modifiers = report->modifiers;
    }
    for (int i = 0; i < 6; i++) {
        uint8_t key = report->keys[i];
        if (key == prev_keys[i]) continue;
        if (key == 0) {
            prev_keys[i] = 0;
            continue;
        }
        prev_keys[i] = key;
        char ascii = hid_to_ascii[key];
        if (ascii) {
            if (shift) {
                if (ascii >= 'a' && ascii <= 'z') ascii -= 32;
                else if (ascii >= '1' && ascii <= '9') {
                    const char* shifted = "!@#$%^&*()";
                    ascii = shifted[ascii - '1'];
                } else {
                    switch (ascii) {
                        case '`': ascii = '~'; break;
                        case '-': ascii = '_'; break;
                        case '=': ascii = '+'; break;
                        case '[': ascii = '{'; break;
                        case ']': ascii = '}'; break;
                        case '\\': ascii = '|'; break;
                        case ';': ascii = ':'; break;
                        case '\'': ascii = '"'; break;
                        case ',': ascii = '<'; break;
                        case '.': ascii = '>'; break;
                        case '/': ascii = '?'; break;
                    }
                }
            }
            usb_kb_callback(ascii);
        } else {
            if (key == 0x52) usb_kb_callback(0x80);
            else if (key == 0x51) usb_kb_callback(0x81);
            else if (key == 0x50) usb_kb_callback(0x82);
            else if (key == 0x4F) usb_kb_callback(0x83);
        }
    }
    for (int i = 0; i < 6; i++) {
        int still_pressed = 0;
        for (int j = 0; j < 6; j++) {
            if (report->keys[j] == prev_keys[i] && report->keys[j] != 0) {
                still_pressed = 1;
                break;
            }
        }
        if (!still_pressed) {
            prev_keys[i] = 0;
        }
    }
}