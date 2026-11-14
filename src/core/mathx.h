/*
 * Cliderness - core/mathx.h
 * Integer math helpers. C90 leaves the rounding of negative division and
 * right-shift of negative values implementation-defined, so all floor
 * operations go through these helpers to stay identical on every compiler.
 */
#ifndef CLIDE_CORE_MATHX_H
#define CLIDE_CORE_MATHX_H

#include "core/types.h"

long mx_floor_div(long a, long b);
long mx_floor_mod(long a, long b);
long mx_abs(long a);
long mx_isqrt(long v);
/* angle: 0..255 = one full turn. Returns sin * 1024 (-1024..1024). */
int  mx_sin256(int angle);
int  mx_cos256(int angle);

#endif
