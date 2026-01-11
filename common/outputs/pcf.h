#ifndef PCF_H
#define PCF_H
#include <outputs/types.h>
#define PCF_MAGIC 0x70636601
#define PCF_BYTE_ORDER(byte) ((byte) == 0x00 ? 0 : 1)
#define PCF_BIT_ORDER(byte)  (((byte) & 0x0100) ? 1 : 0)
typedef struct {
    u32 magic;
    u32 table_count;
} pcf_header_t;
typedef struct {
    u32 type;
    u32 format;
    u32 size;
    u32 offset;
} pcf_table_t;
#define PCF_PROPERTIES   (1 << 0)
#define PCF_ACCELERATORS (1 << 1)
#define PCF_METRICS      (1 << 2)
#define PCF_BITMAPS      (1 << 3)
#define PCF_INK_METRICS  (1 << 4)
#define PCF_BDF_ENCODING (1 << 5)
#define PCF_SWIDTHS      (1 << 6)
#define PCF_GLYPH_NAMES  (1 << 7)
typedef struct {
    u16 left_bearing;
    u16 right_bearing;
    u16 width;
    u16 ascent;
    u16 descent;
    u16 attributes;
} pcf_metric_t;
typedef struct {
    u8* data;
    u32 size;
    pcf_metric_t* metrics;
    u8* bitmaps;
    u16* encoding;
    u32 glyph_count;
    u32 default_char;
    u32 font_ascent;
    u32 font_descent;
    u32 max_bounds_width;
    u32 max_bounds_height;
    u32 min_bounds_width;
    u32 min_bounds_height;
    u32 bytes_per_row;
    u32 bitmap_pad;
    u8 loaded;
} pcf_font_t;
int pcf_load(pcf_font_t* font, const char* filename);
void pcf_unload(pcf_font_t* font);
const u8* pcf_get_glyph_bitmap(pcf_font_t* font, u32 charcode);
u32 pcf_get_glyph_width(pcf_font_t* font, u32 charcode);
u32 pcf_get_glyph_height(pcf_font_t* font);
u32 pcf_get_font_height(pcf_font_t* font);
#endif