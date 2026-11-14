/*
 * Cliderness - platform/unix/posix_platform.c
 * POSIX terminal adapter for Linux, macOS, FreeBSD, OpenBSD, NetBSD and other
 * Unix systems: termios raw mode, ANSI/VT escape output, UTF-8 glyphs.
 * Terminals cannot report bare modifier keys, so Right Shift / Ctrl taps are
 * not available (TERMCAP_RSHIFT is not set; alternative keys are bound).
 */
#if !defined(_WIN32) && !defined(__MSDOS__) && !defined(__DOS__) && !defined(__DJGPP__)

#define _POSIX_C_SOURCE 200112L
#define _DARWIN_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "platform/platform.h"
#include "renderer/cp437.h"
#include "core/str.h"

static struct termios g_old;
static int g_raw = 0;
static volatile sig_atomic_t g_resized = 0;
static char g_out[16384];
static size_t g_olen = 0;
static unsigned char g_inbuf[64];
static int g_inlen = 0;

static void out_flush(void)
{
    size_t off = 0;
    while (off < g_olen) {
        ssize_t w = write(STDOUT_FILENO, g_out + off, g_olen - off);
        if (w <= 0) {
            if (w < 0 && errno == EINTR) {
                continue;
            }
            break;
        }
        off += (size_t)w;
    }
    g_olen = 0;
}

static void out_str(const char *s)
{
    size_t n = strlen(s);
    if (g_olen + n >= sizeof(g_out)) {
        out_flush();
    }
    memcpy(g_out + g_olen, s, n);
    g_olen += n;
}

static void on_winch(int sig)
{
    (void)sig;
    g_resized = 1;
}

static void restore(void)
{
    if (g_raw) {
        out_str("\033[0m\033[?25h\033[?1049l");
        out_flush();
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_old);
        g_raw = 0;
    }
}

static int px_init(void)
{
    struct termios raw;
    struct sigaction sa;
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &g_old) != 0) {
        return 0;
    }
    raw = g_old;
    raw.c_iflag &= ~(tcflag_t)(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(tcflag_t)(OPOST);
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return 0;
    }
    g_raw = 1;
    atexit(restore);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_winch;
    sigaction(SIGWINCH, &sa, NULL);
    out_str("\033[?1049h\033[?25l\033[2J\033]0;Cliderness - by korscripterdev\007");
    out_flush();
    return 1;
}

static void px_shutdown(void)
{
    restore();
}

static void px_get_size(int *w, int *h)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        *w = ws.ws_col;
        *h = ws.ws_row;
    } else {
        *w = 80;
        *h = 25;
    }
}

/* VGA order -> ANSI order */
static const int g_ansi[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

static void sgr(cl_u8 attr)
{
    char buf[32];
    int fg = ATTR_FG(attr);
    int bg = ATTR_BG(attr);
    sprintf(buf, "\033[0;%d;%dm",
            (fg & 8) ? 90 + g_ansi[fg & 7] : 30 + g_ansi[fg & 7],
            (bg & 8) ? 100 + g_ansi[bg & 7] : 40 + g_ansi[bg & 7]);
    out_str(buf);
}

static void px_present(const Framebuffer *back, Framebuffer *front, int force)
{
    int x, y;
    int cur_x = -1, cur_y = -1;
    int last_attr = -1;
    char buf[32];
    if (g_resized) {
        g_resized = 0;
        force = 1;
        out_str("\033[0m\033[2J");
    }
    for (y = 0; y < FB_H; ++y) {
        for (x = 0; x < FB_W; ++x) {
            const FbCell *b = &back->cells[y * FB_W + x];
            FbCell *f = &front->cells[y * FB_W + x];
            long cp;
            int len;
            if (!force && b->ch == f->ch && b->attr == f->attr && b->flags == f->flags) {
                continue;
            }
            *f = *b;
            if (b->flags & FBC_CONT) {
                continue;
            }
            if (cur_x != x || cur_y != y) {
                sprintf(buf, "\033[%d;%dH", y + 1, x + 1);
                out_str(buf);
            }
            if ((int)b->attr != last_attr) {
                sgr(b->attr);
                last_attr = b->attr;
            }
            cp = (b->flags & FBC_UNI) ? (long)b->ch : cp437_to_unicode(b->ch);
            if (cp < 0x20L) {
                cp = ' ';
            }
            len = utf8_encode(cp, buf);
            buf[len] = '\0';
            out_str(buf);
            cur_y = y;
            cur_x = x + uni_width(cp);
            if (uni_width(cp) == 2 && x + 1 < FB_W) {
                front->cells[y * FB_W + x + 1] = back->cells[y * FB_W + x + 1];
            }
        }
    }
    out_flush();
}

static cl_u32 px_caps(void)
{
    const char *v = getenv("LC_ALL");
    cl_u32 caps = TERMCAP_COLOR16;
    if (!v || !*v) {
        v = getenv("LC_CTYPE");
    }
    if (!v || !*v) {
        v = getenv("LANG");
    }
    if (v && (str_icontains(v, "utf-8") || str_icontains(v, "utf8"))) {
        caps |= TERMCAP_UNICODE;
    }
    return caps;
}

static int read_more(int wait_ms)
{
    ssize_t n;
    if (wait_ms > 0) {
        fd_set fds;
        struct timeval tv;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = wait_ms * 1000;
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) <= 0) {
            return 0;
        }
    }
    if (g_inlen >= (int)sizeof(g_inbuf)) {
        return 0;
    }
    n = read(STDIN_FILENO, g_inbuf + g_inlen, sizeof(g_inbuf) - (size_t)g_inlen);
    if (n > 0) {
        g_inlen += (int)n;
        return (int)n;
    }
    return 0;
}

