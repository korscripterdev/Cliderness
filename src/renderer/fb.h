/*
 * Cliderness - renderer/fb.h  (ClideRenderer)
 * Logical 80x25 character-cell framebuffer with VGA 16-colour attributes.
 *
 * A cell stores either a CP437 glyph index (flags without FBC_UNI) or, for
 * localised text that CP437 cannot express, a Unicode BMP code point
 * (FBC_UNI). Wide characters occupy two cells; the second is FBC_CONT.
 * Terminal adapters compare a back buffer with the last presented front
 * buffer and only emit the dirty cells.
 */
#ifndef CLIDE_RENDERER_FB_H
#define CLIDE_RENDERER_FB_H

#include "core/types.h"

#define FB_W 80
#define FB_H 25
#define FB_CELLS (FB_W * FB_H)

/* VGA palette indices (same order as BIOS text attributes). */
enum {
    C_BLACK = 0, C_BLUE, C_GREEN, C_CYAN, C_RED, C_MAGENTA, C_BROWN, C_LIGHTGRAY,
    C_DARKGRAY, C_LIGHTBLUE, C_LIGHTGREEN, C_LIGHTCYAN, C_LIGHTRED, C_LIGHTMAGENTA,
    C_YELLOW, C_WHITE
};

#define ATTR(fg, bg) ((cl_u8)((((bg) & 15) << 4) | ((fg) & 15)))
#define ATTR_FG(a) ((a) & 15)
#define ATTR_BG(a) (((a) >> 4) & 15)

#define FBC_UNI  1
#define FBC_CONT 2

typedef struct FbCell {
    cl_u16 ch;
    cl_u8  attr;
    cl_u8  flags;
} FbCell;

typedef struct Framebuffer {
    FbCell cells[FB_CELLS];
} Framebuffer;

void fb_clear(Framebuffer *fb, cl_u8 attr);
void fb_put(Framebuffer *fb, int x, int y, int glyph, cl_u8 attr);
FbCell *fb_cell(Framebuffer *fb, int x, int y);
void fb_fill(Framebuffer *fb, int x, int y, int w, int h, int glyph, cl_u8 attr);
void fb_attr_rect(Framebuffer *fb, int x, int y, int w, int h, cl_u8 attr);
/* Draw UTF-8 text clipped to maxw columns (maxw <= 0: to screen edge).
 * Returns the number of columns written. */
int  fb_text(Framebuffer *fb, int x, int y, const char *utf8, cl_u8 attr, int maxw);
int  fb_text_center(Framebuffer *fb, int y, const char *utf8, cl_u8 attr);
void fb_box(Framebuffer *fb, int x, int y, int w, int h, cl_u8 attr, int dbl);
void fb_hline(Framebuffer *fb, int x, int y, int w, int glyph, cl_u8 attr);
void fb_vline(Framebuffer *fb, int x, int y, int h, int glyph, cl_u8 attr);
/* Bar of width w showing value/max using shading glyphs */
void fb_bar(Framebuffer *fb, int x, int y, int w, long value, long max, cl_u8 attr);
/* Plain-text dump (UTF-8) of the framebuffer, used by headless tests. */
void fb_dump_row(const Framebuffer *fb, int y, char *out, size_t n);

int  color_from_name(const char *name);
const char *color_name(int c);
/* Night / distance dimming of a colour */
int  color_dim(int c);

#endif
