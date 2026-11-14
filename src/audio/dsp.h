/*
 * Cliderness - audio/dsp.h  (ClideAudio Extreme DSP core)
 * Integer / fixed-point sound synthesis, deterministic for a given seed.
 * Samples are signed 16-bit values stored in short.
 */
#ifndef CLIDE_AUDIO_DSP_H
#define CLIDE_AUDIO_DSP_H

#include "core/types.h"

enum { WAVE_SQUARE = 0, WAVE_SAW, WAVE_TRIANGLE, WAVE_SINE };

typedef struct DspNoise {
    cl_u32 s;
    long   p0, p1, p2;   /* pink filter state */
    long   brown;
} DspNoise;

typedef struct DspEnv {
    long attack, decay, sustain, release;  /* samples; sustain level 0..32767 */
    long t;
    long release_at;
} DspEnv;

void dsp_noise_init(DspNoise *n, cl_u32 seed);
int  dsp_white(DspNoise *n);
int  dsp_pink(DspNoise *n);
int  dsp_brown(DspNoise *n);
/* phase accumulator oscillator; inc = freq * 65536 / rate (16.16) */
int  dsp_osc(cl_u32 *phase, cl_u32 inc, int wave);
cl_u32 dsp_freq_inc(long hz, long rate);
/* one-pole low pass, coef 1..256 (256 = no filtering) */
int  dsp_lowpass(long *state, int in, int coef);
/* envelope level 0..32767 at the current time; advances t */
long dsp_env(DspEnv *e);
/* distance attenuation: gain 0..256 for a distance in blocks */
int  dsp_distance_gain(long dist, int occluders);

/* procedural sound renderers; return number of samples written */
long dsp_render(const char *name, short *out, long n, long rate, cl_u32 seed);
const char *dsp_sound_name(int i);   /* NULL after the last */
int  dsp_write_wav(const char *path, const short *s, long n, long rate);

#endif
