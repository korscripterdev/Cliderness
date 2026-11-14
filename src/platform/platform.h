/*
 * Cliderness - platform/platform.h
 * Platform abstraction layer. Game logic never calls an OS API directly; it
 * only talks to the interfaces below. One ClidePlatform instance is selected
 * at start-up (native adapter or the headless adapter used by tests).
 *
 *   ClideTerminal   - screen setup, size, presenting the framebuffer
 *   ClideInput      - non-blocking key events
 *   ClideClock      - monotonic milliseconds, sleeping
 *   ClideFilesystem - directories, atomic replace
 *   ClideAudio      - tone / PCM back end (may be the null device)
 *   ClideLocale     - user language / code page detection
 *   ClideMemory     - free-memory query
 *   ClideRenderer   - see renderer/fb.h (portable, not per platform)
 */
#ifndef CLIDE_PLATFORM_H
#define CLIDE_PLATFORM_H

#include "core/types.h"
#include "renderer/fb.h"
#include "input/keys.h"

#define TERMCAP_UNICODE   1   /* can show non-CP437 Unicode text */
#define TERMCAP_RSHIFT    2   /* can report a bare Right Shift tap */
#define TERMCAP_BARE_MODS 4   /* can report a bare Ctrl tap */
#define TERMCAP_COLOR16   8   /* 16 foreground + 16 background colours */
#define TERMCAP_HEADLESS  16

typedef struct ClideTerminal {
    int    (*init)(void);
    void   (*shutdown)(void);
    void   (*get_size)(int *w, int *h);
    /* Emits cells that differ between back and front, then front = back. */
    void   (*present)(const Framebuffer *back, Framebuffer *front, int force);
    cl_u32 (*caps)(void);
} ClideTerminal;

typedef struct ClideInput {
    int (*poll)(ClideKeyEvent *ev);   /* 1 = event returned, 0 = none */
} ClideInput;

typedef struct ClideClock {
    cl_u32 (*ms)(void);
    void   (*sleep_ms)(cl_u32 ms);
    cl_u32 (*wall_seed)(void);        /* entropy for random world seeds */
} ClideClock;

typedef struct ClideFilesystem {
    int (*make_dir)(const char *path);            /* 0 ok (or exists) */
    int (*replace)(const char *tmp, const char *dst); /* 0 ok */
    char sep;
} ClideFilesystem;

typedef struct ClideAudio {
    int  (*init)(void);               /* 1 = device available */
    void (*shutdown)(void);
    void (*tone)(int hz);             /* 0 = silence */
    const char *(*name)(void);
} ClideAudio;

typedef struct ClideLocale {
    void (*detect)(char *lang, int n); /* e.g. "ko", "en", "zh-CN" */
    int  (*codepage)(void);            /* 437, 65001 (UTF-8), ... */
} ClideLocale;

typedef struct ClideMemory {
    cl_u32 (*free_bytes)(void);        /* 0 = unknown */
} ClideMemory;

typedef struct ClidePlatform {
    const char     *name;
    ClideTerminal   term;
    ClideInput      input;
    ClideClock      clock;
    ClideFilesystem fs;
    ClideAudio      audio;
    ClideLocale     locale;
    ClideMemory     memory;
} ClidePlatform;

/* Provided by exactly one native adapter (windows / unix / dos). */
const ClidePlatform *plat_native(void);
/* Headless adapter (platform/null): scripted input, fake clock, real fs. */
const ClidePlatform *plat_headless(void);

/* Headless control (tests and --headless). */
void headless_reset(void);
void headless_push(long key, int mods);
void headless_push_wait(int frames);
int  headless_script(const char *script);   /* returns 0 on parse error */
int  headless_done(void);
const Framebuffer *headless_screen(void);
void headless_dump(void *file);              /* FILE* */

#endif
