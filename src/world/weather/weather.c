/* Cliderness - world/weather/weather.c */
#include <stdio.h>
#include <string.h>
#include "world/weather/weather.h"
#include "localization/locale.h"
#include "core/ini.h"
#include "core/str.h"

WeatherDef g_weather[MAX_WEATHER];
int g_weather_count = 0;

typedef struct WCtx {
    int cur;
    char *err;
    size_t errn;
} WCtx;

static int werr(WCtx *c, const char *what, int line)
{
    char num[12];
    str_from_long(num, (long)line);
    str_copy(c->err, "weather.def:", c->errn);
    str_cat(c->err, num, c->errn);
    str_cat(c->err, ": bad ", c->errn);
    str_cat(c->err, what, c->errn);
    return 1;
}

static int wrange(const char *v, cl_s16 *lo, cl_s16 *hi)
{
    char buf[24];
    char *sep;
    long a, b;
    str_copy(buf, v, sizeof(buf));
    sep = strstr(buf, "..");
    if (!sep) {
        return 0;
    }
    *sep = '\0';
    if (!str_to_long(buf, &a) || !str_to_long(sep + 2, &b)) {
        return 0;
    }
    *lo = (cl_s16)a;
    *hi = (cl_s16)b;
    return 1;
}

static int weather_cb(void *user, const char *type, const char *id, const char *key,
                      const char *value, int line)
{
    WCtx *c = (WCtx *)user;
    WeatherDef *w;
    long n;
    if (!str_ieq(type, "weather")) {
        return werr(c, "section", line);
    }
    if (!key) {
        if (g_weather_count >= MAX_WEATHER) {
            return werr(c, "count", line);
        }
        c->cur = g_weather_count++;
        w = &g_weather[c->cur];
        memset(w, 0, sizeof(*w));
        str_copy(w->id, id, sizeof(w->id));
        str_copy(w->name, id, sizeof(w->name));
        w->visibility = 100;
        w->t_lo = 0; w->t_hi = 100; w->m_lo = 0; w->m_hi = 100;
        w->weight = 10;
        w->min_min = 60;
        w->max_min = 240;
        return 0;
    }
    w = &g_weather[c->cur];
    if (str_ieq(key, "name")) {
        str_copy(w->name, value, sizeof(w->name));
    } else if (str_ieq(key, "precip")) {
        static const char *const p[] = { "none", "rain", "snow", "hail", "sand", "ash", "star" };
        int i;
        for (i = 0; i < (int)CL_COUNTOF(p); ++i) {
            if (str_ieq(value, p[i])) {
                break;
            }
        }
        if (i == (int)CL_COUNTOF(p)) return werr(c, "precip", line);
        w->precip = (cl_u8)i;
    } else if (str_ieq(key, "temp")) {
        if (!wrange(value, &w->t_lo, &w->t_hi)) return werr(c, "temp", line);
    } else if (str_ieq(key, "humid")) {
        if (!wrange(value, &w->m_lo, &w->m_hi)) return werr(c, "humid", line);
    } else if (str_ieq(key, "duration")) {
        if (!wrange(value, &w->min_min, &w->max_min)) return werr(c, "duration", line);
    } else {
        if (!str_to_long(value, &n) || n < -100 || n > 1000) return werr(c, key, line);
        if (str_ieq(key, "intensity")) w->intensity = (cl_u8)n;
        else if (str_ieq(key, "visibility")) w->visibility = (cl_s16)n;
        else if (str_ieq(key, "temp_delta")) w->temp_delta = (cl_s16)n;
        else if (str_ieq(key, "move_penalty")) w->move_penalty = (cl_s16)n;
        else if (str_ieq(key, "stamina")) w->stamina = (cl_s16)n;
        else if (str_ieq(key, "wind")) w->wind = (cl_s16)n;
        else if (str_ieq(key, "cloud")) w->cloud = (cl_s16)n;
        else if (str_ieq(key, "thunder")) w->thunder = (cl_u8)n;
        else if (str_ieq(key, "ancient")) w->ancient = (cl_u8)n;
        else if (str_ieq(key, "weight")) w->weight = (cl_s16)n;
        else return werr(c, key, line);
    }
    return 0;
}

