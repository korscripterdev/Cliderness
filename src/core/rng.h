/*
 * Cliderness - core/rng.h
 * Deterministic hashing and pseudo random numbers. Results are bit-identical
 * on 16-, 32- and 64-bit targets (all intermediate values masked to 32 bits).
 */
#ifndef CLIDE_CORE_RNG_H
#define CLIDE_CORE_RNG_H

#include "core/types.h"

typedef struct ClideRng {
    cl_u32 s;
} ClideRng;

cl_u32 rng_hash32(cl_u32 x);
cl_u32 rng_hash3(cl_u32 seed, long x, long y, long z);
cl_u32 rng_hash_str(const char *s);

void   rng_seed(ClideRng *r, cl_u32 seed);
cl_u32 rng_next(ClideRng *r);
/* 0 .. n-1 (n > 0) */
long   rng_range(ClideRng *r, long n);
/* returns 1 with probability per_10000 / 10000 */
int    rng_chance(ClideRng *r, long per_10000);

#endif
