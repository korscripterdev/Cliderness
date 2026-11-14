/*
 * Cliderness - platform/dos/dos_platform.c
 * MS-DOS / FreeDOS adapter.
 *
 *  - Open Watcom 16-bit (wcl -ml, real mode, large memory model)
 *  - Open Watcom 32-bit (DOS/4GW)  and  DJGPP (DPMI, CWSDPMI)
 *
 * Video: writes directly to VGA text memory at B800:0000 (80x25, mode 03h),
 *        blink disabled so all 16 background colours are available.
 * Input: BIOS INT 16h enhanced keyboard services; shift status flags are
 *        polled to detect a Right Shift / Ctrl tap.
 * Audio: PC Speaker through PIT channel 2 (ports 42h/43h/61h).
 * Clock: BIOS tick counter via clock() (~55 ms resolution; the fixed-step
 *        game loop accumulates real elapsed time so the rate stays correct).
 */
#if defined(__MSDOS__) || defined(__DOS__) || defined(__DJGPP__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dos.h>
#include <bios.h>
#include "platform/platform.h"
#include "renderer/cp437.h"
#include "core/str.h"

#if defined(__DJGPP__)
#include <go32.h>
#include <sys/farptr.h>
#include <sys/stat.h>
#include <pc.h>
#include <dpmi.h>
#define VGA_POKE(i, v) _farpokew(_dos_ds, 0xB8000UL + (unsigned long)(i) * 2UL, (unsigned short)(v))
#define PORT_IN(p) inportb(p)
#define PORT_OUT(p, v) outportb((p), (unsigned char)(v))
#define DOS_MKDIR(p) mkdir((p), 0755)
#else /* Open Watcom */
#include <conio.h>
#include <direct.h>
#include <i86.h>
#include <malloc.h>
#if defined(__386__)
static unsigned short *g_vga = (unsigned short *)0xB8000UL;
#else
static unsigned short __far *g_vga = (unsigned short __far *)MK_FP(0xB800, 0);
#endif
#define VGA_POKE(i, v) (g_vga[(i)] = (unsigned short)(v))
#define PORT_IN(p) inp(p)
#define PORT_OUT(p, v) outp((p), (v))
#define DOS_MKDIR(p) mkdir(p)
#endif

#ifndef _NKEYBRD_READ
#define _NKEYBRD_READ        0x10
#define _NKEYBRD_READY       0x11
#define _NKEYBRD_SHIFTSTATUS 0x12
#endif

static int g_inited = 0;
static int g_prev_shift = 0;
static int g_other = 0;
static long g_pending = CK_NONE;
static int g_speaker = 0;

static void bios_video(unsigned short ax, unsigned short bx, unsigned short cx)
{
    union REGS r;
    memset(&r, 0, sizeof(r));
    r.x.ax = ax;
    r.x.bx = bx;
    r.x.cx = cx;
#if defined(__DJGPP__) || !defined(__386__)
    int86(0x10, &r, &r);
#else
    int386(0x10, &r, &r);
#endif
}

static int dos_init(void)
{
    bios_video(0x0003, 0, 0);      /* 80x25 colour text */
    bios_video(0x1003, 0x0000, 0); /* blink off -> 16 background colours */
    bios_video(0x0100, 0, 0x2000); /* hide cursor */
    g_inited = 1;
    return 1;
}

static void dos_shutdown(void)
{
    if (!g_inited) {
        return;
    }
    if (g_speaker) {
        PORT_OUT(0x61, PORT_IN(0x61) & 0xFC);
    }
    bios_video(0x1003, 0x0001, 0);
    bios_video(0x0100, 0, 0x0607);
    bios_video(0x0003, 0, 0);
    g_inited = 0;
}

static void dos_get_size(int *w, int *h)
{
    *w = 80;
    *h = 25;
}

static void dos_present(const Framebuffer *back, Framebuffer *front, int force)
{
    int i;
    for (i = 0; i < FB_CELLS; ++i) {
        const FbCell *b = &back->cells[i];
        FbCell *f = &front->cells[i];
        if (force || b->ch != f->ch || b->attr != f->attr || b->flags != f->flags) {
            unsigned short ch = (b->flags & (FBC_UNI | FBC_CONT)) ? (unsigned short)'?' : b->ch;
            if (b->flags & FBC_CONT) {
                ch = ' ';
            }
            VGA_POKE(i, ((unsigned short)b->attr << 8) | (ch & 0xFF));
            *f = *b;
        }
    }
}

static cl_u32 dos_caps(void)
{
    return TERMCAP_RSHIFT | TERMCAP_BARE_MODS | TERMCAP_COLOR16;
}

static long map_scan(int scan)
{
    switch (scan) {
    case 0x48: return CK_UP;
    case 0x50: return CK_DOWN;
    case 0x4B: return CK_LEFT;
    case 0x4D: return CK_RIGHT;
    case 0x47: return CK_HOME;
    case 0x4F: return CK_END;
    case 0x49: return CK_PGUP;
    case 0x51: return CK_PGDN;
    case 0x52: return CK_INSERT;
    case 0x53: return CK_DELETE;
    case 0x85: return CK_F1 + 10;
    case 0x86: return CK_F1 + 11;
    default: break;
    }
    if (scan >= 0x3B && scan <= 0x44) {
        return CK_F1 + (long)(scan - 0x3B);
    }
    return CK_NONE;
}

