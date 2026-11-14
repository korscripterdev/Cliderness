/*
 * Cliderness - core/noise.h
 * Integer value noise (fixed point, no floating point) so world generation
 * is identical on FPU-less 16-bit DOS machines and modern 64-bit systems.
 * All functions return 0..4095.
 */
#ifndef CLIDE_CORE_NOISE_H
#define CLIDE_CORE_NOISE_H

#include "core/types.h"

#define NOISE_MAX 4095L

/* cell size = 1 << shift (shift 0..12) */
long noise2(cl_u32 seed, long x, long z, int shift);
long noise3(cl_u32 seed, long x, long y, long z, int shift);
/* fractal sum of octaves, octave i uses shift-i */
long fbm2(cl_u32 seed, long x, long z, int shift, int octaves);

#endif