static void consume(int n)
{
    memmove(g_inbuf, g_inbuf + n, (size_t)(g_inlen - n));
    g_inlen -= n;
}

static long csi_tilde(int num)
{
    switch (num) {
    case 1: case 7: return CK_HOME;
    case 2: return CK_INSERT;
    case 3: return CK_DELETE;
    case 4: case 8: return CK_END;
    case 5: return CK_PGUP;
    case 6: return CK_PGDN;
    case 11: return CK_F1;
    case 12: return CK_F1 + 1;
    case 13: return CK_F1 + 2;
    case 14: return CK_F1 + 3;
    case 15: return CK_F1 + 4;
    case 17: return CK_F1 + 5;
    case 18: return CK_F1 + 6;
    case 19: return CK_F1 + 7;
    case 20: return CK_F1 + 8;
    case 21: return CK_F1 + 9;
    case 23: return CK_F1 + 10;
    case 24: return CK_F1 + 11;
    default: return CK_NONE;
    }
}

static int parse_escape(ClideKeyEvent *ev)
{
    int i;
    if (g_inlen == 1) {
        read_more(25);
    }
    if (g_inlen == 1) {
        consume(1);
        ev->key = CK_ESC;
        return 1;
    }
    if (g_inbuf[1] == 'O' && g_inlen >= 3) {
        unsigned char c = g_inbuf[2];
        consume(3);
        switch (c) {
        case 'P': ev->key = CK_F1; return 1;
        case 'Q': ev->key = CK_F1 + 1; return 1;
        case 'R': ev->key = CK_F1 + 2; return 1;
        case 'S': ev->key = CK_F1 + 3; return 1;
        case 'A': ev->key = CK_UP; return 1;
        case 'B': ev->key = CK_DOWN; return 1;
        case 'C': ev->key = CK_RIGHT; return 1;
        case 'D': ev->key = CK_LEFT; return 1;
        case 'H': ev->key = CK_HOME; return 1;
        case 'F': ev->key = CK_END; return 1;
        default: return 0;
        }
    }
    if (g_inbuf[1] == '[') {
        int num = 0, mod = 0, semi = 0;
        for (i = 2; i < g_inlen; ++i) {
            unsigned char c = g_inbuf[i];
            if (c >= '0' && c <= '9') {
                if (semi) {
                    mod = mod * 10 + (c - '0');
                } else {
                    num = num * 10 + (c - '0');
                }
                continue;
            }
            if (c == ';') {
                semi = 1;
                continue;
            }
            consume(i + 1);
            if (mod == 2) {
                ev->mods |= KM_SHIFT;
            } else if (mod == 5) {
                ev->mods |= KM_CTRL;
            }
            switch (c) {
            case 'A': ev->key = CK_UP; return 1;
            case 'B': ev->key = CK_DOWN; return 1;
            case 'C': ev->key = CK_RIGHT; return 1;
            case 'D': ev->key = CK_LEFT; return 1;
            case 'H': ev->key = CK_HOME; return 1;
            case 'F': ev->key = CK_END; return 1;
            case 'P': ev->key = CK_F1; return 1;
            case 'Q': ev->key = CK_F1 + 1; return 1;
            case 'R': ev->key = CK_F1 + 2; return 1;
            case 'S': ev->key = CK_F1 + 3; return 1;
            case '~': ev->key = csi_tilde(num); return ev->key != CK_NONE;
            default: return 0;
            }
        }
        consume(g_inlen);
        return 0;
    }
    /* Alt+key */
    consume(1);
    ev->key = CK_ESC;
    return 1;
}