static int dos_poll(ClideKeyEvent *ev)
{
    unsigned shift;
    ev->mods = 0;
    if (g_pending != CK_NONE) {
        ev->key = g_pending;
        g_pending = CK_NONE;
        return 1;
    }
    shift = _bios_keybrd(_NKEYBRD_SHIFTSTATUS) & 0x07;
    /* bit0 right shift, bit1 left shift, bit2 ctrl */
    if (shift != (unsigned)g_prev_shift) {
        int released = g_prev_shift & ~(int)shift;
        if (((int)shift & ~g_prev_shift) != 0) {
            g_other = 0;
        }
        g_prev_shift = (int)shift;
        if (released && !g_other && shift == 0) {
            if (released & 1) {
                ev->key = CK_RSHIFT;
                return 1;
            }
            if (released & 4) {
                ev->key = CK_CTRL;
                return 1;
            }
        }
    }
    if (_bios_keybrd(_NKEYBRD_READY)) {
        unsigned k = _bios_keybrd(_NKEYBRD_READ);
        int ascii = (int)(k & 0xFF);
        int scan = (int)((k >> 8) & 0xFF);
        g_other = 1;
        if (shift & 3) {
            ev->mods |= KM_SHIFT;
        }
        if (shift & 4) {
            ev->mods |= KM_CTRL;
        }
        if (ascii == 0 || ascii == 0xE0) {
            ev->key = map_scan(scan);
            return ev->key != CK_NONE;
        }
        if (ascii == 3) {
            ev->key = CK_QUIT;
            return 1;
        }
        if (ascii >= 1 && ascii <= 26 && ascii != 8 && ascii != 9 && ascii != 13) {
            ev->key = 'a' + ascii - 1;
            ev->mods |= KM_CTRL;
            return 1;
        }
        if (ascii >= 0x80) {
            ev->key = cp437_to_unicode(ascii);
            return 1;
        }
        ev->key = ascii;
        return 1;
    }
    return 0;
}

static cl_u32 dos_ms(void)
{
    return CL_U32((cl_u32)clock() * (1000UL / (cl_u32)CLOCKS_PER_SEC));
}

static void dos_sleep(cl_u32 ms)
{
    if (ms > 0) {
        delay((unsigned)ms);
    }
}

static cl_u32 dos_seed(void)
{
    return CL_U32((cl_u32)time(NULL) ^ ((cl_u32)clock() << 7));
}

static int dos_mkdir(const char *path)
{
    DOS_MKDIR(path);
    return 0;
}

static int dos_replace(const char *tmp, const char *dst)
{
    /* DOS rename() cannot overwrite: keep a .bak until the new file is in place */
    char bak[96];
    char *dot;
    str_copy(bak, dst, sizeof(bak));
    dot = strrchr(bak, '.');
    if (dot) {
        *dot = '\0';
    }
    str_cat(bak, ".bak", sizeof(bak));
    remove(bak);
    rename(dst, bak);
    if (rename(tmp, dst) != 0) {
        rename(bak, dst);
        return -1;
    }
    remove(bak);
    return 0;
}

static int aud_init(void)
{
    g_speaker = 1;
    return 1;
}

static void aud_shutdown(void)
{
    PORT_OUT(0x61, PORT_IN(0x61) & 0xFC);
}

static void aud_tone(int hz)
{
    if (hz <= 20) {
        PORT_OUT(0x61, PORT_IN(0x61) & 0xFC);
        return;
    }
    {
        unsigned long div = 1193180UL / (unsigned long)hz;
        PORT_OUT(0x43, 0xB6);
        PORT_OUT(0x42, (int)(div & 0xFF));
        PORT_OUT(0x42, (int)((div >> 8) & 0xFF));
        PORT_OUT(0x61, PORT_IN(0x61) | 3);
    }
}

static const char *aud_name(void)
{
    return "PC Speaker";
}

static void dos_detect(char *lang, int n)
{
    /* CP437 cannot display Hangul/Kana/Han/Cyrillic: default to English;
     * Latin languages that fit CP437 can be chosen in Settings. */
    str_copy(lang, "en", (size_t)n);
}

static int dos_codepage(void)
{
    return 437;
}

static cl_u32 dos_free(void)
{
#if defined(__DJGPP__)
    return (cl_u32)_go32_dpmi_remaining_physical_memory();
#elif defined(__WATCOMC__) && !defined(__386__)
    return (cl_u32)_memavl();
#else
    return 0;
#endif
}

static const ClidePlatform g_dos = {
    "MS-DOS / FreeDOS (VGA text)",
    { dos_init, dos_shutdown, dos_get_size, dos_present, dos_caps },
    { dos_poll },
    { dos_ms, dos_sleep, dos_seed },
    { dos_mkdir, dos_replace, '\\' },
    { aud_init, aud_shutdown, aud_tone, aud_name, NULL, 0 },
    { dos_detect, dos_codepage },
    { dos_free }
};

const ClidePlatform *plat_native(void)
{
    return &g_dos;
}

#endif
