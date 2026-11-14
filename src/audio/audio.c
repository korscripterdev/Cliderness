/* Cliderness - audio/audio.c : ClideAudio front end (PCM synthesis or tones) */
#include <string.h>
#include "audio/audio.h"
#include "audio/dsp.h"
#include "core/str.h"

/* PCM synthesis buffer in samples (none on 16-bit DOS: PC Speaker only) */
#ifdef CLIDE_TARGET_DOS16
#define CLIDE_PCM_BUFFER 0
#else
#define CLIDE_PCM_BUFFER 44100L
#endif

typedef struct Tone {
    short hz;
    short ms;
} Tone;

static const Tone s_step[] = { { 90, 12 }, { 0, 0 } };
static const Tone s_mine[] = { { 220, 15 }, { 180, 15 }, { 0, 0 } };
static const Tone s_break[] = { { 300, 20 }, { 200, 20 }, { 120, 30 }, { 0, 0 } };
static const Tone s_place[] = { { 160, 25 }, { 0, 0 } };
static const Tone s_pickup[] = { { 880, 30 }, { 1320, 40 }, { 0, 0 } };
static const Tone s_hurt[] = { { 150, 60 }, { 110, 80 }, { 0, 0 } };
static const Tone s_menu[] = { { 660, 20 }, { 0, 0 } };
static const Tone s_splash[] = { { 400, 15 }, { 250, 20 }, { 500, 15 }, { 0, 0 } };
static const Tone s_thunder[] = { { 60, 120 }, { 45, 160 }, { 70, 90 }, { 0, 0 } };
static const Tone s_crow[] = { { 700, 60 }, { 1, 30 }, { 650, 70 }, { 0, 0 } };
static const Tone s_bird[] = { { 2400, 25 }, { 3000, 25 }, { 2600, 30 }, { 0, 0 } };
/* wood: a creaking hull and a cracking plank */
static const Tone s_creak[] = { { 130, 40 }, { 150, 50 }, { 125, 60 }, { 0, 0 } };
static const Tone s_crack[] = { { 900, 10 }, { 240, 25 }, { 1, 10 }, { 180, 40 }, { 90, 60 }, { 0, 0 } };

/* per effect: PC Speaker tones, DSP renderer, PCM length (ms), volume (/256),
 * priority (a louder event interrupts a quieter one) */
typedef struct SfxInfo {
    const char *name;
    const Tone *tones;
    const char *dsp;
    short ms;
    short vol;
    short prio;
} SfxInfo;

static const SfxInfo g_sfx[SFX_COUNT] = {
    { "none", 0, 0, 0, 0, 0 },
    { "step", s_step, "step", 110, 150, 1 },
    { "mine", s_mine, "mining", 170, 200, 2 },
    { "break", s_break, "stone", 260, 230, 3 },
    { "place", s_place, "stone", 140, 160, 2 },
    { "pickup", s_pickup, 0, 0, 120, 4 },
    { "hurt", s_hurt, 0, 0, 170, 6 },
    { "menu", s_menu, 0, 0, 90, 5 },
    { "splash", s_splash, "waves", 450, 200, 3 },
    { "thunder", s_thunder, "thunder", 1600, 256, 7 },
    { "crow", s_crow, "crow", 520, 160, 4 },
    { "bird", s_bird, "bird", 450, 140, 2 },
    { "creak", s_creak, "wood_creak", 650, 220, 4 },
    { "crack", s_crack, "wood_break", 520, 256, 6 },
    { "rain", 0, "rain", 1800, 110, 1 },
    { "wind", 0, "wind", 1800, 120, 1 },
    { "storm", 0, "storm", 1800, 140, 1 },
    { "cave", 0, "cave", 1800, 110, 1 },
    { "ancient", 0, "ancient", 1800, 110, 1 },
    { "river", 0, "river", 1800, 100, 1 },
    { "fire", 0, "fire", 1500, 120, 1 }
};

static const ClidePlatform *g_p = 0;
static int g_available = 0;
static int g_enabled = 1;
static const Tone *g_cur = 0;
static long g_left = 0;
static int g_prio = 0;
static int g_use_pcm = 0;
static int g_caps = 0;        /* PCM_* of the back end */
static int g_stream = 0;      /* queue-only back end: the front end streams the mix */
static int g_streaming = 0;   /* lead-in already written */
static long g_pcm_left = 0;
static long g_played = 0;

