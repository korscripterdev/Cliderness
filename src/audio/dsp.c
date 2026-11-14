/* Cliderness - audio/dsp.c */
#include <stdio.h>
#include <string.h>
#include "audio/dsp.h"
#include "core/mathx.h"
#include "core/rng.h"
#include "core/str.h"

static int clip(long v)
{
    if (v > 32767L) return 32767;
    if (v < -32768L) return -32768;
    return (int)v;
}

void dsp_noise_init(DspNoise *n, cl_u32 seed)
{
    memset(n, 0, sizeof(*n));
    n->s = rng_hash32(seed) | 1UL;
}

int dsp_white(DspNoise *n)
{
    cl_u32 x = n->s;
    x ^= CL_U32(x << 13);
    x ^= x >> 17;
    x ^= CL_U32(x << 5);
    n->s = x;
    return (int)((long)((x >> 16) & 0xFFFFUL) - 32768L);
}

/* Paul Kellet's economy pink filter in fixed point (coefficients * 1024) */
int dsp_pink(DspNoise *n)
{
    long w = dsp_white(n);
    n->p0 = (n->p0 * 1015L) / 1024L + w * 102L / 1024L;
    n->p1 = (n->p1 * 984L) / 1024L + w * 309L / 1024L;
    n->p2 = (n->p2 * 589L) / 1024L + w * 1018L / 1024L;
    return clip((n->p0 + n->p1 + n->p2 + w * 189L / 1024L) / 4L);
}

int dsp_brown(DspNoise *n)
{
    long w = dsp_white(n);
    n->brown += w / 16L;
    n->brown = n->brown * 1020L / 1024L;   /* leak keeps it bounded */
    return clip(n->brown * 3L);
}

cl_u32 dsp_freq_inc(long hz, long rate)
{
    return (cl_u32)((hz * 65536L) / rate) << 0;
}

int dsp_osc(cl_u32 *phase, cl_u32 inc, int wave)
{
    long p;
    *phase = CL_U32(*phase + inc);
    p = (long)(*phase & 0xFFFFUL);    /* 0..65535 */
    switch (wave) {
    case WAVE_SQUARE:
        return p < 32768L ? 24000 : -24000;
    case WAVE_SAW:
        return (int)(p - 32768L);
    case WAVE_TRIANGLE:
        return (int)(p < 32768L ? (p * 2L - 32768L) : (98303L - p * 2L));
    default:
        return mx_sin256((int)(p >> 8)) * 31;
    }
}

int dsp_lowpass(long *state, int in, int coef)
{
    *state += ((long)in - *state) * (long)coef / 256L;
    return (int)*state;
}

long dsp_env(DspEnv *e)
{
    long t = e->t++;
    long lvl;
    if (e->release_at >= 0 && t >= e->release_at) {
        long r = t - e->release_at;
        if (r >= e->release) {
            return 0;
        }
        return e->sustain * (e->release - r) / (e->release ? e->release : 1);
    }
    if (t < e->attack) {
        return 32767L * t / (e->attack ? e->attack : 1);
    }
    t -= e->attack;
    if (t < e->decay) {
        lvl = 32767L - (32767L - e->sustain) * t / (e->decay ? e->decay : 1);
        return lvl;
    }
    return e->sustain;
}

int dsp_distance_gain(long dist, int occluders)
{
    long g;
    if (dist < 1) {
        dist = 1;
    }
    g = 256L * 4L / (dist + 3L);
    while (occluders-- > 0) {
        g = g * 2L / 3L;
    }
    return (int)CL_CLAMP(g, 0L, 256L);
}

static const char *const g_sounds[] = {
    "beep", "white", "pink", "brown", "wind", "rain", "storm", "thunder", "river", "waves",
    "fire", "wood_creak", "wood_break", "stone", "mining", "step", "bird", "crow", "cave", "ancient", NULL
};

const char *dsp_sound_name(int i)
{
    if (i < 0 || i >= (int)(sizeof(g_sounds) / sizeof(g_sounds[0]))) {
        return NULL;
    }
    return g_sounds[i];
}

