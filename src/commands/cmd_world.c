/* Cliderness - commands/cmd_world.c : world, time, weather, season */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "world/biomes/biome.h"
#include "localization/locale.h"
#include "save/save.h"
#include "core/str.h"
#include "core/mathx.h"

static int parse_time_word(const char *s, long *secs)
{
    long h, m;
    const char *colon;
    if (str_ieq(s, "day") || str_ieq(s, "morning")) { *secs = 7L * 3600L; return 1; }
    if (str_ieq(s, "noon")) { *secs = 12L * 3600L; return 1; }
    if (str_ieq(s, "sunset") || str_ieq(s, "evening")) { *secs = 18L * 3600L; return 1; }
    if (str_ieq(s, "night")) { *secs = 21L * 3600L; return 1; }
    if (str_ieq(s, "midnight")) { *secs = 0; return 1; }
    if (str_ieq(s, "sunrise") || str_ieq(s, "dawn")) { *secs = 6L * 3600L; return 1; }
    colon = strchr(s, ':');
    if (colon) {
        char hb[8];
        size_t len = (size_t)(colon - s);
        if (len == 0 || len > 2) {
            return 0;
        }
        memcpy(hb, s, len);
        hb[len] = '\0';
        if (!str_to_long(hb, &h) || !str_to_long(colon + 1, &m) || h < 0 || h > 23 || m < 0 || m > 59) {
            return 0;
        }
        *secs = h * 3600L + m * 60L;
        return 1;
    }
    return 0;
}

/* number with optional unit suffix s/m/h/d; plain numbers are game seconds */
static int parse_duration(const char *s, long *secs)
{
    char buf[24];
    size_t len;
    long mul = 1, n;
    str_copy(buf, s, sizeof(buf));
    len = strlen(buf);
    if (len == 0) {
        return 0;
    }
    switch (buf[len - 1]) {
    case 's': mul = 1; buf[len - 1] = '\0'; break;
    case 'm': mul = 60; buf[len - 1] = '\0'; break;
    case 'h': mul = 3600; buf[len - 1] = '\0'; break;
    case 'd': mul = 86400; buf[len - 1] = '\0'; break;
    default: break;
    }
    if (!str_to_long(buf, &n) || n < 0 || n > 100000000L / mul) {
        return 0;
    }
    *secs = n * mul;
    return 1;
}

static void print_time(ConCtx *c)
{
    ClideDate d;
    char buf[128];
    ctime_date(&c->g->time, &d);
    sprintf(buf, "%02d:%02d:%02d  %s %d, %s %ld  (day %ld)", d.hour, d.minute, d.second,
            loc_get(ctime_season_key(d.season)), d.day_of_month, loc_get("ui.year"), d.year,
            c->g->time.day + 1);
    con_out(buf);
    sprintf(buf, "%s  sunrise %02d:%02d sunset %02d:%02d", loc_get(ctime_moon_key(d.moon_phase)),
            d.sunrise_min / 60, d.sunrise_min % 60, d.sunset_min / 60, d.sunset_min % 60);
    con_out(buf);
}

static int c_time(ConCtx *c)
{
    long secs;
    if (c->argc < 2 || str_ieq(c->argv[1], "query") || str_ieq(c->argv[1], "get")) {
        print_time(c);
        return CMD_OK;
    }
    if (!c->g->cheats) {
        con_err(loc_get("con.no_cheats"));
        return CMD_DENIED;
    }
    c->g->cheated = 1;
    if (c->argc < 3) {
        return con_usage(c);
    }
    if (str_ieq(c->argv[1], "set")) {
        if (!parse_time_word(c->argv[2], &secs) && !parse_duration(c->argv[2], &secs)) {
            return con_usage(c);
        }
        ctime_set_time_of_day(&c->g->time, secs);
    } else if (str_ieq(c->argv[1], "add")) {
        if (!parse_duration(c->argv[2], &secs)) {
            return con_usage(c);
        }
        ctime_add_seconds(&c->g->time, secs);
    } else {
        return con_usage(c);
    }
    c->g->last_minute = ctime_total_minutes(&c->g->time);
    print_time(c);
    return CMD_OK;
}

