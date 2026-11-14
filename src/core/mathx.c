/* Cliderness - core/mathx.c */
#include "core/mathx.h"

static const short g_sin_q[65] = {
    0, 25, 50, 75, 100, 125, 150, 175, 200, 224, 249, 273, 297, 321, 345, 369,
    392, 415, 438, 460, 483, 505, 526, 548, 569, 590, 610, 630, 650, 669, 688, 706,
    724, 742, 759, 775, 792, 807, 822, 837, 851, 865, 878, 891, 903, 915, 926, 936,
    946, 955, 964, 972, 980, 987, 993, 999, 1004, 1009, 1013, 1016, 1019, 1021, 1023, 1024,
    1024
};

long mx_floor_div(long a, long b)
{
    if (b < 0) {
        a = -a;
        b = -b;
    }
    if (a >= 0) {
        return a / b;
    }
    return -((-a + b - 1) / b);
}

long mx_floor_mod(long a, long b)
{
    return a - mx_floor_div(a, b) * b;
}

long mx_abs(long a)
{
    return a < 0 ? -a : a;
}

long mx_isqrt(long v)
{
    long r = 0;
    long bit = 1L << 30;
    if (v <= 0) {
        return 0;
    }
    while (bit > v) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

int mx_sin256(int angle)
{
    int a = angle & 255;
    if (a < 64) {
        return g_sin_q[a];
    }
    if (a < 128) {
        return g_sin_q[128 - a];
    }
    if (a < 192) {
        return -g_sin_q[a - 128];
    }
    return -g_sin_q[256 - a];
}

int mx_cos256(int angle)
{
    return mx_sin256(angle + 64);
}