#if CLIDE_PCM_BUFFER > 0
static cl_u32 g_seed = 1;
static short g_buf[CLIDE_PCM_BUFFER];
/* g_mix: what is still to be heard (flush back ends: what is playing now) */
static short g_mix[CLIDE_PCM_BUFFER];
static long g_mix_len = 0;
static void stream_out(long n);
#endif

void aud_init(const ClidePlatform *p, int enabled)
{
    g_p = p;
    g_enabled = enabled;
    g_available = p->audio.init();
#if CLIDE_PCM_BUFFER > 0
    g_use_pcm = g_available && p->audio.pcm != 0;
    g_caps = (g_use_pcm && p->audio.pcm_caps) ? p->audio.pcm_caps() : 0;
    g_stream = g_use_pcm && (g_caps & PCM_STREAM) != 0;
    g_streaming = 0;
#else
    g_use_pcm = 0;
#endif
    g_cur = 0;
    g_pcm_left = 0;
    g_played = 0;
}

void aud_shutdown(void)
{
    if (g_p && g_available) {
        g_p->audio.tone(0);
        g_p->audio.shutdown();
    }
    g_available = 0;
    g_use_pcm = 0;
}

void aud_set_enabled(int on)
{
    g_enabled = on;
    if (!on) {
        aud_silence();
    }
}

int aud_enabled(void) { return g_enabled; }
int aud_available(void) { return g_available; }
int aud_pcm(void) { return g_use_pcm; }
long aud_played(void) { return g_played; }

const char *aud_backend(void)
{
    return g_p ? g_p->audio.name() : "none";
}

int aud_find(const char *name)
{
    int i;
    for (i = 1; i < SFX_COUNT; ++i) {
        if (str_ieq(name, g_sfx[i].name)) return i;
    }
    return -1;
}

const char *aud_sfx_name(int sfx)
{
    return (sfx > 0 && sfx < SFX_COUNT) ? g_sfx[sfx].name : "none";
}

void aud_silence(void)
{
    if (g_p && g_available) {
        g_p->audio.tone(0);
    }
    g_cur = 0;
    g_left = 0;
    g_pcm_left = 0;
    g_prio = 0;
#if CLIDE_PCM_BUFFER > 0
    g_mix_len = 0;
    g_streaming = 0;
#endif
}

#if CLIDE_PCM_BUFFER > 0
/* square-wave rendering of a tone sequence (effects without a DSP voice) */
static long render_tones(const Tone *t, short *out, long max)
{
    long n = 0;
    cl_u32 ph = 0;
    for (; t->ms > 0 && n < max; ++t) {
        long k, len = (long)t->ms * CLIDE_PCM_RATE / 1000L;
        cl_u32 inc = t->hz > 1 ? dsp_freq_inc(t->hz, CLIDE_PCM_RATE) : 0;
        for (k = 0; k < len && n < max; ++k) {
            /* short fade in/out against clicks */
            long edge = k < 64 ? k : (len - k < 64 ? len - k : 64);
            out[n++] = (short)(inc ? (long)dsp_osc(&ph, inc, WAVE_SQUARE) / 3L * edge / 64L : 0);
        }
    }
    return n;
}


/* write the next n samples of the mix to a streaming back end */
static void stream_out(long n)
{
    if (n > g_mix_len) n = g_mix_len;
    if (n <= 0) {
        return;
    }
    g_p->audio.pcm(g_mix, n, 0);
    g_mix_len -= n;
    memmove(g_mix, g_mix + n, sizeof(short) * (size_t)g_mix_len);
}

