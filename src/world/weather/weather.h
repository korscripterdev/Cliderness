/*
 * Cliderness - world/weather/weather.h
 * Data-driven weather (data/weather/weather.def). Each game minute the active
 * weather counts down; when it ends a successor is picked by weighted random
 * among the definitions whose climate range matches the player's location.
 */
#ifndef CLIDE_WORLD_WEATHER_H
#define CLIDE_WORLD_WEATHER_H

#include "core/types.h"
#include "core/rng.h"

#define MAX_WEATHER 24

enum { PRECIP_NONE = 0, PRECIP_RAIN, PRECIP_SNOW, PRECIP_HAIL, PRECIP_SAND, PRECIP_ASH, PRECIP_STAR };

typedef struct WeatherDef {
    char   id[20];
    char   name[28];
    cl_u8  precip;
    cl_u8  intensity;     /* 0..100 particle density */
    cl_s16 visibility;    /* % of normal */
    cl_s16 temp_delta;    /* deg C */
    cl_s16 move_penalty;  /* % slower */
    cl_s16 stamina;       /* extra stamina drain per minute */
    cl_s16 wind;          /* 0..100 */
    cl_s16 cloud;         /* 0..100 */
    cl_u8  thunder;
    cl_u8  ancient;       /* 1 = only in the Ancient Dimension, 2 = both */
    cl_s16 t_lo, t_hi, m_lo, m_hi;
    cl_s16 weight;
    cl_s16 min_min, max_min; /* duration in game minutes */
} WeatherDef;

typedef struct WeatherState {
    int    cur;
    long   minutes_left;
    int    wind_dir;      /* 0..255 angle, 0 = east */
    int    flash;         /* lightning flash frames remaining */
    ClideRng rng;
} WeatherState;

extern WeatherDef g_weather[MAX_WEATHER];
extern int g_weather_count;

int  weather_load(const char *root, char *err, size_t errn);
int  weather_find(const char *id);
const char *weather_name(int w);
void weather_init(WeatherState *s, cl_u32 seed);
/* Called once per elapsed game minute. temp/humid = climate 0..100 */
void weather_minute(WeatherState *s, int temp, int humid, int dim);
/* Called every tick for short visual events (lightning). */
void weather_tick(WeatherState *s);
void weather_force(WeatherState *s, int w, long minutes);
const WeatherDef *weather_cur(const WeatherState *s);

#endif
