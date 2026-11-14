/* Cliderness - world/sky/sky.c */
#include <string.h>
#include "world/sky/sky.h"
#include "world/weather/weather.h"
#include "renderer/cp437.h"
#include "core/rng.h"
#include "core/noise.h"
#include "core/mathx.h"
#include "core/str.h"

#define NSTARS 140

static const char *const g_syl[] = {
    "Al", "be", "ra", "Cor", "va", "nis", "Th", "ul", "Ka", "ri", "on", "Sel",
    "dra", "Myr", "ta", "Ves", "pe", "lo", "Ux", "an", "Ir", "go", "Ze", "th"
};

const char *sky_constellation_name(cl_u32 seed, int i, char *buf, int n)
{
    cl_u32 h = rng_hash3(seed, i, 777, 0);
    int k, parts = 2 + (int)(h % 2UL);
    buf[0] = '\0';
    for (k = 0; k < parts; ++k) {
        str_cat(buf, g_syl[(h >> (k * 5)) % 24UL], (size_t)n);
    }
    if (buf[0] >= 'a' && buf[0] <= 'z') {
        buf[0] = (char)(buf[0] - 'a' + 'A');
    }
    return buf;
}

static void put(Framebuffer *fb, int x0, int y0, int w, int h, int x, int y, int g, int fg, int bg)
{
    if (x < 0 || y < 0 || x >= w || y >= h) {
        return;
    }
    fb_put(fb, x0 + x, y0 + y, g, ATTR(fg, bg));
}

static int bg_at(const Framebuffer *fb, int x, int y)
{
    return ATTR_BG(fb->cells[y * FB_W + x].attr);
}

int sky_veyra_phase(const Game *g)
{
    /* eight phases over three days */
    long t = g->time.day * 1440L + ctime_minute_of_day(&g->time);
    return (int)((t / 540L) % 8L);
}