static void play_pcm(int sfx)
{
    const SfxInfo *s = &g_sfx[sfx];
    long n, i, rest = 0, out;
    int busy = g_pcm_left > 0;
    if (s->dsp) {
        n = (long)s->ms * CLIDE_PCM_RATE / 1000L;
        if (n > CLIDE_PCM_BUFFER) n = CLIDE_PCM_BUFFER;
        g_seed = g_seed * 1103515245UL + 12345UL;
        n = dsp_render(s->dsp, g_buf, n, CLIDE_PCM_RATE, CL_U32(g_seed));
        /* fade the tail so cut sounds do not click */
        for (i = 0; i < 256 && i < n; ++i) {
            g_buf[n - 1 - i] = (short)((long)g_buf[n - 1 - i] * i / 256L);
        }
    } else {
        n = render_tones(s->tones, g_buf, CLIDE_PCM_BUFFER);
    }
    if (n <= 0) {
        return;
    }
    for (i = 0; i < n; ++i) {
        g_buf[i] = (short)((long)g_buf[i] * s->vol / 256L);
    }
    if (g_stream) {
        /* streaming: g_mix starts at the next sample to be written */
        out = CL_MAX(g_mix_len, n);
        for (i = 0; i < out; ++i) {
            long v = (i < g_mix_len ? (long)g_mix[i] : 0L) + (i < n ? (long)g_buf[i] : 0L);
            g_mix[i] = (short)CL_CLAMP(v, -32767L, 32767L);
        }
        g_mix_len = out;
        g_pcm_left = out * 1000L / CLIDE_PCM_RATE;
        g_prio = CL_MAX((int)s->prio, busy ? g_prio : 0);
        ++g_played;
        if (!g_streaming) {
            stream_out(CLIDE_PCM_RATE / 16L);   /* ~60 ms lead against underruns */
            g_streaming = 1;
        }
        return;
    }
    if (busy && !(g_caps & PCM_FLUSH) && s->prio <= g_prio) {
        return;   /* whole-sound back end (afplay): do not pile sounds up */
    }
    if (busy && (g_caps & PCM_FLUSH)) {
        /* mix the new sound into the part of the current one still to come */
        long done = (g_mix_len * 1000L / CLIDE_PCM_RATE - g_pcm_left) * CLIDE_PCM_RATE / 1000L;
        if (done < 0) done = 0;
        rest = g_mix_len - done;
        if (rest > 0) {
            memmove(g_mix, g_mix + done, sizeof(short) * (size_t)rest);
        } else {
            rest = 0;
        }
    }
    out = CL_MAX(rest, n);
    for (i = 0; i < out; ++i) {
        long v = (i < rest ? (long)g_mix[i] : 0L) + (i < n ? (long)g_buf[i] : 0L);
        g_mix[i] = (short)CL_CLAMP(v, -32767L, 32767L);
    }
    if (g_p->audio.pcm(g_mix, out, busy)) {
        g_mix_len = out;
        g_pcm_left = out * 1000L / CLIDE_PCM_RATE;
        g_prio = CL_MAX((int)s->prio, busy ? g_prio : 0);
        ++g_played;
    }
}
#endif

void aud_play(int sfx)
{
    if (!g_available || !g_enabled || sfx <= SFX_NONE || sfx >= SFX_COUNT) {
        return;
    }
#if CLIDE_PCM_BUFFER > 0
    if (g_use_pcm) {
        play_pcm(sfx);
        return;
    }
#endif
    if (!g_sfx[sfx].tones || (g_cur && g_sfx[sfx].prio < g_prio)) {
        return;
    }
    g_cur = g_sfx[sfx].tones;
    g_prio = g_sfx[sfx].prio;
    g_left = g_cur->ms;
    g_p->audio.tone(g_cur->hz);
    ++g_played;
}

void aud_update(long ms)
{
#if CLIDE_PCM_BUFFER > 0
    if (g_stream && g_streaming) {
        stream_out(ms * CLIDE_PCM_RATE / 1000L);
        if (g_mix_len == 0) {
            g_streaming = 0;
        }
    }
#endif
    if (g_pcm_left > 0) {
        g_pcm_left -= ms;
        if (g_pcm_left <= 0) {
            g_pcm_left = 0;
            g_prio = 0;
        }
    }
    if (!g_cur || !g_available) {
        return;
    }
    g_left -= ms;
    while (g_cur && g_left <= 0) {
        ++g_cur;
        if (g_cur->ms == 0) {
            g_cur = 0;
            g_prio = 0;
            g_p->audio.tone(0);
            return;
        }
        g_left += g_cur->ms;
        g_p->audio.tone(g_cur->hz);
    }
}