static int c_timescale(ConCtx *c)
{
    char buf[64];
    long pct;
    if (c->argc < 2) {
        sprintf(buf, "time_scale = %ld.%02ld", c->g->time.cfg.scale_pct / 100, c->g->time.cfg.scale_pct % 100);
        con_out(buf);
        return CMD_OK;
    }
    {
        /* accepts 1, 1.5, 0.25 ... */
        const char *s = c->argv[1];
        long whole = 0, frac = 0, div = 1;
        char tmp[16];
        const char *dot = strchr(s, '.');
        if (dot) {
            size_t l = (size_t)(dot - s);
            if (l >= sizeof(tmp)) return con_usage(c);
            memcpy(tmp, s, l);
            tmp[l] = '\0';
            if (l && !str_to_long(tmp, &whole)) return con_usage(c);
            str_copy(tmp, dot + 1, 3);
            if (*tmp && !str_to_long(tmp, &frac)) return con_usage(c);
            div = strlen(tmp) == 1 ? 10 : 100;
        } else if (!str_to_long(s, &whole)) {
            return con_usage(c);
        }
        pct = whole * 100 + frac * 100 / div;
    }
    if (pct < 0 || pct > 10000) {
        return con_usage(c);
    }
    c->g->time.cfg.scale_pct = pct;
    sprintf(buf, "time_scale = %ld.%02ld", pct / 100, pct % 100);
    con_good(buf);
    return CMD_OK;
}

static int c_daylength(ConCtx *c)
{
    long n;
    char buf[64];
    if (c->argc < 2) {
        sprintf(buf, "real_seconds_per_game_day = %ld", c->g->time.cfg.real_seconds_per_day);
        con_out(buf);
        return CMD_OK;
    }
    if (!con_arg_long(c, 1, &n) || n < 60 || n > 864000L) {
        return con_usage(c);
    }
    c->g->time.cfg.real_seconds_per_day = n;
    sprintf(buf, "real_seconds_per_game_day = %ld", n);
    con_good(buf);
    return CMD_OK;
}

static int c_weather(ConCtx *c)
{
    int w;
    long mins = 0;
    char buf[128];
    if (c->argc < 2 || str_ieq(c->argv[1], "query")) {
        sprintf(buf, "%s (%ld min)", weather_name(c->g->weather.cur), c->g->weather.minutes_left);
        con_out(buf);
        return CMD_OK;
    }
    if (str_ieq(c->argv[1], "list")) {
        int i;
        buf[0] = '\0';
        for (i = 0; i < g_weather_count; ++i) {
            if (strlen(buf) + strlen(g_weather[i].id) + 2 > 76) {
                con_out(buf);
                buf[0] = '\0';
            }
            str_cat(buf, g_weather[i].id, sizeof(buf));
            str_cat(buf, " ", sizeof(buf));
        }
        con_out(buf);
        return CMD_OK;
    }
    if (!c->g->cheats) {
        con_err(loc_get("con.no_cheats"));
        return CMD_DENIED;
    }
    c->g->cheated = 1;
    if (str_ieq(c->argv[1], "clear")) {
        w = weather_find("clear");
        if (c->argc >= 3) {
            con_arg_long(c, 2, &mins);
        }
    } else if (str_ieq(c->argv[1], "set") && c->argc >= 3) {
        w = weather_find(c->argv[2]);
        if (w < 0 && str_ieq(c->argv[2], "thunder")) {
            w = weather_find("thunderstorm");
        }
        if (c->argc >= 4 && (!con_arg_long(c, 3, &mins) || mins < 1)) {
            return con_usage(c);
        }
    } else {
        return con_usage(c);
    }
    if (w < 0) {
        con_err(loc_fmt("con.unknown_weather", c->argc >= 3 ? c->argv[2] : "?", NULL, NULL));
        return CMD_ERR;
    }
    weather_force(&c->g->weather, w, mins);
    con_good(loc_fmt("con.weather_set", weather_name(w), NULL, NULL));
    return CMD_OK;
}

static int c_season(ConCtx *c)
{
    static const char *const names[4] = { "spring", "summer", "autumn", "winter" };
    int i;
    ClideDate d;
    if (c->argc < 2 || str_ieq(c->argv[1], "query")) {
        ctime_date(&c->g->time, &d);
        con_out(loc_get(ctime_season_key(d.season)));
        return CMD_OK;
    }
    if (!c->g->cheats) {
        con_err(loc_get("con.no_cheats"));
        return CMD_DENIED;
    }
    if (!str_ieq(c->argv[1], "set") || c->argc < 3) {
        return con_usage(c);
    }
    for (i = 0; i < 4; ++i) {
        if (str_ieq(c->argv[2], names[i]) || (str_ieq(c->argv[2], "fall") && i == 2)) {
            c->g->cheated = 1;
            ctime_set_season(&c->g->time, i);
            c->g->last_minute = ctime_total_minutes(&c->g->time);
            con_good(loc_get(ctime_season_key(i)));
            return CMD_OK;
        }
    }
    return con_usage(c);
}

static int c_seed(ConCtx *c)
{
    char buf[48];
    sprintf(buf, "%s: %lu", loc_get("ui.seed"), (unsigned long)c->g->seed);
    con_out(buf);
    return CMD_OK;
}