static int px_poll(ClideKeyEvent *ev)
{
    if (g_resized) {
        ev->key = CK_RESIZE;
        ev->mods = 0;
        return 1;
    }
    for (;;) {
        unsigned char c;
        if (g_inlen == 0 && read_more(0) == 0) {
            return 0;
        }
        c = g_inbuf[0];
        ev->mods = 0;
        if (c == 27) {
            if (parse_escape(ev)) {
                return 1;
            }
            continue;
        }
        if (c >= 0x80) {
            const char *p;
            long cp;
            int need = (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : 4;
            if (g_inlen < need) {
                read_more(10);
            }
            if (g_inlen < need) {
                consume(g_inlen);
                continue;
            }
            p = (const char *)g_inbuf;
            cp = utf8_next(&p);
            consume((int)(p - (const char *)g_inbuf));
            ev->key = cp;
            return 1;
        }
        consume(1);
        if (c == 127 || c == 8) {
            ev->key = CK_BACKSPACE;
        } else if (c == '\r' || c == '\n') {
            ev->key = CK_ENTER;
        } else if (c == '\t') {
            ev->key = CK_TAB;
        } else if (c == 3) {
            ev->key = CK_QUIT;
        } else if (c >= 1 && c <= 26) {
            ev->key = 'a' + c - 1;
            ev->mods = KM_CTRL;
        } else if (c < 32) {
            continue;
        } else {
            ev->key = c;
            if (c >= 'A' && c <= 'Z') {
                ev->mods = KM_SHIFT;
            }
        }
        return 1;
    }
}

static cl_u32 px_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return CL_U32((cl_u32)ts.tv_sec * 1000UL + (cl_u32)(ts.tv_nsec / 1000000L));
}

static void px_sleep(cl_u32 ms)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000UL);
    ts.tv_nsec = (long)(ms % 1000UL) * 1000000L;
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {
    }
}

static cl_u32 px_seed(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return CL_U32((cl_u32)tv.tv_sec * 1000003UL ^ (cl_u32)tv.tv_usec ^ (cl_u32)getpid());
}

static int px_mkdir(const char *path)
{
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
}

static int px_replace(const char *tmp, const char *dst)
{
    return rename(tmp, dst) == 0 ? 0 : -1;
}

/* ---- audio: a pipe to the system's PCM player ----
 * Linux / BSD: pacat (PulseAudio), pw-cat (PipeWire) or aplay (ALSA) reading
 * raw samples from a non-blocking pipe; macOS: afplay with short WAV files.
 * Without any of them the game runs silently. */
static FILE *g_au_pipe = NULL;
static int g_au_fd = -1;
static int g_au_mode = 0;            /* 0 none, 1 pipe, 2 afplay */
static const char *g_au_name = "none (no PCM player found)";
#ifdef __APPLE__
static int g_au_file = 0;
#endif

static int have_cmd(const char *probe)
{
    return system(probe) == 0;
}

static int aud_init(void)
{
    static const char *const probe[3] = {
        "command -v pacat >/dev/null 2>&1 && pactl info >/dev/null 2>&1",
        "command -v pw-cat >/dev/null 2>&1",
        "command -v aplay >/dev/null 2>&1 && aplay -l 2>/dev/null | grep -q card"
    };
    static const char *const cmd[3] = {
        "pacat --raw --format=s16le --rate=22050 --channels=1 --latency-msec=60 2>/dev/null",
        "pw-cat --playback --format=s16 --rate=22050 --channels=1 - 2>/dev/null",
        "aplay -q -t raw -f S16_LE -r 22050 -c 1 2>/dev/null"
    };
    static const char *const names[3] = { "PulseAudio (pacat) 22050 Hz", "PipeWire (pw-cat) 22050 Hz",
                                          "ALSA (aplay) 22050 Hz" };
    int i;
    if (g_au_mode) {
        return 1;
    }
    if (getenv("CLIDE_NO_AUDIO")) {
        return 0;
    }
    signal(SIGPIPE, SIG_IGN);
#ifdef __APPLE__
    if (have_cmd("command -v afplay >/dev/null 2>&1")) {
        g_au_mode = 2;
        g_au_name = "macOS afplay 22050 Hz";
        return 1;
    }
#endif
    for (i = 0; i < 3; ++i) {
        if (have_cmd(probe[i])) {
            g_au_pipe = popen(cmd[i], "w");
            if (g_au_pipe) {
                g_au_fd = fileno(g_au_pipe);
                fcntl(g_au_fd, F_SETFL, fcntl(g_au_fd, F_GETFL) | O_NONBLOCK);
                g_au_mode = 1;
                g_au_name = names[i];
                return 1;
            }
        }
    }
    return 0;
}

static void aud_shutdown(void)
{
    if (g_au_pipe) {
        pclose(g_au_pipe);
    }
    g_au_pipe = NULL;
    g_au_fd = -1;
    g_au_mode = 0;
}

static void aud_tone(int hz) { (void)hz; }

