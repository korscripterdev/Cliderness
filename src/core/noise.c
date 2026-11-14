/* Cliderness - core/noise.c */
#include "core/noise.h"
#include "core/rng.h"
#include "core/mathx.h"

#define NFRAC 32768L

static long lat2(cl_u32 seed, long ix, long iz)
{
    return (long)(rng_hash3(seed, ix, 0x5EEDL, iz) & 4095UL);
}

static long lat3(cl_u32 seed, long ix, long iy, long iz)
{
    return (long)(rng_hash3(seed ^ 0xA5A5A5A5UL, ix, iy, iz) & 4095UL);
}

/* smoothstep on 0..32768 */
static long smooth(long t)
{
    cl_u32 tt = ((cl_u32)t * (cl_u32)t) >> 15;
    return (long)((tt * (cl_u32)(3L * NFRAC - 2L * t)) >> 15);
}

/* a, b in 0..4095 (non-negative so plain division is exact) */
static long lerp(long a, long b, long t)
{
    return (a * (NFRAC - t) + b * t) / NFRAC;
}

static void split(long v, int shift, long *cell, long *frac)
{
    long size = 1L << shift;
    long c = mx_floor_div(v, size);
    long f = v - c * size;
    *cell = c;
    *frac = smooth((f * NFRAC) / size);
}

long noise2(cl_u32 seed, long x, long z, int shift)
{
    long ix, iz, tx, tz, a, b;
    if (shift <= 0) {
        return lat2(seed, x, z);
    }
    split(x, shift, &ix, &tx);
    split(z, shift, &iz, &tz);
    a = lerp(lat2(seed, ix, iz), lat2(seed, ix + 1, iz), tx);
    b = lerp(lat2(seed, ix, iz + 1), lat2(seed, ix + 1, iz + 1), tx);
    return lerp(a, b, tz);
}

long noise3(cl_u32 seed, long x, long y, long z, int shift)
{
    long ix, iy, iz, tx, ty, tz, a, b, c, d, e, f;
    if (shift <= 0) {
        return lat3(seed, x, y, z);
    }
    split(x, shift, &ix, &tx);
    split(y, shift, &iy, &ty);
    split(z, shift, &iz, &tz);
    a = lerp(lat3(seed, ix, iy, iz), lat3(seed, ix + 1, iy, iz), tx);
    b = lerp(lat3(seed, ix, iy, iz + 1), lat3(seed, ix + 1, iy, iz + 1), tx);
    c = lerp(lat3(seed, ix, iy + 1, iz), lat3(seed, ix + 1, iy + 1, iz), tx);
    d = lerp(lat3(seed, ix, iy + 1, iz + 1), lat3(seed, ix + 1, iy + 1, iz + 1), tx);
    e = lerp(a, b, tz);
    f = lerp(c, d, tz);
    return lerp(e, f, ty);
}

long fbm2(cl_u32 seed, long x, long z, int shift, int octaves)
{
    long total = 0;
    long wsum = 0;
    int i;
    for (i = 0; i < octaves && shift - i >= 0; ++i) {
        long w = 1L << (octaves - 1 - i);
        total += noise2(CL_U32(seed + (cl_u32)i * 1013UL), x, z, shift - i) * w;
        wsum += w;
    }
    if (wsum == 0) {
        return 0;
    }
    return total / wsum;
}
