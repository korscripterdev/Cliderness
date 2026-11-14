/* Cliderness - core/rng.c */
#include "core/rng.h"

cl_u32 rng_hash32(cl_u32 x)
{
    x = CL_U32(x);
    x ^= x >> 16;
    x = CL_U32(x * 0x7FEB352DUL);
    x ^= x >> 15;
    x = CL_U32(x * 0x846CA68BUL);
    x ^= x >> 16;
    return x;
}

cl_u32 rng_hash3(cl_u32 seed, long x, long y, long z)
{
    cl_u32 h = CL_U32(CL_U32(seed) * 0x9E3779B1UL);
    h = rng_hash32(h ^ CL_U32((cl_u32)x));
    h = rng_hash32(h ^ CL_U32(CL_U32((cl_u32)y) * 0x85EBCA6BUL));
    h = rng_hash32(h ^ CL_U32(CL_U32((cl_u32)z) * 0xC2B2AE35UL));
    return h;
}

cl_u32 rng_hash_str(const char *s)
{
    cl_u32 h = 2166136261UL;
    while (*s) {
        h ^= (cl_u32)(unsigned char)*s++;
        h = CL_U32(h * 16777619UL);
    }
    return h;
}

void rng_seed(ClideRng *r, cl_u32 seed)
{
    r->s = rng_hash32(seed);
    if (r->s == 0) {
        r->s = 0x6D2B79F5UL;
    }
}

cl_u32 rng_next(ClideRng *r)
{
    cl_u32 x = r->s;
    x ^= CL_U32(x << 13);
    x ^= x >> 17;
    x ^= CL_U32(x << 5);
    r->s = x;
    return x;
}

long rng_range(ClideRng *r, long n)
{
    if (n <= 1) {
        return 0;
    }
    return (long)(rng_next(r) % (cl_u32)n);
}

int rng_chance(ClideRng *r, long per_10000)
{
    return rng_range(r, 10000L) < per_10000;
}
