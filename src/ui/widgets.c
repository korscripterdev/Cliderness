/* Cliderness - ui/widgets.c */
#include <string.h>
#include "ui/widgets.h"
#include "core/str.h"

void ti_set(TextInput *t, const char *s, int max, int ascii_only)
{
    str_copy(t->buf, s, sizeof(t->buf));
    t->max = max < (int)sizeof(t->buf) - 1 ? max : (int)sizeof(t->buf) - 1;
    t->ascii_only = ascii_only;
}

int ti_key(TextInput *t, const ClideKeyEvent *ev)
{
    size_t len = strlen(t->buf);
    if (ev->key == CK_BACKSPACE) {
        /* remove one UTF-8 character */
        while (len > 0) {
            --len;
            if (((unsigned char)t->buf[len] & 0xC0) != 0x80) {
                break;
            }
        }
        t->buf[len] = '\0';
        return 1;
    }
    if ((ev->mods & (KM_CTRL | KM_ALT)) || ev->key < 32 || ev->key >= CK_SPECIAL || ev->key == 127) {
        return 0;
    }
    if (t->ascii_only && ev->key > 126) {
        return 0;
    }
    {
        char enc[4];
        int n = utf8_encode(ev->key, enc);
        if ((int)len + n > t->max) {
            return 1;
        }
        memcpy(t->buf + len, enc, (size_t)n);
        t->buf[len + (size_t)n] = '\0';
    }
    return 1;
}

int menu_key(int *sel, int count, const ClideKeyEvent *ev)
{
    if (count <= 0) {
        return 0;
    }
    if (ev->key == CK_UP || ev->key == 'w' || ev->key == 'k') {
        *sel = (*sel + count - 1) % count;
    } else if (ev->key == CK_DOWN || ev->key == 's' || ev->key == 'j') {
        *sel = (*sel + 1) % count;
    } else if (ev->key == CK_HOME) {
        *sel = 0;
    } else if (ev->key == CK_END) {
        *sel = count - 1;
    } else if (ev->key == CK_ENTER || ev->key == CK_SPACE) {
        return 1;
    }
    return 0;
}

void menu_draw(Framebuffer *fb, int x, int y, int w, const char *const *items, int count, int sel,
               cl_u8 attr, cl_u8 sel_attr)
{
    int i;
    for (i = 0; i < count; ++i) {
        cl_u8 a = (i == sel) ? sel_attr : attr;
        int tw = utf8_width(items[i]);
        int pad = (w - tw - 4) / 2;
        fb_fill(fb, x, y + i, w, 1, ' ', a);
        if (pad < 0) {
            pad = 0;
        }
        fb_text(fb, x + pad, y + i, "[ ", a, 0);
        fb_text(fb, x + pad + 2, y + i, items[i], a, w - pad - 4);
        fb_text(fb, x + pad + 2 + tw, y + i, " ]", a, 0);
    }
}

void ui_frame(Framebuffer *fb, int x, int y, int w, int h, const char *title, cl_u8 attr)
{
    fb_box(fb, x, y, w, h, attr, 1);
    if (title && *title) {
        int tw = utf8_width(title);
        int tx = x + (w - tw - 2) / 2;
        fb_text(fb, tx, y, " ", attr, 0);
        fb_text(fb, tx + 1, y, title, (cl_u8)((attr & 0xF0) | C_YELLOW), w - 4);
        fb_text(fb, tx + 1 + tw, y, " ", attr, 0);
    }
}

void ui_hint(Framebuffer *fb, const char *text)
{
    fb_fill(fb, 0, FB_H - 1, FB_W, 1, ' ', ATTR(C_BLACK, C_LIGHTGRAY));
    fb_text(fb, 1, FB_H - 1, text, ATTR(C_BLACK, C_LIGHTGRAY), FB_W - 2);
}

int ui_wrap(const char *text, int w, int *line_off, int *line_len, int max)
{
    int lines = 0;
    const char *s = text;
    while (*s && lines < max) {
        const char *line_start = s;
        const char *p = s;
        const char *last_space = NULL;
        int col = 0;
        while (*p && *p != '\n') {
            const char *q = p;
            long cp = utf8_next(&q);
            int cw = uni_width(cp);
            if (col + cw > w) {
                break;
            }
            if (cp == ' ') {
                last_space = p;
            }
            col += cw;
            p = q;
        }
        if (*p && *p != '\n' && last_space && last_space > line_start) {
            p = last_space;
        }
        if (p == line_start && *p && *p != '\n') {
            utf8_next(&p);   /* a single glyph wider than the line */
        }
        line_off[lines] = (int)(line_start - text);
        line_len[lines] = (int)(p - line_start);
        ++lines;
        if (*p == '\n' || *p == ' ') {
            ++p;
        }
        s = p;
        if (!*s && p > text && p[-1] == '\n' && lines < max) {
            break;
        }
    }
    return lines;
}

void ui_draw_wrapped(Framebuffer *fb, int x, int y, int w, int h, const char *text, int scroll, cl_u8 attr)
{
    static int off[400], len[400];
    static char line[256];
    int n = ui_wrap(text, w, off, len, 400);
    int i;
    for (i = 0; i < h && scroll + i < n; ++i) {
        int l = len[scroll + i];
        cl_u8 a = attr;
        if (l >= (int)sizeof(line)) {
            l = (int)sizeof(line) - 1;
        }
        memcpy(line, text + off[scroll + i], (size_t)l);
        line[l] = '\0';
        /* simple markup: lines starting with '#' are headings, '>' are key hints */
        if (line[0] == '#') {
            a = (cl_u8)((attr & 0xF0) | C_YELLOW);
            fb_text(fb, x, y + i, line + (line[1] == ' ' ? 2 : 1), a, w);
        } else if (line[0] == '>') {
            a = (cl_u8)((attr & 0xF0) | C_LIGHTCYAN);
            fb_text(fb, x, y + i, line + (line[1] == ' ' ? 2 : 1), a, w);
        } else {
            fb_text(fb, x, y + i, line, a, w);
        }
    }
}
