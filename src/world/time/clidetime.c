/* Cliderness - world/time/clidetime.c */
#include "world/time/clidetime.h"
#include "core/mathx.h"

void ctime_default_cfg(ClideTimeCfg *c)
{
    c->real_seconds_per_day = 1200L;
    c->days_per_year = 360L;
    c->days_per_season = 90L;
    c->scale_pct = 100L;
}

void ctime_init(ClideTime *t, const ClideTimeCfg *cfg, int start_hour, int start_minute)
{
    t->cfg = *cfg;
    if (t->cfg.real_seconds_per_day < 60L) t->cfg.real_seconds_per_day = 60L;
    if (t->cfg.days_per_year < 4L) t->cfg.days_per_year = 4L;
    if (t->cfg.days_per_season < 1L) t->cfg.days_per_season = t->cfg.days_per_year / 4L;
    if (t->cfg.scale_pct < 0L) t->cfg.scale_pct = 0L;
    t->day = 0;
    t->ms = ((long)start_hour * 3600L + (long)start_minute * 60L) * 1000L;
    t->rem1 = 0;
    t->rem2 = 0;
}

static void normalize(ClideTime *t)
{
    while (t->ms >= CT_MS_PER_DAY) {
        t->ms -= CT_MS_PER_DAY;
        ++t->day;
    }
    while (t->ms < 0) {
        t->ms += CT_MS_PER_DAY;
        --t->day;
    }
    if (t->day < 0) {
        t->day = 0;
        t->ms = 0;
    }
}

/*
 * game_ms = real_ms * 86400 / real_seconds_per_day * scale_pct / 100
 * Done in two stages with carried remainders so that 32-bit longs never
 * overflow (real_ms <= 1000 per call) and no time is lost to rounding.
 */
void ctime_advance(ClideTime *t, long real_ms)
{
    long num, base, num2, add;
    /* limits keep every intermediate below 2^31 */
    t->cfg.real_seconds_per_day = CL_CLAMP(t->cfg.real_seconds_per_day, 60L, 864000L);
    t->cfg.scale_pct = CL_CLAMP(t->cfg.scale_pct, 0L, 10000L);
    while (real_ms > 0) {
        long step = real_ms > 50L ? 50L : real_ms;
        real_ms -= step;
        num = step * 86400L + t->rem1;
        base = num / t->cfg.real_seconds_per_day;
        t->rem1 = num % t->cfg.real_seconds_per_day;
        num2 = base * t->cfg.scale_pct + t->rem2;
        add = num2 / 100L;
        t->rem2 = num2 % 100L;
        t->ms += add;
        normalize(t);
    }
}

long ctime_ms_per_tick(const ClideTime *t, long tick_ms)
{
    return tick_ms * 86400L / t->cfg.real_seconds_per_day * t->cfg.scale_pct / 100L;
}

void ctime_add_seconds(ClideTime *t, long s)
{
    long days = s / 86400L;
    long rest = s % 86400L;
    t->day += days;
    t->ms += rest * 1000L;
    normalize(t);
}

void ctime_set_time_of_day(ClideTime *t, long seconds)
{
    seconds = mx_floor_mod(seconds, 86400L);
    if (seconds * 1000L < t->ms) {
        ++t->day; /* time never runs backwards */
    }
    t->ms = seconds * 1000L;
}

void ctime_set_season(ClideTime *t, int season)
{
    long dpy = t->cfg.days_per_year;
    long doy = t->day % dpy;
    long target = (long)(season & 3) * t->cfg.days_per_season;
    long diff = target - doy;
    if (diff < 0) {
        diff += dpy;
    }
    t->day += diff;
}

long ctime_minute_of_day(const ClideTime *t)
{
    return t->ms / 60000L;
}

long ctime_total_minutes(const ClideTime *t)
{
    return t->day * 1440L + t->ms / 60000L;
}

void ctime_date(const ClideTime *t, ClideDate *d)
{
    long secs = t->ms / 1000L;
    long dpy = t->cfg.days_per_year;
    long doy = t->day % dpy;
    long mlen = dpy / 12L;
    int ang, off;
    if (mlen < 1) {
        mlen = 1;
    }
    d->hour = (int)(secs / 3600L);
    d->minute = (int)((secs / 60L) % 60L);
    d->second = (int)(secs % 60L);
    d->year = t->day / dpy + 1L;
    d->day_of_year = (int)doy;
    d->month = (int)(doy / mlen) + 1;
    if (d->month > 12) {
        d->month = 12;
    }
    d->day_of_month = (int)(doy - (long)(d->month - 1) * mlen) + 1;
    d->season = (int)((doy / t->cfg.days_per_season) & 3);
    d->moon_phase = (int)(((t->day % 30L) * 8L + 15L) / 30L) & 7;
    /* day length: +-90 minutes around 12h, longest mid-summer */
    ang = (int)((doy * 256L) / dpy);
    off = (mx_sin256(ang) * 90) / 1024;
    d->sunrise_min = 360 - off;
    d->sunset_min = 1080 + off;
    d->is_day = (secs / 60L) >= d->sunrise_min && (secs / 60L) < d->sunset_min;
}

int ctime_daylight(const ClideTime *t)
{
    ClideDate d;
    long m = ctime_minute_of_day(t);
    long len, pos;
    int s;
    ctime_date(t, &d);
    /* twilight 40 minutes on each side */
    if (m < d.sunrise_min - 40 || m > d.sunset_min + 40) {
        return 0;
    }
    if (m < d.sunrise_min + 40) {
        return (int)((m - (d.sunrise_min - 40)) * 60L / 80L);
    }
    if (m > d.sunset_min - 40) {
        return (int)(((d.sunset_min + 40) - m) * 60L / 80L);
    }
    len = d.sunset_min - d.sunrise_min;
    pos = m - d.sunrise_min;
    s = mx_sin256((int)(pos * 128L / len));
    return 60 + (s * 40) / 1024;
}

const char *ctime_season_key(int season)
{
    static const char *const k[4] = {
        "season.spring", "season.summer", "season.autumn", "season.winter"
    };
    return k[season & 3];
}

const char *ctime_moon_key(int phase)
{
    static const char *const k[8] = {
        "moon.new", "moon.waxing_crescent", "moon.first_quarter", "moon.waxing_gibbous",
        "moon.full", "moon.waning_gibbous", "moon.last_quarter", "moon.waning_crescent"
    };
    return k[phase & 7];
}
