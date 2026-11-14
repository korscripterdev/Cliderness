/* Cliderness - renderer/fb.c */
#include <string.h>
#include "renderer/fb.h"
#include "renderer/cp437.h"
#include "core/str.h"

static const char *const g_color_names[16] = {
    "black", "blue", "green", "cyan", "red", "magenta", "brown", "lightgray",
    "darkgray", "lightblue", "lightgreen", "lightcyan", "lightred", "lightmagenta",
    "yellow", "white"
};

/* bright -> dark, dark -> darkgray/black */
static const cl_u8 g_dim[16] = {
    C_BLACK, C_BLACK, C_DARKGRAY, C_DARKGRAY, C_DARKGRAY, C_DARKGRAY, C_DARKGRAY, C_DARKGRAY,
    C_BLACK, C_BLUE, C_GREEN, C_CYAN, C_RED, C_MAGENTA, C_BROWN, C_LIGHTGRAY
};

int color_from_name(const char *name)
{
    int i;
    for (i = 0; i < 16; ++i) {
        if (str_ieq(name, g_color_names[i])) {
            return i;
        }
    }
    if (str_ieq(name, "yellowbrown") || str_ieq(name, "darkyellow")) {
        return C_BROWN;
    }
    if (str_ieq(name, "gray") || str_ieq(name, "grey")) {
        return C_LIGHTGRAY;
    }
    return -1;
}

const char *color_name(int c)
{
    return g_color_names[c & 15];
}

int color_dim(int c)
{
    return g_dim[c & 15];
}

void fb_clear(Framebuffer *fb, cl_u8 attr)
{
    int i;
    for (i = 0; i < FB_CELLS; ++i) {
        fb->cells[i].ch = ' ';
        fb->cells[i].attr = attr;
        fb->cells[i].flags = 0;
    }
}

FbCell *fb_cell(Framebuffer *fb, int x, int y)
{
    if (x < 0 || y < 0 || x >= FB_W || y >= FB_H) {
        return NULL;
    }
    return &fb->cells[y * FB_W + x];
}

static void fix_wide_neighbour(Framebuffer *fb, int x, int y)
{
    FbCell *c = fb_cell(fb, x, y);
    FbCell *n;
    if (!c) {
        return;
    }
    if (c->flags & FBC_CONT) {
        n = fb_cell(fb, x - 1, y);
        if (n) {
            n->ch = ' ';
            n->flags = 0;
        }
    }
    n = fb_cell(fb, x + 1, y);
    if (n && (n->flags & FBC_CONT)) {
        n->ch = ' ';
        n->flags = 0;
    }
}

void fb_put(Framebuffer *fb, int x, int y, int glyph, cl_u8 attr)
{
    FbCell *c = fb_cell(fb, x, y);
    if (!c) {
        return;
    }
    fix_wide_neighbour(fb, x, y);
    c->ch = (cl_u16)(glyph & 255);
    c->attr = attr;
    c->flags = 0;
}

void fb_fill(Framebuffer *fb, int x, int y, int w, int h, int glyph, cl_u8 attr)
{
    int i, j;
    for (j = 0; j < h; ++j) {
        for (i = 0; i < w; ++i) {
            fb_put(fb, x + i, y + j, glyph, attr);
        }
    }
}

void fb_attr_rect(Framebuffer *fb, int x, int y, int w, int h, cl_u8 attr)
{
    int i, j;
    for (j = 0; j < h; ++j) {
        for (i = 0; i < w; ++i) {
            FbCell *c = fb_cell(fb, x + i, y + j);
            if (c) {
                c->attr = attr;
            }
        }
    }
}

