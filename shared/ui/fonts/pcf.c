#include "pcf.h"
#include <string/string.h>  
#include <kernel/file_systems/vfs/vfs.h>
#define MAX_FONT_SIZE (16 * 1024)
static u8 font_buffer[MAX_FONT_SIZE];
static u32 read_u32(const u8* data, u32 offset) {
    return *((u32*)(data + offset));
}
int pcf_load(pcf_font_t* font, const char* filename) {
    for (u32 i = 0; i < sizeof(pcf_font_t); i++) {
        ((u8*)font)[i] = 0;
    }
    int fd = fs_open(filename, 0);
    if (fd < 0) {
        return -1;
    }
    u32 total_read = 0;
    u32 read_bytes;
    while ((read_bytes = fs_read(fd, font_buffer + total_read, 
                                 sizeof(font_buffer) - total_read)) > 0) {
        total_read += read_bytes;
        if (total_read >= sizeof(font_buffer)) {
            break;
        }
    }
    fs_close(fd);
    if (total_read < sizeof(pcf_header_t)) {
        return -1;
    }
    font->data = font_buffer;
    pcf_header_t* header = (pcf_header_t*)font->data;
    if (header->magic != PCF_MAGIC) {
        return -1;
    }
    pcf_table_t* tables = (pcf_table_t*)(font->data + sizeof(pcf_header_t));
    for (u32 i = 0; i < header->table_count; i++) {
        if (tables[i].type & PCF_METRICS) {
            u8* table_data = font->data + tables[i].offset;
            u32 format = read_u32(table_data, 0);
            table_data += 4;
            u32 glyphs = 0;
            if (format & 0x0200) {
                glyphs = (table_data[0] << 8) | table_data[1];
                table_data += 2;
            } else {
                glyphs = table_data[0] | (table_data[1] << 8);
                table_data += 2;
            }
            font->glyph_count = glyphs;
            font->metrics = (pcf_metric_t*)table_data;
        }
        if (tables[i].type & PCF_BITMAPS) {
            u8* table_data = font->data + tables[i].offset;
            u32 format = read_u32(table_data, 0);
            table_data += 4;
            u32 bitmap_sizes[4];
            for (int j = 0; j < 4; j++) {
                if (format & 0x0200) {
                    bitmap_sizes[j] = (table_data[0] << 24) | (table_data[1] << 16) | 
                                      (table_data[2] << 8) | table_data[3];
                } else {
                    bitmap_sizes[j] = table_data[0] | (table_data[1] << 8) | 
                                      (table_data[2] << 16) | (table_data[3] << 24);
                }
                table_data += 4;
            }
            (void)bitmap_sizes; 
            font->bitmaps = table_data;
        }
        if (tables[i].type & PCF_BDF_ENCODING) {
            u8* table_data = font->data + tables[i].offset;
            u32 format = read_u32(table_data, 0);
            table_data += 4;
            u16 min_char;
            if (format & 0x0200) {
                min_char = (table_data[0] << 8) | table_data[1];
            } else {
                min_char = table_data[0] | (table_data[1] << 8);
            }
            table_data += 4;
            font->encoding = (u16*)table_data;
            font->default_char = min_char;
        }
    }
    if (font->metrics && font->glyph_count > 0) {
        font->font_ascent = font->metrics[0].ascent;
        font->font_descent = font->metrics[0].descent;
        font->min_bounds_width = 7;
        font->max_bounds_width = 7;
        font->min_bounds_height = 14;
        font->max_bounds_height = 14;
        font->bytes_per_row = (7 + 7) / 8;
    } else {
        font->font_ascent = 11;
        font->font_descent = 3;
        font->min_bounds_width = 7;
        font->max_bounds_width = 7;
        font->min_bounds_height = 14;
        font->max_bounds_height = 14;
        font->bytes_per_row = 1;
        font->glyph_count = 256;
    }
    font->loaded = 1;
    return 0;
}
void pcf_unload(pcf_font_t* font) {
    for (u32 i = 0; i < sizeof(pcf_font_t); i++) {
        ((u8*)font)[i] = 0;
    }
}
const u8* pcf_get_glyph_bitmap(pcf_font_t* font, u32 charcode) {
    if (!font->loaded || charcode >= font->glyph_count) {
        return NULL;
    }
    u32 offset = charcode * 14;
    if (font->bitmaps + offset >= font->data + MAX_FONT_SIZE) {
        return NULL;
    }
    return font->bitmaps + offset;
}
u32 pcf_get_glyph_width(pcf_font_t* font, u32 charcode) {
    (void)charcode;
    if (!font->loaded) {
        return 8;
    }
    return font->min_bounds_width;
}
u32 pcf_get_glyph_height(pcf_font_t* font) {
    if (!font->loaded) {
        return 8;
    }
    return font->font_ascent + font->font_descent;
}
u32 pcf_get_font_height(pcf_font_t* font) {
    return pcf_get_glyph_height(font);
}