static int aud_pcm(const short *s, long n, int restart)
{
    (void)restart;
    if (n <= 0) {
        return 0;
    }
    if (g_au_mode == 1) {
        /* little-endian samples; the pipe never blocks the game */
        static unsigned char buf[CLIDE_PCM_RATE * 4L];
        long i, bytes = n * 2L > (long)sizeof(buf) ? (long)sizeof(buf) : n * 2L;
        ssize_t w;
        for (i = 0; i < bytes / 2; ++i) {
            buf[i * 2] = (unsigned char)(s[i] & 0xFF);
            buf[i * 2 + 1] = (unsigned char)((s[i] >> 8) & 0xFF);
        }
        w = write(g_au_fd, buf, (size_t)bytes);
        if (w < 0 && errno == EPIPE) {
            aud_shutdown();   /* the player went away */
            return 0;
        }
        return w > 0;
    }
#ifdef __APPLE__
    if (g_au_mode == 2) {
        char path[160], cmdline[220];
        const char *tmp = getenv("TMPDIR");
        FILE *f;
        long i;
        unsigned long rate = (unsigned long)CLIDE_PCM_RATE, len = (unsigned long)n * 2UL;
        unsigned char h[44];
        static const unsigned char tmpl[44] = {
            'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,
            0,0,0,0,0,0,0,0,2,0,16,0,'d','a','t','a',0,0,0,0 };
        memcpy(h, tmpl, 44);
        h[4] = (unsigned char)((len + 36UL) & 0xFF); h[5] = (unsigned char)(((len + 36UL) >> 8) & 0xFF);
        h[6] = (unsigned char)(((len + 36UL) >> 16) & 0xFF); h[7] = (unsigned char)(((len + 36UL) >> 24) & 0xFF);
        h[24] = (unsigned char)(rate & 0xFF); h[25] = (unsigned char)((rate >> 8) & 0xFF);
        h[28] = (unsigned char)((rate * 2UL) & 0xFF); h[29] = (unsigned char)(((rate * 2UL) >> 8) & 0xFF);
        h[30] = (unsigned char)(((rate * 2UL) >> 16) & 0xFF);
        h[40] = (unsigned char)(len & 0xFF); h[41] = (unsigned char)((len >> 8) & 0xFF);
        h[42] = (unsigned char)((len >> 16) & 0xFF); h[43] = (unsigned char)((len >> 24) & 0xFF);
        g_au_file = (g_au_file + 1) & 3;
        snprintf(path, sizeof(path), "%s/clide_sfx%d.wav", tmp ? tmp : "/tmp", g_au_file);
        f = fopen(path, "wb");
        if (!f) {
            return 0;
        }
        fwrite(h, 1, 44, f);
        for (i = 0; i < n; ++i) {
            fputc(s[i] & 0xFF, f);
            fputc((s[i] >> 8) & 0xFF, f);
        }
        fclose(f);
        snprintf(cmdline, sizeof(cmdline), "afplay '%s' >/dev/null 2>&1 &", path);
        return system(cmdline) == 0;
    }
#endif
    return 0;
}

/* pipes stream continuously; afplay plays whole sounds */
static int aud_caps(void) { return g_au_mode == 1 ? PCM_STREAM : 0; }

static const char *aud_name(void) { return g_au_name; }

static void px_detect(char *lang, int n)
{
    const char *v = getenv("LC_ALL");
    char tmp[16];
    if (!v || !*v) {
        v = getenv("LC_MESSAGES");
    }
    if (!v || !*v) {
        v = getenv("LANG");
    }
    if (!v || !*v || str_ieq(v, "C") || str_ieq(v, "POSIX")) {
        str_copy(lang, "en", (size_t)n);
        return;
    }
    str_copy(tmp, v, sizeof(tmp));
    if (str_iprefix(tmp, "zh_CN") || str_iprefix(tmp, "zh_SG")) {
        str_copy(lang, "zh-CN", (size_t)n);
    } else if (str_iprefix(tmp, "zh")) {
        str_copy(lang, "zh-TW", (size_t)n);
    } else {
        tmp[2] = '\0';
        str_lower(tmp);
        str_copy(lang, tmp, (size_t)n);
    }
}

static int px_codepage(void)
{
    return (px_caps() & TERMCAP_UNICODE) ? 65001 : 0;
}

static cl_u32 px_free(void)
{
    return 0;
}

static const ClidePlatform g_px = {
    "POSIX terminal",
    { px_init, px_shutdown, px_get_size, px_present, px_caps },
    { px_poll },
    { px_ms, px_sleep, px_seed },
    { px_mkdir, px_replace, '/' },
    { aud_init, aud_shutdown, aud_tone, aud_name, aud_pcm, aud_caps },
    { px_detect, px_codepage },
    { px_free }
};

const ClidePlatform *plat_native(void)
{
    return &g_px;
}

#endif