int fb_text(Framebuffer *fb, int x, int y, const char *utf8, cl_u8 attr, int maxw)
{
    int col = 0;
    long cp;
    int limit;
    if (!utf8 || y < 0 || y >= FB_H) {
        return 0;
    }
    limit = (maxw > 0) ? maxw : FB_W - x;
    while ((cp = utf8_next(&utf8)) >= 0) {
        int w = uni_width(cp);
        int g;
        FbCell *c;
        if (cp == '\n') {
            break;
        }
        if (col + w > limit || x + col + w > FB_W) {
            break;
        }
        if (x + col < 0) {
            col += w;
            continue;
        }
        g = (cp < 0x10000L) ? unicode_to_cp437(cp) : -1;
        if (g >= 0 && w == 1) {
            fb_put(fb, x + col, y, g, attr);
        } else {
            c = fb_cell(fb, x + col, y);
            fix_wide_neighbour(fb, x + col, y);
            c->ch = (cl_u16)(cp < 0x10000L ? cp : 0xFFFDL);
            c->attr = attr;
            c->flags = FBC_UNI;
            if (w == 2) {
                FbCell *c2 = fb_cell(fb, x + col + 1, y);
                fix_wide_neighbour(fb, x + col + 1, y);
                c2->ch = 0;
                c2->attr = attr;
                c2->flags = FBC_CONT;
            }
        }
        col += w;
    }
    return col;
}

int fb_text_center(Framebuffer *fb, int y, const char *utf8, cl_u8 attr)
{
    int w = utf8_width(utf8);
    int x = (FB_W - w) / 2;
    if (x < 0) {
        x = 0;
    }
    return fb_text(fb, x, y, utf8, attr, 0);
}

void fb_hline(Framebuffer *fb, int x, int y, int w, int glyph, cl_u8 attr)
{
    int i;
    for (i = 0; i < w; ++i) {
        fb_put(fb, x + i, y, glyph, attr);
    }
}

void fb_vline(Framebuffer *fb, int x, int y, int h, int glyph, cl_u8 attr)
{
    int i;
    for (i = 0; i < h; ++i) {
        fb_put(fb, x, y + i, glyph, attr);
    }
}

void fb_box(Framebuffer *fb, int x, int y, int w, int h, cl_u8 attr, int dbl)
{
    int hl = dbl ? G_DHLINE : G_HLINE;
    int vl = dbl ? G_DVLINE : G_VLINE;
    if (w < 2 || h < 2) {
        return;
    }
    fb_fill(fb, x + 1, y + 1, w - 2, h - 2, ' ', attr);
    fb_hline(fb, x + 1, y, w - 2, hl, attr);
    fb_hline(fb, x + 1, y + h - 1, w - 2, hl, attr);
    fb_vline(fb, x, y + 1, h - 2, vl, attr);
    fb_vline(fb, x + w - 1, y + 1, h - 2, vl, attr);
    fb_put(fb, x, y, dbl ? G_DTL : G_TL, attr);
    fb_put(fb, x + w - 1, y, dbl ? G_DTR : G_TR, attr);
    fb_put(fb, x, y + h - 1, dbl ? G_DBL : G_BL, attr);
    fb_put(fb, x + w - 1, y + h - 1, dbl ? G_DBR : G_BR, attr);
}

void fb_bar(Framebuffer *fb, int x, int y, int w, long value, long max, cl_u8 attr)
{
    long eighths;
    int i;
    if (max <= 0) {
        max = 1;
    }
    value = CL_CLAMP(value, 0L, max);
    /* 2 sub-steps per cell: full block or half shade */
    eighths = (value * (long)w * 2L + max / 2L) / max;
    for (i = 0; i < w; ++i) {
        long cell = eighths - (long)i * 2L;
        int g = cell >= 2 ? G_BLOCK : (cell == 1 ? G_SHADE2 : G_MIDDOT);
        fb_put(fb, x + i, y, g, cell > 0 ? attr : ATTR(C_DARKGRAY, ATTR_BG(attr)));
    }
}

void fb_dump_row(const Framebuffer *fb, int y, char *out, size_t n)
{
    size_t o = 0;
    int x;
    char tmp[4];
    for (x = 0; x < FB_W; ++x) {
        const FbCell *c = &fb->cells[y * FB_W + x];
        long cp;
        int len, k;
        if (c->flags & FBC_CONT) {
            continue;
        }
        cp = (c->flags & FBC_UNI) ? (long)c->ch : cp437_to_unicode(c->ch);
        len = utf8_encode(cp, tmp);
        if (o + (size_t)len + 1 >= n) {
            break;
        }
        for (k = 0; k < len; ++k) {
            out[o++] = tmp[k];
        }
    }
    while (o > 0 && out[o - 1] == ' ') {
        --o;
    }
    out[o] = '\0';
}