static int c_biome(ConCtx *c)
{
    Player *p = &c->g->p;
    int b = world_biome(&c->g->world, p->dim, p->x, p->z);
    char buf[128];
    sprintf(buf, "%s [%s] %d C, humidity %d%%", biome_name(b), g_biomes[b].id, g_biomes[b].base_temp,
            g_biomes[b].humidity);
    con_out(buf);
    return CMD_OK;
}

static int c_biomes(ConCtx *c)
{
    char line[128];
    int i;
    (void)c;
    line[0] = '\0';
    for (i = 0; i < g_biome_count; ++i) {
        if (strlen(line) + strlen(g_biomes[i].id) + 2 > 76) {
            con_out(line);
            line[0] = '\0';
        }
        str_cat(line, g_biomes[i].id, sizeof(line));
        str_cat(line, " ", sizeof(line));
    }
    con_out(line);
    return CMD_OK;
}

static int c_locate(ConCtx *c)
{
    int b;
    long r, x, z;
    Player *p = &c->g->p;
    char buf[96];
    if (c->argc < 2) {
        return con_usage(c);
    }
    b = biome_find(c->argv[1]);
    if (b < 0) {
        con_err(loc_fmt("con.unknown_biome", c->argv[1], NULL, NULL));
        return CMD_ERR;
    }
    for (r = 16; r <= 3072; r += 16) {
        for (x = -r; x <= r; x += 16) {
            long zs[2];
            int k;
            zs[0] = -r;
            zs[1] = r;
            for (k = 0; k < 2; ++k) {
                z = zs[k];
                if (world_biome(&c->g->world, p->dim, p->x + x, p->z + z) == b) {
                    sprintf(buf, "%s: x=%ld z=%ld (%ld)", biome_name(b), p->x + x, p->z + z,
                            mx_isqrt(x * x + z * z));
                    con_good(buf);
                    return CMD_OK;
                }
            }
            if (x != -r && x != r) {
                continue;
            }
            for (z = -r + 16; z < r; z += 16) {
                if (world_biome(&c->g->world, p->dim, p->x + x, p->z + z) == b) {
                    sprintf(buf, "%s: x=%ld z=%ld (%ld)", biome_name(b), p->x + x, p->z + z,
                            mx_isqrt(x * x + z * z));
                    con_good(buf);
                    return CMD_OK;
                }
            }
        }
    }
    con_err(loc_get("con.not_found"));
    return CMD_ERR;
}

static int c_save(ConCtx *c)
{
    char err[96];
    if (!c->g->dir[0]) {
        con_err("memory-only world");
        return CMD_ERR;
    }
    if (save_game(c->g, c->plat, err, sizeof(err))) {
        con_good(loc_get("msg.saved"));
        return CMD_OK;
    }
    con_err(loc_fmt("msg.save_failed", err, NULL, NULL));
    return CMD_ERR;
}

static int c_world(ConCtx *c)
{
    char buf[128];
    sprintf(buf, "%s  seed %lu  %s  cheats %s", c->g->name, (unsigned long)c->g->seed,
            loc_get(game_mode_key(c->g->mode)), loc_get(c->g->cheats ? "con.on" : "con.off"));
    con_out(buf);
    sprintf(buf, "chunks cached %d, modified %d, generated %lu, loaded %lu, items %d",
            world_cached_chunks(&c->g->world), world_modified_chunks(&c->g->world),
            (unsigned long)c->g->world.stats.generated, (unsigned long)c->g->world.stats.loaded,
            ent_count(c->g));
    con_out(buf);
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "time", "time", "<set|add|query> [noon|midnight|HH:MM|N[s|m|h|d]]", "xt", "set|add|query", PERM_ANY, c_time, NULL },
    { "timescale", "time", "[multiplier]", "n", NULL, PERM_CHEAT, c_timescale, NULL },
    { "daylength", "time", "[real seconds per game day]", "n", NULL, PERM_CHEAT, c_daylength, NULL },
    { "weather", "weather", "<set|clear|query|list> [type] [minutes]", "xw", "set|clear|query|list", PERM_ANY, c_weather, NULL },
    { "season", "time", "<set|query> [spring|summer|autumn|winter]", "xS", "set|query", PERM_ANY, c_season, NULL },
    { "seed", "world", "", "", NULL, PERM_ANY, c_seed, NULL },
    { "biome", "world", "", "", NULL, PERM_ANY, c_biome, NULL },
    { "biomes", "world", "", "", NULL, PERM_ANY, c_biomes, NULL },
    { "locate", "world", "<biome>", "B", NULL, PERM_CHEAT, c_locate, NULL },
    { "save", "system", "", "", NULL, PERM_ANY, c_save, NULL },
    { "world", "world", "", "", NULL, PERM_ANY, c_world, "worldinfo" }
};

CommandTable cmd_table_world(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
