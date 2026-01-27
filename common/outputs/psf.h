#ifndef PSF_H
#define PSF_H
#include <outputs/types.h>
#define PSF1_MAGIC 0x0436
#define PSF2_MAGIC 0x864ab572
typedef struct {
    u16 magic;
    u8 mode;
    u8 charsize;
} __attribute__((packed)) psf1_header_t;
typedef struct {
    u32 magic;
    u32 version;
    u32 headersize;
    u32 flags;
    u32 numglyph;
    u32 bytesperglyph;
    u32 height;
    u32 width;
} __attribute__((packed)) psf2_header_t;
typedef struct {
    u8* data;
    u32 size;
    u8 version;
    u32 num_chars;
    u32 char_size;
    u32 height;
    u32 width;
    u8 loaded;
} psf_font_t;
int psf_load(psf_font_t* font, const char* filename);
void psf_unload(psf_font_t* font);
const u8* psf_get_glyph_bitmap(psf_font_t* font, u32 charcode);
u32 psf_get_glyph_width(psf_font_t* font);
u32 psf_get_glyph_height(psf_font_t* font);
#endif