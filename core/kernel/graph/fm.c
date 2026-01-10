#include "fm.h"
#include <ui/fonts.h>
#include <ui/fonts/font_8x16.h>
#include <kernel/communication/serial.h>
#include <theme/stdclrs.h>
#include <kernel/graph/theme.h>
#include <theme/tmx.h>
void fm_init(void) {
    BOOTUP_PRINTBS("[FM] ", GFX_GRAY_70);
    BOOTUP_PRINTBS("font manager initialized\n", white());
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