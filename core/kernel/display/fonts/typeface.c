#include "typeface.h"
#include <ui/fonts.h>
#include <ui/fonts/font_8x16.h>
#include <kernel/communication/serial.h>
#include <ui/theme/colors.h>
#include <config/boot.h>
void fm_init(void) {
    SYSTEM_PRINTBS("[FM] ", gray_70);
    SYSTEM_PRINTBS("font manager initialized\n", theme_white);
}
const u8* fm_get_glyph(u8 character){
    return font_8x16[character % 256];
}
u32 fm_get_char_width(void) {
    return FONT_WIDTH;
}
u32 fm_get_char_height(void) {
    return FONT_HEIGHT;
}