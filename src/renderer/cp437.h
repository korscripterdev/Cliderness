/*
 * Cliderness - renderer/cp437.h
 * Logical CP437 glyph set (Cliderness Visual Standard). Glyph index 0..255 is
 * what lives in the framebuffer; adapters translate it for the terminal.
 * Indices 0x00-0x1F and 0x7F are drawable glyphs, never raw control bytes.
 */
#ifndef CLIDE_RENDERER_CP437_H
#define CLIDE_RENDERER_CP437_H

#include "core/types.h"

long cp437_to_unicode(int glyph);
/* -1 when the code point has no CP437 glyph */
int  unicode_to_cp437(long cp);
/* 1 when every character of the UTF-8 string is representable in CP437 */
int  cp437_can_show(const char *utf8);

/* Frequently used glyphs */
#define G_SMILE    0x01
#define G_HEART    0x03
#define G_DIAMOND  0x04
#define G_CLUB     0x05
#define G_SPADE    0x06
#define G_BULLET   0x07
#define G_SUN      0x0F
#define G_ARROW_R  0x10
#define G_ARROW_L  0x11
#define G_UP       0x1E
#define G_DOWN     0x1F
#define G_YEN      0x9D
#define G_SHADE1   0xB0
#define G_SHADE2   0xB1
#define G_SHADE3   0xB2
#define G_VLINE    0xB3
#define G_HLINE    0xC4
#define G_TL       0xDA
#define G_TR       0xBF
#define G_BL       0xC0
#define G_BR       0xD9
#define G_DVLINE   0xBA
#define G_DHLINE   0xCD
#define G_DTL      0xC9
#define G_DTR      0xBB
#define G_DBL      0xC8
#define G_DBR      0xBC
#define G_BLOCK    0xDB
#define G_HALF_LO  0xDC
#define G_HALF_HI  0xDF
#define G_MIDDOT   0xFA
#define G_SQUARE   0xFE
#define G_DEGREE   0xF8
#define G_APPROX   0xF7

#endif
