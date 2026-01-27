#include "psf.h"
#include <string/string.h>
#include <fs/vfs/vfs.h>
#include <drivers/memory/mem.h>
#define MAX_FONT_SIZE (64 * 1024)
static u8 font_buffer[MAX_FONT_SIZE];
static u16 read_u16_le(const u8* data, u32 offset) {
    return data[offset] | (data[offset + 1] << 8);
}
static u32 read_u32_le(const u8* data, u32 offset) {
    return data[offset] | (data[offset + 1] << 8) |
           (data[offset + 2] << 16) | (data[offset + 3] << 24);
}
int psf_load(psf_font_t* font, const char* filename) {
    memset(font, 0, sizeof(psf_font_t));
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
    if (total_read < sizeof(psf1_header_t)) {
        return -1;
    }
    font->data = font_buffer;
    font->size = total_read;
    u32 magic = read_u32_le(font->data, 0);
    if (magic == PSF2_MAGIC) {
        font->version = 2;
        psf2_header_t* header = (psf2_header_t*)font->data;
        font->num_chars = header->numglyph;
        font->char_size = header->bytesperglyph;
        font->height = header->height;
        font->width = header->width;
    } else if ((magic & 0xFFFF) == PSF1_MAGIC) {
        font->version = 1;
        psf1_header_t* header = (psf1_header_t*)font->data;
        font->num_chars = (header->mode & 0x01) ? 512 : 256;
        font->char_size = header->charsize;
        font->height = header->charsize;
        font->width = 8;
    } else {
        return -1;
    }
    font->loaded = 1;
    return 0;
}
void psf_unload(psf_font_t* font) {
    memset(font, 0, sizeof(psf_font_t));
}
const u8* psf_get_glyph_bitmap(psf_font_t* font, u32 charcode) {
    if (!font->loaded || charcode >= font->num_chars) {
        return NULL;
    }
    u32 offset;
    if (font->version == 2) {
        psf2_header_t* header = (psf2_header_t*)font->data;
        offset = header->headersize + (charcode * font->char_size);
    } else {
        psf1_header_t* header = (psf1_header_t*)font->data;
        offset = sizeof(psf1_header_t);
        if (header->mode & 0x02) {
        }
        offset += (charcode * font->char_size);
    }
    if (offset + font->char_size > font->size) {
        return NULL;
    }
    return font->data + offset;
}
u32 psf_get_glyph_width(psf_font_t* font) {
    if (!font->loaded) return 8;
    return font->width;
}
u32 psf_get_glyph_height(psf_font_t* font) {
    if (!font->loaded) return 16;  
    return font->height;
}