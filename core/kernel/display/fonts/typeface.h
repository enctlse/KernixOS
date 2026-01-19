#ifndef FM_H
#define FM_H
#include <outputs/types.h>
void fm_init(void);
const u8* fm_get_glyph(u8 character);
u32 fm_get_char_width(void);
u32 fm_get_char_height(void);
#endif