void sky_render(Framebuffer *fb, Game *g, int x0, int y0, int w, int h, int show_names, SkyInfo *info)
{
    ClideDate d;
    const WeatherDef *wd = weather_cur(&g->weather);
    int light = ctime_daylight(&g->time);
    long minute = ctime_minute_of_day(&g->time);
    long anim = g->tick;
    cl_u32 seed = g->seed;
    int x, y, i;
    int sky_h = h - 2;
    int night = light < 25;
    int cloud = wd->cloud;
    int base_bg, low_bg;
    int climate_t = world_climate_temp(&g->world, g->p.dim, g->p.x, g->p.z);
    int solar_ecl, lunar_ecl;
    long wind_off;
    SkyInfo dummy;
    if (!info) {
        info = &dummy;
    }
    memset(info, 0, sizeof(*info));
    ctime_date(&g->time, &d);
    solar_ecl = d.moon_phase == 0 && (g->time.day % 90L) == (long)(seed % 90UL) && d.is_day;
    lunar_ecl = d.moon_phase == 4 && (g->time.day % 60L) == (long)(seed % 60UL);
    info->solar_eclipse = solar_ecl;
    info->lunar_eclipse = lunar_ecl;

    /* ---- background gradient ---- */
    if (g->p.dim == DIM_ANCIENT) {
        base_bg = night ? C_BLACK : C_MAGENTA;
        low_bg = night ? C_MAGENTA : C_BLUE;
    } else if (light >= 60) {
        base_bg = cloud > 70 ? C_DARKGRAY : C_LIGHTBLUE;
        low_bg = cloud > 70 ? C_LIGHTGRAY : C_CYAN;
    } else if (light >= 25) {
        base_bg = C_BLUE;
        low_bg = (minute < 720) ? C_MAGENTA : C_RED;
    } else if (light > 0) {
        base_bg = C_BLACK;
        low_bg = (minute < 720) ? C_BLUE : C_RED;
    } else {
        base_bg = C_BLACK;
        low_bg = C_BLACK;
    }
    if (solar_ecl) {
        base_bg = C_BLUE;
        low_bg = C_DARKGRAY;
    }
    if (g->weather.flash > 0) {
        base_bg = C_WHITE;
        low_bg = C_LIGHTGRAY;
    }
    for (y = 0; y < sky_h; ++y) {
        int bg = y >= sky_h - 3 ? low_bg : base_bg;
        if (y == sky_h - 4 && light > 0 && light < 60) {
            bg = C_BROWN;
        }
        for (x = 0; x < w; ++x) {
            put(fb, x0, y0, w, h, x, y, ' ', C_WHITE, bg);
        }
    }

    /* ---- Milky Way and stars (night) ---- */
    if (light < 30 && cloud < 90) {
        long rot = minute * (long)w / 1440L;
        info->stars_visible = 1;
        for (y = 0; y < sky_h - 2; ++y) {
            for (x = 0; x < w; ++x) {
                long band = mx_floor_mod((long)x + rot + (long)y * 3L - (long)(seed % 40UL), (long)w + 30L);
                if (band < 6) {
                    put(fb, x0, y0, w, h, x, y, band < 2 ? G_SHADE2 : G_SHADE1, C_DARKGRAY, bg_at(fb, x0 + x, y0 + y));
                }
            }
        }
        for (i = 0; i < NSTARS; ++i) {
            cl_u32 hs = rng_hash3(seed, i, 4242, 0);
            int sx = (int)mx_floor_mod((long)(hs % 1000UL) * (long)w / 1000L + rot, (long)w);
            int sy = (int)((hs >> 10) % (cl_u32)(sky_h - 2));
            int cls = (int)((hs >> 20) & 7UL);
            int twinkle = (rng_hash3(seed, i, anim / 6L, 9) & 7UL) == 0;
            int glyph = cls == 0 ? '*' : cls < 3 ? '+' : G_MIDDOT;
            int col = cls == 0 ? C_WHITE : cls == 1 ? C_YELLOW : cls == 2 ? C_LIGHTCYAN : C_LIGHTGRAY;
            if (g->p.dim == DIM_ANCIENT) {
                /* ancient stars burn in magenta and cyan */
                col = cls < 2 ? C_LIGHTMAGENTA : cls < 5 ? C_LIGHTCYAN : C_MAGENTA;
            }
            if (twinkle) {
                glyph = cls == 0 ? '+' : '.';
                col = C_DARKGRAY;
            }
            put(fb, x0, y0, w, h, sx, sy, glyph, col, bg_at(fb, x0 + CL_CLAMP(sx, 0, w - 1), y0 + sy));
        }
        /* constellations: small seeded star groups */
        for (i = 0; i < SKY_CONSTELLATIONS; ++i) {
            cl_u32 hc = rng_hash3(seed, i, 99, 1);
            int cx = (int)mx_floor_mod((long)(hc % 1000UL) * (long)w / 1000L + rot, (long)w);
            int cy = 1 + (int)((hc >> 10) % (cl_u32)CL_MAX(sky_h - 6, 1));
            int k, nst = 4 + (int)((hc >> 20) % 3UL);
            for (k = 0; k < nst; ++k) {
                cl_u32 hk = rng_hash3(seed, i, k, 2);
                int sx = cx + (int)(hk % 9UL) - 4;
                int sy = cy + (int)((hk >> 8) % 4UL) - 1;
                if (sx >= 0 && sx < w && sy >= 0 && sy < sky_h - 2) {
                    put(fb, x0, y0, w, h, sx, sy, '*', C_WHITE, bg_at(fb, x0 + sx, y0 + sy));
                }
            }
            if (show_names) {
                char nm[24];
                sky_constellation_name(seed, i, nm, sizeof(nm));
                if (cx + 1 < w && cy + 2 < sky_h - 2) {
                    fb_text(fb, x0 + cx - 2 < x0 ? x0 : x0 + cx - 2, y0 + cy + 2, nm,
                            ATTR(C_LIGHTCYAN, bg_at(fb, x0 + cx, y0 + cy + 2)), w - cx);
                }
            }
        }
    }

    /* ---- aurora (cold climates at night) ---- */
    if (night && climate_t < 30 && cloud < 60 && g->p.dim == DIM_OVERWORLD) {
        info->aurora = 1;
        for (x = 0; x < w; ++x) {
            int wy = 2 + (mx_sin256((int)(x * 9 + anim * 2)) * 2) / 1024;
            int col = ((x + anim / 3) % 7) < 3 ? C_LIGHTGREEN : C_LIGHTCYAN;
            put(fb, x0, y0, w, h, x, wy, G_SHADE2, col, C_BLACK);
            put(fb, x0, y0, w, h, x, wy + 1, G_SHADE1, C_GREEN, C_BLACK);
        }
    }

    /* ---- the Ancient Dimension: aether veil, ringed world, second moon ---- */
    if (g->p.dim == DIM_ANCIENT) {
        /* the aether veil ripples across every night sky */
        if (night && cloud < 80) {
            info->aether_veil = 1;
            for (x = 0; x < w; ++x) {
                int wy = 1 + (mx_sin256((int)(x * 7 + anim)) * 2 + 2048) / 1024;
                int col = ((x + anim / 4) % 9) < 4 ? C_LIGHTMAGENTA : C_MAGENTA;
                put(fb, x0, y0, w, h, x, wy, G_SHADE1, col, bg_at(fb, x0 + x, y0 + wy));
            }
        }
        /* the ringed world hangs low in the south, drifting once per day */
        if (cloud < 90) {
            int rx = (int)mx_floor_mod((long)(seed % (cl_u32)w) + minute * (long)w / 1440L, (long)w);
            int ry = sky_h - 5;
            int rb = bg_at(fb, x0 + CL_CLAMP(rx, 0, w - 1), y0 + CL_CLAMP(ry, 0, sky_h - 1));
            info->ringed_world = 1;
            put(fb, x0, y0, w, h, rx, ry, 233, C_LIGHTCYAN, rb);
            put(fb, x0, y0, w, h, rx - 2, ry, G_DHLINE, C_CYAN, rb);
            put(fb, x0, y0, w, h, rx - 1, ry, G_DHLINE, C_LIGHTCYAN, rb);
            put(fb, x0, y0, w, h, rx + 1, ry, G_DHLINE, C_LIGHTCYAN, rb);
            put(fb, x0, y0, w, h, rx + 2, ry, G_DHLINE, C_CYAN, rb);
        }
        /* Veyra, the second moon, runs through eight phases in three days */
        {
            static const char vg[8] = { 'o', ')', 'D', 'O', 'O', 'O', 'C', '(' };
            int ph = sky_veyra_phase(g);
            long pos = mx_floor_mod(minute * 2L + (long)ph * 90L, 1440L);
            if (pos < 720 && cloud < 85) {
                int vx = (int)(pos * (long)(w - 1) / 720L);
                int vy = (sky_h - 3) - (mx_sin256((int)(pos * 128L / 720L)) * (sky_h - 5)) / 1024;
                info->second_moon = 1;
                put(fb, x0, y0, w, h, vx, vy, vg[ph & 7], ph == 0 ? C_DARKGRAY : C_LIGHTMAGENTA,
                    bg_at(fb, x0 + CL_CLAMP(vx, 0, w - 1), y0 + CL_CLAMP(vy, 0, sky_h - 1)));
            }
        }
    }

    /* ---- sun ---- */
    if (minute >= d.sunrise_min - 20 && minute <= d.sunset_min + 20) {
        long span = d.sunset_min - d.sunrise_min + 40;
        long pos = minute - d.sunrise_min + 20;
        int sx = (int)(pos * (long)(w - 1) / span);
        int sy = (sky_h - 2) - (mx_sin256((int)(pos * 128L / span)) * (sky_h - 3)) / 1024;
        int sb = bg_at(fb, x0 + CL_CLAMP(sx, 0, w - 1), y0 + CL_CLAMP(sy, 0, sky_h - 1));
        info->sun_visible = cloud < 85;
        if (info->sun_visible) {
            if (solar_ecl) {
                put(fb, x0, y0, w, h, sx, sy, 'O', C_DARKGRAY, C_BLACK);
                put(fb, x0, y0, w, h, sx - 1, sy, '(', C_YELLOW, sb);
                put(fb, x0, y0, w, h, sx + 1, sy, ')', C_YELLOW, sb);
            } else {
                put(fb, x0, y0, w, h, sx, sy, G_SUN, C_YELLOW, light < 50 ? C_RED : sb);
                put(fb, x0, y0, w, h, sx - 1, sy, '-', C_YELLOW, sb);
                put(fb, x0, y0, w, h, sx + 1, sy, '-', C_YELLOW, sb);
            }
        }
    }

    /* ---- moon (rises later each day with the phase) ---- */
    {
        static const char phase_glyph[8] = { 'o', ')', 'D', 'O', 'O', 'O', 'C', '(' };
        long rise = mx_floor_mod((long)d.sunset_min - 720L + (long)d.moon_phase * 180L, 1440L);
        long pos = mx_floor_mod(minute - rise, 1440L);
        if (pos < 720 && (light < 60 || d.moon_phase >= 3) && cloud < 85) {
            int mx = (int)(pos * (long)(w - 1) / 720L);
            int my = (sky_h - 2) - (mx_sin256((int)(pos * 128L / 720L)) * (sky_h - 3)) / 1024;
            int col = d.moon_phase == 0 ? C_DARKGRAY : (d.moon_phase == 4 ? C_WHITE : C_LIGHTGRAY);
            if (lunar_ecl) {
                col = C_RED;
            }
            info->moon_visible = 1;
            put(fb, x0, y0, w, h, mx, my, phase_glyph[d.moon_phase & 7], col,
                bg_at(fb, x0 + CL_CLAMP(mx, 0, w - 1), y0 + CL_CLAMP(my, 0, sky_h - 1)));
        }
    }

    /* ---- comet (deterministic comet nights) ---- */
    if (night && (g->time.day % 37L) == (long)(seed % 37UL) && cloud < 70) {
        int cx = w / 3 + (int)(seed % 10UL), cy = 3;
        info->comet = 1;
        put(fb, x0, y0, w, h, cx, cy, '*', C_WHITE, C_BLACK);
        for (i = 1; i <= 6; ++i) {
            put(fb, x0, y0, w, h, cx + i, cy - (i > 3 ? 1 : 0), i < 3 ? G_DHLINE : '-',
                i < 4 ? C_LIGHTCYAN : C_CYAN, C_BLACK);
        }
    }

    /* ---- meteors ---- */
    if (night && cloud < 70) {
        long slot = anim / 30L;
        cl_u32 hm = rng_hash3(seed, slot, 31337, 0);
        if (hm % 4UL == 0UL) {
            int age = (int)(anim % 30L);
            int mx = (int)(hm >> 8) % CL_MAX(w - 10, 1) + age;
            int my = (int)((hm >> 20) % 4UL) + age / 3;
            if (age < 14 && my < sky_h - 3) {
                info->meteor = 1;
                put(fb, x0, y0, w, h, mx, my, '*', C_WHITE, C_BLACK);
                put(fb, x0, y0, w, h, mx - 1, my, '-', C_LIGHTGRAY, C_BLACK);
                put(fb, x0, y0, w, h, mx - 2, my, '-', C_DARKGRAY, C_BLACK);
            }
        }
    }

    /* ---- clouds drift with the wind ---- */
    {
        int dirx = mx_cos256(g->weather.wind_dir) >= 0 ? 1 : -1;
        int thr = 4095 - cloud * 26;
        wind_off = anim * (long)(5 + wd->wind) / 120L * dirx;
        for (y = 0; y < sky_h - 1; ++y) {
            for (x = 0; x < w; ++x) {
                long n = noise2(CL_U32(seed + 555UL), ((long)x - wind_off) * 2L, (long)y * 5L, 3);
                if (n > thr) {
                    int dense = n > thr + 400;
                    int col = night ? C_DARKGRAY : (cloud > 70 ? C_LIGHTGRAY : C_WHITE);
                    put(fb, x0, y0, w, h, x, y, dense ? G_SHADE3 : G_SHADE1, col, bg_at(fb, x0 + x, y0 + y));
                }
            }
        }
    }

    /* ---- birds (day) and crows (dusk/day); none in the Ancient Dimension ---- */
    if (light >= 40 && cloud < 90 && !wd->precip && g->p.dim == DIM_OVERWORLD) {
        info->birds = 1;
        for (i = 0; i < 3; ++i) {
            int bx = (int)mx_floor_mod(anim / 3L + (long)i * 23L + (long)(seed % 17UL), (long)w + 12L) - 6;
            int by = 3 + i * 2 + ((anim / 8L + i) % 2 == 0 ? 0 : 1);
            int flap = ((anim / 2L) + i) % 2 == 0;
            put(fb, x0, y0, w, h, bx, by, flap ? 'v' : '^', C_BLACK, bg_at(fb, x0 + CL_CLAMP(bx, 0, w - 1), y0 + by));
        }
    }
    if (light >= 15 && light < 70 && !wd->precip && g->p.dim == DIM_OVERWORLD) {
        info->crows = 1;
        for (i = 0; i < 2; ++i) {
            int bx = w - (int)mx_floor_mod(anim / 2L + (long)i * 31L, (long)w + 10L) + 5;
            int by = 5 + i * 3;
            put(fb, x0, y0, w, h, bx, by, '~', C_BLACK, bg_at(fb, x0 + CL_CLAMP(bx, 0, w - 1), y0 + by));
        }
    }

    /* ---- precipitation ---- */
    if (wd->precip) {
        int dens = wd->intensity;
        for (y = 0; y < sky_h; ++y) {
            for (x = 0; x < w; ++x) {
                cl_u32 hp = rng_hash3(seed, x * 3 + (int)(anim % 997L), y - (int)(anim % 977L), 5);
                if ((long)(hp % 1000UL) < (long)dens * 3L) {
                    int gch = wd->precip == PRECIP_SNOW ? '*' : wd->precip == PRECIP_HAIL ? 'o' :
                              wd->precip == PRECIP_SAND ? G_SHADE1 : wd->precip == PRECIP_ASH ? G_MIDDOT :
                              wd->precip == PRECIP_STAR ? '+' : '/';
                    int col = wd->precip == PRECIP_SNOW ? C_WHITE : wd->precip == PRECIP_SAND ? C_BROWN :
                              wd->precip == PRECIP_ASH ? C_DARKGRAY : wd->precip == PRECIP_STAR ? C_LIGHTMAGENTA :
                              C_LIGHTCYAN;
                    put(fb, x0, y0, w, h, x, y, gch, col, bg_at(fb, x0 + x, y0 + y));
                }
            }
        }
    }

    /* ---- lightning bolt ---- */
    if (g->weather.flash > 0) {
        int bx = (int)(rng_hash3(seed, anim / 3L, 1, 1) % (cl_u32)w);
        for (y = 0; y < sky_h; ++y) {
            put(fb, x0, y0, w, h, bx + (y % 3 == 1 ? 1 : 0), y, (y % 2) ? '/' : '\\', C_YELLOW, C_WHITE);
        }
    }

    /* ---- horizon silhouette ---- */
    for (x = 0; x < w; ++x) {
        long hh = noise2(CL_U32(seed + 9UL), (long)x + g->p.x, 0, 3);
        int col = night ? C_DARKGRAY : C_GREEN;
        put(fb, x0, y0, w, h, x, sky_h, hh > 2300 ? G_BLOCK : G_HALF_LO, col, hh > 2300 ? C_GREEN : low_bg);
        put(fb, x0, y0, w, h, x, sky_h + 1, G_BLOCK, night ? C_BLACK : C_GREEN, C_BLACK);
    }
}
