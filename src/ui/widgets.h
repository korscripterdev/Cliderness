/*
 * Cliderness - ui/widgets.h
 * Small reusable UI pieces drawn into the framebuffer.
 */
#ifndef CLIDE_UI_WIDGETS_H
#define CLIDE_UI_WIDGETS_H

#include "renderer/fb.h"
#include "input/keys.h"

#define UI_TEXT   ATTR(C_LIGHTGRAY, C_BLACK)
#define UI_DIM    ATTR(C_DARKGRAY, C_BLACK)
#define UI_TITLE  ATTR(C_YELLOW, C_BLACK)
#define UI_SEL    ATTR(C_BLACK, C_LIGHTGRAY)
#define UI_HI     ATTR(C_WHITE, C_BLACK)
#define UI_BOX    ATTR(C_LIGHTGRAY, C_BLUE)
#define UI_BOXSEL ATTR(C_BLUE, C_LIGHTGRAY)
#define UI_GOOD   ATTR(C_LIGHTGREEN, C_BLACK)
#define UI_BAD    ATTR(C_LIGHTRED, C_BLACK)
#define UI_KEY    ATTR(C_LIGHTCYAN, C_BLACK)

typedef struct TextInput {
    char buf[80];
    int  max;      /* bytes */
    int  ascii_only;
} TextInput;

/* returns 1 if the key was consumed */
int  ti_key(TextInput *t, const ClideKeyEvent *ev);
void ti_set(TextInput *t, const char *s, int max, int ascii_only);

/* vertical menu: returns 1 when Enter pressed on an item */
int  menu_key(int *sel, int count, const ClideKeyEvent *ev);
void menu_draw(Framebuffer *fb, int x, int y, int w, const char *const *items, int count, int sel, cl_u8 attr, cl_u8 sel_attr);

/* frame with a title in the top border */
void ui_frame(Framebuffer *fb, int x, int y, int w, int h, const char *title, cl_u8 attr);
/* hint line at the bottom of the screen */
void ui_hint(Framebuffer *fb, const char *text);
/* word-wrap UTF-8 text into lines of width w; returns line count.
 * line_off receives byte offsets of each line start (max lines). */
int  ui_wrap(const char *text, int w, int *line_off, int *line_len, int max);
void ui_draw_wrapped(Framebuffer *fb, int x, int y, int w, int h, const char *text, int scroll, cl_u8 attr);

#endif
