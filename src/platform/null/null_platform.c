/*
 * Cliderness - platform/null/null_platform.c
 * Headless adapter: scripted key input, deterministic fake clock, no audio,
 * in-memory "screen". Used by automated tests and `cliderness --headless`.
 * The real filesystem of the native adapter is reused.
 *
 * Script syntax: plain characters are typed as keys; braces name specials:
 *   {ENTER} {ESC} {TAB} {BS} {SPACE} {UP} {DOWN} {LEFT} {RIGHT} {PGUP} {PGDN}
 *   {HOME} {END} {DEL} {F1}..{F12} {RSHIFT} {CTRL} {CTRL+q} {WAIT:n}
 * Exactly one event is delivered per frame so the game sees every key.
 */
#include <stdio.h>
#include <string.h>
#include "platform/platform.h"
#include "core/str.h"

#define HQ_MAX 2048

typedef struct HEvent {
    long key;
    int  mods;
    int  wait;
} HEvent;

static HEvent g_q[HQ_MAX];
static int g_qh = 0, g_qt = 0;
static int g_delivered = 0;
static cl_u32 g_ms = 0;
static Framebuffer g_screen;
static ClidePlatform g_head;
static int g_built = 0;

void headless_reset(void)
{
    g_qh = g_qt = 0;
    g_delivered = 0;
    g_ms = 0;
    fb_clear(&g_screen, ATTR(C_LIGHTGRAY, C_BLACK));
}

static void push(long key, int mods, int wait)
{
    if (g_qt >= HQ_MAX) {
        return;
    }
    g_q[g_qt].key = key;
    g_q[g_qt].mods = mods;
    g_q[g_qt].wait = wait;
    ++g_qt;
}

void headless_push(long key, int mods)
{
    push(key, mods, 0);
}

void headless_push_wait(int frames)
{
    push(CK_NONE, 0, frames);
}

static long special(const char *name, int *mods)
{
    static const char *const names[] = {
        "ENTER", "ESC", "TAB", "BS", "SPACE", "UP", "DOWN", "LEFT", "RIGHT",
        "PGUP", "PGDN", "HOME", "END", "DEL", "RSHIFT", "CTRL"
    };
    static const long codes[] = {
        CK_ENTER, CK_ESC, CK_TAB, CK_BACKSPACE, CK_SPACE, CK_UP, CK_DOWN, CK_LEFT, CK_RIGHT,
        CK_PGUP, CK_PGDN, CK_HOME, CK_END, CK_DELETE, CK_RSHIFT, CK_CTRL
    };
    int i;
    long n;
    for (i = 0; i < (int)CL_COUNTOF(names); ++i) {
        if (str_ieq(name, names[i])) {
            return codes[i];
        }
    }
    if ((name[0] == 'F' || name[0] == 'f') && str_to_long(name + 1, &n) && n >= 1 && n <= 12) {
        return CK_F1 + n - 1;
    }
    if (str_iprefix(name, "CTRL+") && name[5] && !name[6]) {
        *mods = KM_CTRL;
        return (long)(unsigned char)name[5];
    }
    return CK_NONE;
}

int headless_script(const char *s)
{
    while (*s) {
        if (*s == '{') {
            char name[24];
            const char *e = strchr(s, '}');
            size_t len;
            long n;
            int mods = 0;
            long key;
            if (!e) {
                return 0;
            }
            len = (size_t)(e - s - 1);
            if (len >= sizeof(name)) {
                return 0;
            }
            memcpy(name, s + 1, len);
            name[len] = '\0';
            s = e + 1;
            if (str_iprefix(name, "WAIT:")) {
                if (!str_to_long(name + 5, &n) || n < 0) {
                    return 0;
                }
                push(CK_NONE, 0, (int)n);
                continue;
            }
            key = special(name, &mods);
            if (key == CK_NONE) {
                return 0;
            }
            push(key, mods, 0);
        } else {
            long cp = utf8_next(&s);
            push(cp, (cp >= 'A' && cp <= 'Z') ? KM_SHIFT : 0, 0);
        }
    }
    return 1;
}