long dsp_render(const char *name, short *out, long n, long rate, cl_u32 seed)
{
    DspNoise nz;
    long i;
    long lp = 0, lp2 = 0;
    cl_u32 ph = 0, ph2 = 0;
    DspEnv env;
    ClideRng r;
    dsp_noise_init(&nz, seed);
    rng_seed(&r, seed);
    memset(&env, 0, sizeof(env));
    env.release_at = -1;
    for (i = 0; i < n; ++i) {
        long v = 0;
        long t = i * 1000L / rate;   /* ms */
        if (str_ieq(name, "beep")) {
            v = dsp_osc(&ph, dsp_freq_inc(880, rate), WAVE_SQUARE) / 2;
        } else if (str_ieq(name, "white")) {
            v = dsp_white(&nz) / 3;
        } else if (str_ieq(name, "pink")) {
            v = dsp_pink(&nz);
        } else if (str_ieq(name, "brown")) {
            v = dsp_brown(&nz);
        } else if (str_ieq(name, "wind")) {
            /* brown noise with a slowly swept low pass = gusts */
            int gust = 20 + (mx_sin256((int)(t / 23L)) + 1024) * 40 / 2048;
            v = dsp_lowpass(&lp, dsp_brown(&nz), gust);
        } else if (str_ieq(name, "rain") || str_ieq(name, "storm")) {
            long drops = dsp_lowpass(&lp, dsp_white(&nz), 160) / 4;
            if (rng_range(&r, str_ieq(name, "storm") ? 40 : 200) == 0) {
                lp2 = 20000;
            }
            lp2 = lp2 * 90L / 100L;
            v = drops + (dsp_white(&nz) * lp2) / 32768L;
        } else if (str_ieq(name, "thunder")) {
            env.attack = rate / 50;
            env.decay = rate;
            env.sustain = 3000;
            env.release = rate;
            env.release_at = rate * 3 / 2;
            v = dsp_lowpass(&lp, dsp_brown(&nz), 12) * dsp_env(&env) / 4096L;
        } else if (str_ieq(name, "river")) {
            v = dsp_lowpass(&lp, dsp_pink(&nz), 90) + dsp_white(&nz) / 24;
        } else if (str_ieq(name, "waves")) {
            int swell = (mx_sin256((int)(t / 31L)) + 1024) * 200 / 2048 + 8;
            v = dsp_lowpass(&lp, dsp_pink(&nz), swell) * 2;
        } else if (str_ieq(name, "fire")) {
            v = dsp_lowpass(&lp, dsp_brown(&nz), 60);
            if (rng_range(&r, 900) == 0) {
                lp2 = 26000;
            }
            lp2 = lp2 * 70L / 100L;
            v += (dsp_white(&nz) * lp2) / 32768L;
        } else if (str_ieq(name, "wood_creak")) {
            long f = 90 + (mx_sin256((int)(t / 4L)) * 30) / 1024;
            v = dsp_lowpass(&lp, dsp_osc(&ph, dsp_freq_inc(f, rate), WAVE_SAW), 40);
        } else if (str_ieq(name, "wood_break") || str_ieq(name, "stone") || str_ieq(name, "mining")) {
            int coef = str_ieq(name, "wood_break") ? 70 : 150;
            env.attack = rate / 500;
            env.decay = rate / 6;
            env.sustain = 0;
            env.release = 1;
            env.release_at = rate / 5;
            if (str_ieq(name, "mining") && (i % (rate / 3)) == 0) {
                env.t = 0;
            }
            v = dsp_lowpass(&lp, dsp_white(&nz), coef) * dsp_env(&env) / 32768L;
        } else if (str_ieq(name, "step")) {
            if ((i % (rate / 2)) == 0) {
                env.t = 0;
            }
            env.attack = rate / 400;
            env.decay = rate / 20;
            env.sustain = 0;
            env.release = 1;
            env.release_at = rate / 15;
            v = dsp_lowpass(&lp, dsp_brown(&nz), 50) * dsp_env(&env) / 16384L;
        } else if (str_ieq(name, "bird")) {
            long chirp = (t % 400L);
            if (chirp < 120L) {
                long f = 2600 + chirp * 12;
                v = dsp_osc(&ph, dsp_freq_inc(f, rate), WAVE_SINE) / 2;
            }
        } else if (str_ieq(name, "crow")) {
            long caw = t % 700L;
            if (caw < 220L) {
                long f = 520 - caw;
                v = dsp_lowpass(&lp, clip((long)dsp_osc(&ph, dsp_freq_inc(f, rate), WAVE_SAW) / 2L +
                                              (long)dsp_white(&nz) / 3L), 90);
            }
        } else if (str_ieq(name, "cave")) {
            /* sparse drips with a simple feedback echo (reverb) */
            static short echo[4096];
            long e = i % 4096L;
            long drip = 0;
            if (i < 4096) {
                echo[e] = 0;
            }
            if (rng_range(&r, rate) == 0) {
                ph2 = 1;
            }
            if (ph2 > 0 && ph2 < (cl_u32)(rate / 40)) {
                drip = dsp_osc(&ph, dsp_freq_inc(1500 - (long)ph2 * 8, rate), WAVE_SINE) / 3;
                ++ph2;
            } else {
                ph2 = 0;
            }
            v = drip + echo[e] * 6L / 10L + dsp_lowpass(&lp2, dsp_brown(&nz), 8) / 6;
            echo[e] = (short)clip(v);
        } else if (str_ieq(name, "ancient")) {
            /* slow detuned drones with a breathing filter */
            long a = dsp_osc(&ph, dsp_freq_inc(55, rate), WAVE_TRIANGLE);
            long b = dsp_osc(&ph2, dsp_freq_inc(83, rate), WAVE_SINE);
            int open = 10 + (mx_sin256((int)(t / 40L)) + 1024) * 40 / 2048;
            v = dsp_lowpass(&lp, clip((a + b) / 2L + (long)dsp_pink(&nz) / 6L), open);
        } else {
            return 0;
        }
        out[i] = (short)clip(v);
    }
    return n;
}

static void put_le(FILE *f, cl_u32 v, int bytes)
{
    int i;
    for (i = 0; i < bytes; ++i) {
        fputc((int)((v >> (8 * i)) & 0xFFUL), f);
    }
}

int dsp_write_wav(const char *path, const short *s, long n, long rate)
{
    FILE *f = fopen(path, "wb");
    long i;
    cl_u32 bytes = (cl_u32)n * 2UL;
    if (!f) {
        return 0;
    }
    fwrite("RIFF", 1, 4, f);
    put_le(f, 36UL + bytes, 4);
    fwrite("WAVEfmt ", 1, 8, f);
    put_le(f, 16, 4);
    put_le(f, 1, 2);
    put_le(f, 1, 2);
    put_le(f, (cl_u32)rate, 4);
    put_le(f, (cl_u32)rate * 2UL, 4);
    put_le(f, 2, 2);
    put_le(f, 16, 2);
    fwrite("data", 1, 4, f);
    put_le(f, bytes, 4);
    for (i = 0; i < n; ++i) {
        put_le(f, (cl_u32)(unsigned short)s[i], 2);
    }
    return fclose(f) == 0;
}