int weather_load(const char *root, char *err, size_t errn)
{
    char path[160];
    WCtx c;
    int r;
    g_weather_count = 0;
    c.cur = 0;
    c.err = err;
    c.errn = errn;
    str_copy(path, root, sizeof(path));
    str_cat(path, "/weather/weather.def", sizeof(path));
    r = ini_parse_file(path, weather_cb, &c);
    if (r < 0) {
        str_copy(err, "cannot open ", errn);
        str_cat(err, path, errn);
        return 0;
    }
    if (r > 0) {
        return 0;
    }
    if (g_weather_count == 0) {
        str_copy(err, "no weather defined", errn);
        return 0;
    }
    return 1;
}

int weather_find(const char *id)
{
    int i;
    for (i = 0; i < g_weather_count; ++i) {
        if (str_ieq(g_weather[i].id, id)) {
            return i;
        }
    }
    return -1;
}

const char *weather_name(int w)
{
    char key[32];
    if (w < 0 || w >= g_weather_count) {
        return "?";
    }
    str_copy(key, "weather.", sizeof(key));
    str_cat(key, g_weather[w].id, sizeof(key));
    return loc_get_or(key, g_weather[w].name);
}

const WeatherDef *weather_cur(const WeatherState *s)
{
    return &g_weather[(s->cur >= 0 && s->cur < g_weather_count) ? s->cur : 0];
}

void weather_init(WeatherState *s, cl_u32 seed)
{
    rng_seed(&s->rng, CL_U32(seed ^ 0x57EA7E5UL));
    s->cur = 0;
    s->minutes_left = 120 + rng_range(&s->rng, 120);
    s->wind_dir = (int)rng_range(&s->rng, 256);
    s->flash = 0;
}

void weather_force(WeatherState *s, int w, long minutes)
{
    if (w < 0 || w >= g_weather_count) {
        return;
    }
    s->cur = w;
    s->minutes_left = minutes > 0 ? minutes : 60 + rng_range(&s->rng, 180);
}

static int allowed(const WeatherDef *w, int temp, int humid, int dim)
{
    if (dim == 1) {
        if (w->ancient == 0) {
            return 0;
        }
    } else if (w->ancient == 1) {
        return 0;
    }
    return temp >= w->t_lo && temp <= w->t_hi && humid >= w->m_lo && humid <= w->m_hi;
}

void weather_minute(WeatherState *s, int temp, int humid, int dim)
{
    long total = 0, roll;
    int i;
    /* wind slowly veers */
    s->wind_dir = (s->wind_dir + (int)rng_range(&s->rng, 5) - 2) & 255;
    if (--s->minutes_left > 0) {
        /* leave climates where the current weather is impossible */
        if (allowed(&g_weather[s->cur], temp, humid, dim) || s->minutes_left < 5) {
            return;
        }
        s->minutes_left = 5;
        return;
    }
    for (i = 0; i < g_weather_count; ++i) {
        if (allowed(&g_weather[i], temp, humid, dim)) {
            total += g_weather[i].weight;
        }
    }
    if (total <= 0) {
        s->cur = 0;
        s->minutes_left = 120;
        return;
    }
    roll = rng_range(&s->rng, total);
    for (i = 0; i < g_weather_count; ++i) {
        if (!allowed(&g_weather[i], temp, humid, dim)) {
            continue;
        }
        roll -= g_weather[i].weight;
        if (roll < 0) {
            const WeatherDef *w = &g_weather[i];
            long span = (long)(w->max_min - w->min_min);
            s->cur = i;
            s->minutes_left = (long)w->min_min + (span > 0 ? rng_range(&s->rng, span + 1) : 0);
            return;
        }
    }
}

void weather_tick(WeatherState *s)
{
    if (s->flash > 0) {
        --s->flash;
    }
    if (g_weather[s->cur].thunder && rng_range(&s->rng, 400) == 0) {
        s->flash = 3;
    }
}