int headless_done(void)
{
    return g_qh >= g_qt;
}

const Framebuffer *headless_screen(void)
{
    return &g_screen;
}

void headless_dump(void *file)
{
    FILE *f = (FILE *)file;
    char row[FB_W * 4 + 1];
    int y;
    for (y = 0; y < FB_H; ++y) {
        fb_dump_row(&g_screen, y, row, sizeof(row));
        fprintf(f, "%s\n", row);
    }
}

static int h_init(void) { return 1; }
static void h_shutdown(void) { }
static void h_size(int *w, int *h) { *w = FB_W; *h = FB_H; }

static void h_present(const Framebuffer *back, Framebuffer *front, int force)
{
    (void)force;
    memcpy(&g_screen, back, sizeof(Framebuffer));
    memcpy(front, back, sizeof(Framebuffer));
    g_delivered = 0;
}

static cl_u32 h_caps(void) { return TERMCAP_UNICODE | TERMCAP_RSHIFT | TERMCAP_BARE_MODS | TERMCAP_COLOR16 | TERMCAP_HEADLESS; }

static int h_poll(ClideKeyEvent *ev)
{
    HEvent *e;
    if (g_delivered || g_qh >= g_qt) {
        return 0;
    }
    e = &g_q[g_qh];
    if (e->wait > 0) {
        --e->wait;
        g_delivered = 1;
        if (e->wait == 0) {
            ++g_qh;
        }
        return 0;
    }
    if (e->key == CK_NONE) {
        ++g_qh;
        return 0;
    }
    ev->key = e->key;
    ev->mods = e->mods;
    ++g_qh;
    g_delivered = 1;
    return 1;
}

static cl_u32 h_ms(void) { return g_ms; }
static void h_sleep(cl_u32 ms) { g_ms += ms; }
static cl_u32 h_seed(void) { return 12345UL; }
/* headless audio: no device, but PCM requests are counted for tests */
static long g_pcm_sounds = 0, g_pcm_samples = 0;
static int a_init(void) { return 1; }
static void a_shutdown(void) { }
static void a_tone(int hz) { (void)hz; }
static const char *a_name(void) { return "headless (counted, silent)"; }
static int a_pcm(const short *s, long n, int interrupt)
{
    (void)s;
    (void)interrupt;
    ++g_pcm_sounds;
    g_pcm_samples += n;
    return 1;
}
long headless_pcm_sounds(void) { return g_pcm_sounds; }
long headless_pcm_samples(void) { return g_pcm_samples; }
static void h_detect(char *lang, int n) { str_copy(lang, "en", (size_t)n); }
static int h_cp(void) { return 65001; }
static cl_u32 h_free(void) { return 0; }

const ClidePlatform *plat_headless(void)
{
    if (!g_built) {
        const ClidePlatform *n = plat_native();
        g_head.name = "headless";
        g_head.term.init = h_init;
        g_head.term.shutdown = h_shutdown;
        g_head.term.get_size = h_size;
        g_head.term.present = h_present;
        g_head.term.caps = h_caps;
        g_head.input.poll = h_poll;
        g_head.clock.ms = h_ms;
        g_head.clock.sleep_ms = h_sleep;
        g_head.clock.wall_seed = h_seed;
        g_head.fs = n->fs;
        g_head.audio.init = a_init;
        g_head.audio.shutdown = a_shutdown;
        g_head.audio.tone = a_tone;
        g_head.audio.name = a_name;
        g_head.audio.pcm = a_pcm;
        g_head.audio.pcm_flush = 1;
        g_head.locale.detect = h_detect;
        g_head.locale.codepage = h_cp;
        g_head.memory.free_bytes = h_free;
        g_built = 1;
        headless_reset();
    }
    return &g_head;
}
