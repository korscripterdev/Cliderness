/*
 * Cliderness - world/time/clidetime.h  (ClideTime)
 * Game time is completely independent from real time. It only advances
 * through ctime_advance(), which the main loop calls from simulation ticks;
 * paused screens (help, pause menu) simply do not run ticks.
 *
 * Calendar: 12 months x 30 days = 360 days/year, 4 seasons x 90 days.
 * Lunar cycle 30 days (8 phases). Day 0 = first day of spring, year 1.
 */
#ifndef CLIDE_WORLD_TIME_H
#define CLIDE_WORLD_TIME_H

#include "core/types.h"

#define CT_MS_PER_DAY 86400000L

enum { SEASON_SPRING = 0, SEASON_SUMMER, SEASON_AUTUMN, SEASON_WINTER };

typedef struct ClideTimeCfg {
    long real_seconds_per_day;   /* default 1200 (20 real minutes) */
    long days_per_year;          /* default 360 */
    long days_per_season;        /* default 90 */
    long scale_pct;              /* time_scale * 100, default 100 */
} ClideTimeCfg;

typedef struct ClideTime {
    long day;        /* days since world creation */
    long ms;         /* milliseconds into the current day */
    long rem1, rem2; /* fixed-point remainders (exact long-term rate) */
    ClideTimeCfg cfg;
} ClideTime;

typedef struct ClideDate {
    int  hour, minute, second;
    long year;
    int  month, day_of_month;  /* 1-based */
    int  day_of_year;          /* 0-based */
    int  season;
    int  moon_phase;           /* 0 new .. 4 full .. 7 */
    int  sunrise_min, sunset_min;
    int  is_day;
} ClideDate;

void ctime_default_cfg(ClideTimeCfg *c);
void ctime_init(ClideTime *t, const ClideTimeCfg *cfg, int start_hour, int start_minute);
/* Advance by real_ms of *simulated* real time. */
void ctime_advance(ClideTime *t, long real_ms);
void ctime_add_seconds(ClideTime *t, long game_seconds);
void ctime_set_time_of_day(ClideTime *t, long seconds);
void ctime_set_season(ClideTime *t, int season);
void ctime_date(const ClideTime *t, ClideDate *d);
long ctime_minute_of_day(const ClideTime *t);
long ctime_total_minutes(const ClideTime *t);
/* 0..100: daylight strength from the sun position */
int  ctime_daylight(const ClideTime *t);
/* game milliseconds produced by one tick of tick_ms real ms (informational) */
long ctime_ms_per_tick(const ClideTime *t, long tick_ms);
const char *ctime_season_key(int season);
const char *ctime_moon_key(int phase);

#endif
