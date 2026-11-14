/* Cliderness - audio/audio.c : PC Speaker style tone sequencer */
#include "audio/audio.h"

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

static const Tone *const g_seq[SFX_COUNT] = {
    0, s_step, s_mine, s_break, s_place, s_pickup, s_hurt, s_menu, s_splash, s_thunder, s_crow, s_bird,
    s_creak, s_crack
};

static const ClidePlatform *g_p = 0;
static int g_available = 0;
static int g_enabled = 1;
static const Tone *g_cur = 0;
static long g_left = 0;
static int g_prio = 0;

void aud_init(const ClidePlatform *p, int enabled)
{
    g_p = p;
    g_enabled = enabled;
    g_available = p->audio.init();
    g_cur = 0;
}

void aud_shutdown(void)
{
    if (g_p && g_available) {
        g_p->audio.tone(0);
        g_p->audio.shutdown();
    }
    g_available = 0;
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

const char *aud_backend(void)
{
    return g_p ? g_p->audio.name() : "none";
}

void aud_silence(void)
{
    if (g_p && g_available) {
        g_p->audio.tone(0);
    }
    g_cur = 0;
    g_left = 0;
}

void aud_play(int sfx)
{
    /* simple priority mixing: a louder event interrupts a quieter one */
    static const int prio[SFX_COUNT] = { 0, 1, 2, 3, 2, 4, 6, 5, 3, 7, 4, 2, 4, 6 };
    if (!g_available || !g_enabled || sfx <= SFX_NONE || sfx >= SFX_COUNT) {
        return;
    }
    if (g_cur && prio[sfx] < g_prio) {
        return;
    }
    g_cur = g_seq[sfx];
    g_prio = prio[sfx];
    g_left = g_cur->ms;
    g_p->audio.tone(g_cur->hz);
}

void aud_update(long ms)
{
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
