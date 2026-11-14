/*
 * Cliderness - commands/cmd_info.c : read-only information about the player,
 * the world, content definitions and the session.
 */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "content/effects.h"
#include "content/recipes.h"
#include "world/biomes/biome.h"
#include "world/weather/weather.h"
#include "world/structures/structure.h"
#include "gameplay/entities/creature.h"
#include "gameplay/vehicles/ship.h"
#include "localization/locale.h"
#include "core/mathx.h"
#include "core/str.h"

static const char *const g_dirs8[8] = { "E", "SE", "S", "SW", "W", "NW", "N", "NE" };

static const char *dir8(long dx, long dz)
{
    /* angle 0 = east, 64 = south (screen coordinates) */
    long ax = mx_abs(dx), az = mx_abs(dz);
    int k;
    if (dx == 0 && dz == 0) return "-";
    if (az * 5 < ax * 2) k = dx > 0 ? 0 : 4;
    else if (ax * 5 < az * 2) k = dz > 0 ? 2 : 6;
    else k = dx > 0 ? (dz > 0 ? 1 : 7) : (dz > 0 ? 3 : 5);
    return g_dirs8[k];
}

static void kv(const char *key, const char *val)
{
    char buf[200];
    sprintf(buf, "%-14.40s %.120s", loc_get(key), val);
    con_out(buf);
}

/* ------------------------------------------------------------ player */

static int c_survival(ConCtx *c)
{
    Player *p = &c->g->p;
    char b[64];
    sprintf(b, "%d/%d", p->hp, p->hp_max); kv("hud.hp", b);
    sprintf(b, "%d%%", p->stamina / 10); kv("hud.stamina", b);
    sprintf(b, "%d%%", p->hunger / 10); kv("hud.hunger", b);
    sprintf(b, "%d%%", p->thirst / 10); kv("hud.thirst", b);
    sprintf(b, "%d%%", p->fatigue / 10); kv("hud.fatigue", b);
    sprintf(b, "%d.%d C / %d C", p->body_temp / 10, p->body_temp % 10, p->env_temp); kv("hud.temp", b);
    return CMD_OK;
}

static int c_effects(ConCtx *c)
{
    int k, n = 0;
    char b[96];
    for (k = 0; k < PLAYER_FX; ++k) {
        if (c->g->p.fx[k].id >= 0 && c->g->p.fx[k].minutes > 0) {
            sprintf(b, "%-24.40s %d min", effect_name(c->g->p.fx[k].id), c->g->p.fx[k].minutes);
            con_out(b);
            ++n;
        }
    }
    if (!n) con_out("-");
    return CMD_OK;
}

static int c_skills(ConCtx *c)
{
    int k;
    char b[96];
    for (k = 0; k < SK_COUNT; ++k) {
        long need = player_level_xp(player_skill_level(&c->g->p, k) + 1);
        sprintf(b, "%-20.40s %3d  %ld / %ld xp", loc_get(player_skill_key(k)), player_skill_level(&c->g->p, k),
                c->g->p.skill_xp[k], need);
        con_out(b);
    }
    return CMD_OK;
}

static int c_carry(ConCtx *c)
{
    char b[64];
    sprintf(b, "%ld.%ld / %ld kg", inv_weight(&c->g->inv) / 1000L, (inv_weight(&c->g->inv) % 1000L) / 100L,
            game_carry_capacity(c->g) / 1000L);
    kv("hud.carry", b);
    sprintf(b, "%ld l", inv_volume(&c->g->inv) / 100L);
    con_out(b);
    return CMD_OK;
}

static int c_noise(ConCtx *c)
{
    char b[32];
    sprintf(b, "%d%%", c->g->p.noise); kv("hud.noise", b);
    sprintf(b, "%d%%", c->g->p.visibility); kv("hud.visibility", b);
    kv("hud.move", loc_get(player_move_key(c->g->p.move_mode)));
    return CMD_OK;
}

static int c_whereami(ConCtx *c)
{
    Player *p = &c->g->p;
    char b[96];
    int bi = world_biome(&c->g->world, p->dim, p->x, p->z);
    sprintf(b, "%ld, %ld, %ld", p->x, p->y, p->z); kv("hud.pos", b);
    kv("hud.dimension", loc_get(dim_key(p->dim)));
    kv("hud.biome", biome_name(bi));
    sprintf(b, "%d", world_gen_height(&c->g->world, p->dim, p->x, p->z)); kv("hud.surface", b);
    sprintf(b, "chunk %ld, %ld (y %ld)", (long)mx_floor_div(p->x, 32L), (long)mx_floor_div(p->z, 32L), p->y);
    con_out(b);
    return CMD_OK;
}

static int c_compass(ConCtx *c)
{
    Player *p = &c->g->p;
    long dx = p->spawn_x - p->x, dz = p->spawn_z - p->z;
    char b[96];
    sprintf(b, "%s %s, %ld m (%s)", loc_get("con.to_spawn"), dir8(dx, dz), mx_isqrt(dx * dx + dz * dz),
            loc_get(dim_key(p->spawn_dim)));
    con_out(b);
    return CMD_OK;
}

static int c_distance(ConCtx *c)
{
    long x, z, dx, dz;
    char b[64];
    if (c->argc < 3 || !con_arg_coord(c, 1, c->g->p.x, &x) || !con_arg_coord(c, 2, c->g->p.z, &z)) {
        return con_usage(c);
    }
    dx = x - c->g->p.x;
    dz = z - c->g->p.z;
    sprintf(b, "%ld m %s", mx_isqrt(dx * dx + dz * dz), dir8(dx, dz));
    con_good(b);
    return CMD_OK;
}

static int c_look(ConCtx *c)
{
    Player *p = &c->g->p;
    long dx, dz, x, z;
    int m, s;
    cl_u8 b;
    char buf[128];
    dir_delta(p->facing, &dx, &dz);
    x = p->x + dx;
    z = p->z + dz;
    m = mob_at(c->g, p->dim, x, p->y, z);
    s = ship_at(c->g, p->dim, x, p->y, z);
    b = world_get(&c->g->world, p->dim, x, p->y, z);
    if (m >= 0) {
        sprintf(buf, "%s %d/%d (%s)", creature_name(c->g->mobs[m].def), c->g->mobs[m].hp,
                g_creatures[c->g->mobs[m].def].hp, loc_get(mob_state_key(c->g->mobs[m].state)));
    } else if (s >= 0) {
        sprintf(buf, "%s (%s)", shipdef_name(c->g->ships[s].def), loc_get(ship_stage_key(ship_stage(c->g, s))));
    } else if (b != BLOCK_AIR) {
        sprintf(buf, "%s", block_name(b));
    } else {
        sprintf(buf, "%s / %s", block_name(b), block_name(world_get(&c->g->world, p->dim, x, p->y - 1, z)));
    }
    con_out(buf);
    return CMD_OK;
}

static int c_playtime(ConCtx *c)
{
    char b[96];
    long secs = c->g->tick / TICKS_PER_SECOND;
    sprintf(b, "%ld ticks, %ld:%02ld:%02ld, %s %ld", c->g->tick, secs / 3600L, (secs / 60L) % 60L, secs % 60L,
            loc_get("con.day"), c->g->time.day + 1);
    con_out(b);
    return CMD_OK;
}

/* ------------------------------------------------------------ time and sky */

static int c_date(ConCtx *c)
{
    ClideDate d;
    char b[96];
    ctime_date(&c->g->time, &d);
    sprintf(b, "%s %ld, %02d/%02d %02d:%02d", loc_get("ui.year"), d.year, d.month, d.day_of_month, d.hour, d.minute);
    kv("hud.date", b);
    kv("hud.season", loc_get(ctime_season_key(d.season)));
    sprintf(b, "%d", d.day_of_year + 1);
    kv("con.day", b);
    return CMD_OK;
}

static int c_moon(ConCtx *c)
{
    ClideDate d;
    ctime_date(&c->g->time, &d);
    kv("hud.moon", loc_get(ctime_moon_key(d.moon_phase)));
    return CMD_OK;
}

static int c_sun(ConCtx *c)
{
    ClideDate d;
    char b[64];
    ctime_date(&c->g->time, &d);
    sprintf(b, "%02d:%02d / %02d:%02d  (%d%%)", d.sunrise_min / 60, d.sunrise_min % 60, d.sunset_min / 60,
            d.sunset_min % 60, ctime_daylight(&c->g->time));
    kv("hud.sun", b);
    return CMD_OK;
}

static int c_forecast(ConCtx *c)
{
    char b[96];
    const WeatherDef *w = weather_cur(&c->g->weather);
    sprintf(b, "%s, %ld min", weather_name(c->g->weather.cur), c->g->weather.minutes_left);
    kv("hud.forecast", b);
    sprintf(b, "%d%%  cloud %d%%  visibility %d%%  %+d C", w->wind, w->cloud, w->visibility, w->temp_delta);
    kv("hud.wind", b);
    return CMD_OK;
}

static int c_weathers(ConCtx *c)
{
    int i;
    char b[128];
    (void)c;
    for (i = 0; i < g_weather_count; ++i) {
        const WeatherDef *w = &g_weather[i];
        sprintf(b, "%-15.15s %-22.40s wind %2d vis %3d %s", w->id, weather_name(i), w->wind, w->visibility,
                w->ancient == 1 ? loc_get("dim.ancient") : w->ancient == 2 ? "*" : "");
        con_out(b);
    }
    return CMD_OK;
}

static int c_light(ConCtx *c)
{
    char b[64];
    sprintf(b, "%d%%, radius %d", ctime_daylight(&c->g->time), game_light_radius(c->g));
    con_out(b);
    return CMD_OK;
}

/* ------------------------------------------------------------ terrain */

static int coords_xz(ConCtx *c, int i, long *x, long *z)
{
    *x = c->g->p.x;
    *z = c->g->p.z;
    if (c->argc > i + 1) {
        return con_arg_coord(c, i, c->g->p.x, x) && con_arg_coord(c, i + 1, c->g->p.z, z);
    }
    return 1;
}

static int c_climate(ConCtx *c)
{
    long x, z;
    char b[96];
    int bi;
    if (!coords_xz(c, 1, &x, &z)) return con_usage(c);
    bi = world_biome(&c->g->world, c->g->p.dim, x, z);
    sprintf(b, "%s: %d C, %d%%, climate %d/100", biome_name(bi), g_biomes[bi].base_temp, g_biomes[bi].humidity,
            world_climate_temp(&c->g->world, c->g->p.dim, x, z));
    con_out(b);
    return CMD_OK;
}

static int c_height(ConCtx *c)
{
    long x, z;
    char b[64];
    if (!coords_xz(c, 1, &x, &z)) return con_usage(c);
    sprintf(b, "%ld, %ld: %d (%s %d)", x, z, world_gen_height(&c->g->world, c->g->p.dim, x, z), loc_get("hud.sea"),
            SEA_LEVEL);
    con_out(b);
    return CMD_OK;
}

static int c_depth(ConCtx *c)
{
    Player *p = &c->g->p;
    char b[64];
    long top = world_top(&c->g->world, p->dim, p->x, p->z, WORLD_H - 1);
    sprintf(b, "%ld (%ld / %ld)", top + 1 - p->y, p->y, top + 1);
    con_out(b);
    return CMD_OK;
}

static int c_biomeat(ConCtx *c)
{
    long x, z;
    if (c->argc < 3 || !coords_xz(c, 1, &x, &z)) return con_usage(c);
    con_out(biome_name(world_biome(&c->g->world, c->g->p.dim, x, z)));
    return CMD_OK;
}

static int c_blockat(ConCtx *c)
{
    long x, y, z;
    Player *p = &c->g->p;
    char b[96];
    if (c->argc < 4 || !con_arg_coord(c, 1, p->x, &x) || !con_arg_coord(c, 2, p->y, &y) ||
        !con_arg_coord(c, 3, p->z, &z)) {
        return con_usage(c);
    }
    sprintf(b, "%ld,%ld,%ld: %s (%s)", x, y, z, block_name(world_get(&c->g->world, p->dim, x, y, z)),
            g_blocks[world_get(&c->g->world, p->dim, x, y, z)].id);
    con_out(b);
    return CMD_OK;
}

static int c_survey(ConCtx *c)
{
    long r = 16, x, z, lo = 999, hi = -1, water = 0, n = 0, sum = 0;
    Player *p = &c->g->p;
    char b[128];
    if (c->argc >= 2 && (!con_arg_long(c, 1, &r) || r < 4 || r > 64)) return con_usage(c);
    for (z = p->z - r; z <= p->z + r; z += 4) {
        for (x = p->x - r; x <= p->x + r; x += 4) {
            long h = world_gen_height(&c->g->world, p->dim, x, z);
            lo = CL_MIN(lo, h);
            hi = CL_MAX(hi, h);
            sum += h;
            water += h < SEA_LEVEL;
            ++n;
        }
    }
    sprintf(b, "r %ld: min %ld, max %ld, mean %ld, under sea %ld%%", r, lo, hi, sum / n, water * 100 / n);
    con_out(b);
    return CMD_OK;
}

/* biome letters around the player: a small text map */
static int c_map(ConCtx *c)
{
    long r = 8, step = 8, x, z;
    Player *p = &c->g->p;
    char line[64];
    char legend[40][24];
    int nleg = 0, i;
    if (c->argc >= 2 && (!con_arg_long(c, 1, &step) || step < 1 || step > 64)) return con_usage(c);
    for (z = -r; z <= r; ++z) {
        int k = 0;
        for (x = -r; x <= r; ++x) {
            int bi = world_biome(&c->g->world, p->dim, p->x + x * step, p->z + z * step);
            char ch = g_biomes[bi].id[0];
            if (x == 0 && z == 0) ch = '@';
            else {
                for (i = 0; i < nleg && strcmp(legend[i], g_biomes[bi].id) != 0; ++i) {
                }
                if (i == nleg && nleg < 40) str_copy(legend[nleg++], g_biomes[bi].id, 24);
                ch = (char)(i < 26 ? 'a' + i : 'A' + (i - 26) % 26);
            }
            line[k++] = ch;
            line[k++] = ' ';
        }
        line[k] = '\0';
        con_out(line);
    }
    for (i = 0; i < nleg; ++i) {
        char b[64];
        sprintf(b, "%c %s", i < 26 ? 'a' + i : 'A' + (i - 26) % 26, biome_name(biome_find(legend[i])));
        con_out(b);
    }
    return CMD_OK;
}

static int c_count(ConCtx *c)
{
    long r = 8, x, y, z, n = 0;
    int b;
    Player *p = &c->g->p;
    char buf[64];
    if (c->argc < 2 || (b = block_find(c->argv[1])) < 0) return con_usage(c);
    if (c->argc >= 3 && (!con_arg_long(c, 2, &r) || r < 1 || r > 16)) return con_usage(c);
    for (y = p->y - r; y <= p->y + r; ++y)
        for (z = p->z - r; z <= p->z + r; ++z)
            for (x = p->x - r; x <= p->x + r; ++x)
                if (world_get(&c->g->world, p->dim, x, y, z) == (cl_u8)b) ++n;
    sprintf(buf, "%s: %ld", block_name(b), n);
    con_out(buf);
    return CMD_OK;
}

static int c_orescan(ConCtx *c)
{
    long r = 8, x, y, z;
    long cnt[MAX_BLOCKS];
    int i, any = 0;
    Player *p = &c->g->p;
    char buf[96];
    if (c->argc >= 2 && (!con_arg_long(c, 1, &r) || r < 1 || r > 16)) return con_usage(c);
    memset(cnt, 0, sizeof(cnt));
    for (y = p->y - r; y <= p->y + r; ++y)
        for (z = p->z - r; z <= p->z + r; ++z)
            for (x = p->x - r; x <= p->x + r; ++x) {
                cl_u8 b = world_get(&c->g->world, p->dim, x, y, z);
                if (g_blocks[b].ore_chance > 0) ++cnt[b];
            }
    for (i = 0; i < g_block_count; ++i) {
        if (cnt[i]) {
            sprintf(buf, "%-20.40s %ld", block_name(i), cnt[i]);
            con_out(buf);
            any = 1;
        }
    }
    if (!any) con_out("-");
    return CMD_OK;
}

static int c_ores(ConCtx *c)
{
    int i;
    char buf[128];
    (void)c;
    for (i = 0; i < g_block_count; ++i) {
        const BlockDef *b = &g_blocks[i];
        if (b->ore_chance > 0) {
            sprintf(buf, "%-20.40s y %3d-%-3d  %3u/10000  in %s", block_name(i), b->ore_ymin, b->ore_ymax,
                    (unsigned)b->ore_chance, block_name(b->ore_host));
            con_out(buf);
        }
    }
    return CMD_OK;
}

static int c_highest(ConCtx *c)
{
    char b[64];
    long top = world_top(&c->g->world, c->g->p.dim, c->g->p.x, c->g->p.z, WORLD_H - 1);
    sprintf(b, "%ld: %s", top, block_name(world_get(&c->g->world, c->g->p.dim, c->g->p.x, top, c->g->p.z)));
    con_out(b);
    return CMD_OK;
}

/* ------------------------------------------------------------ definitions */

static int c_biomeinfo(ConCtx *c)
{
    int b;
    char buf[160];
    const BiomeDef *d;
    if (c->argc < 2) {
        b = world_biome(&c->g->world, c->g->p.dim, c->g->p.x, c->g->p.z);
    } else if ((b = biome_find(c->argv[1])) < 0) {
        return con_usage(c);
    }
    d = &g_biomes[b];
    con_good(biome_name(b));
    sprintf(buf, "%s / %s / %s, %d C, %d%%, trees %u, plants %u", block_name(d->surface), block_name(d->subsurface),
            block_name(d->stone), d->base_temp, d->humidity, (unsigned)d->tree_density, (unsigned)d->plant_density);
    con_out(buf);
    return CMD_OK;
}

static int c_loottables(ConCtx *c)
{
    int i;
    char buf[64];
    (void)c;
    for (i = 0; i < g_loot_count; ++i) {
        sprintf(buf, "%-16.16s %d", g_loot[i].id, g_loot[i].n);
        con_out(buf);
    }
    return CMD_OK;
}

static int c_lootinfo(ConCtx *c)
{
    int l, k;
    char buf[96];
    if (c->argc < 2 || (l = loot_find(c->argv[1])) < 0) return con_usage(c);
    for (k = 0; k < g_loot[l].n; ++k) {
        sprintf(buf, "%-22.40s %d-%d  %d%%", item_name(g_loot[l].item[k]), g_loot[l].lo[k], g_loot[l].hi[k],
                g_loot[l].chance[k]);
        con_out(buf);
    }
    return CMD_OK;
}

static int c_effectlist(ConCtx *c)
{
    int i;
    char buf[128];
    (void)c;
    for (i = 0; i < g_effect_count; ++i) {
        const EffectDef *e = &g_effects[i];
        sprintf(buf, "%-16.20s %-22.40s %3d min  hp %+d  temp %+d  regen %d%%", e->id, effect_name(i), e->duration, e->hp,
                e->temp, e->regen);
        con_out(buf);
    }
    return CMD_OK;
}

static int c_usedin(ConCtx *c)
{
    int it, i, k, n = 0;
    char buf[96];
    if (c->argc < 2 || (it = item_find(c->argv[1])) < 0) return con_usage(c);
    for (i = 0; i < g_recipe_count; ++i) {
        for (k = 0; k < g_recipes[i].nin; ++k) {
            if (g_recipes[i].in_item[k] == it) {
                sprintf(buf, "%-24.40s x%d (%s)", item_name(g_recipes[i].result), g_recipes[i].in_count[k],
                        loc_get(station_key(g_recipes[i].station)));
                con_out(buf);
                ++n;
            }
        }
    }
    if (!n) con_out("-");
    return CMD_OK;
}

static int search(ConCtx *c, int blocks)
{
    int i, n = 0, total = blocks ? g_block_count : g_item_count;
    char buf[96], low[40], name[64];
    if (c->argc < 2) return con_usage(c);
    str_copy(low, c->argv[1], sizeof(low));
    str_lower(low);
    for (i = 0; i < total && n < 40; ++i) {
        const char *id = blocks ? g_blocks[i].id : g_items[i].id;
        str_copy(name, blocks ? block_name(i) : item_name(i), sizeof(name));
        str_lower(name);
        if (strstr(id, low) || strstr(name, low)) {
            sprintf(buf, "%-22.24s %.40s", id, blocks ? block_name(i) : item_name(i));
            con_out(buf);
            ++n;
        }
    }
    if (!n) con_out("-");
    return CMD_OK;
}

static int c_itemsearch(ConCtx *c) { return search(c, 0); }
static int c_blocksearch(ConCtx *c) { return search(c, 1); }

/* ------------------------------------------------------------ creatures */

static int c_nearest(ConCtx *c)
{
    int i, best = -1, def = -1;
    long bd = 0x7FFFFFFL;
    Player *p = &c->g->p;
    char buf[128];
    if (c->argc >= 2 && (def = creature_find(c->argv[1])) < 0) return con_usage(c);
    for (i = 0; i < MAX_MOBS; ++i) {
        const Mob *m = &c->g->mobs[i];
        long dx, dz;
        if (!m->active || m->dim != p->dim || (def >= 0 && m->def != def)) continue;
        dx = m->x - p->x;
        dz = m->z - p->z;
        if (dx * dx + dz * dz < bd) {
            bd = dx * dx + dz * dz;
            best = i;
        }
    }
    if (best < 0) {
        con_out("-");
        return CMD_OK;
    }
    sprintf(buf, "%s: %ld m %s (%ld,%ld,%ld)", creature_name(c->g->mobs[best].def), mx_isqrt(bd),
            dir8(c->g->mobs[best].x - p->x, c->g->mobs[best].z - p->z), c->g->mobs[best].x, c->g->mobs[best].y,
            c->g->mobs[best].z);
    con_out(buf);
    return CMD_OK;
}

static int c_hostiles(ConCtx *c)
{
    int i, n = 0;
    Player *p = &c->g->p;
    char buf[128];
    for (i = 0; i < MAX_MOBS; ++i) {
        const Mob *m = &c->g->mobs[i];
        long dx, dz;
        if (!m->active || m->dim != p->dim) continue;
        if (g_creatures[m->def].kind != MK_MONSTER && !m->aggro && g_creatures[m->def].aggression < AGG_NIGHT) continue;
        dx = m->x - p->x;
        dz = m->z - p->z;
        sprintf(buf, "%-20.40s %ld m %s  %s", creature_name(m->def), mx_isqrt(dx * dx + dz * dz), dir8(dx, dz),
                loc_get(mob_state_key(m->state)));
        con_out(buf);
        ++n;
    }
    if (!n) con_out(loc_get("con.threat_no"));
    return CMD_OK;
}

static int c_lootof(ConCtx *c)
{
    int d, l, k;
    char buf[96];
    if (c->argc < 2 || (d = creature_find(c->argv[1])) < 0) return con_usage(c);
    l = g_creatures[d].loot[0] ? loot_find(g_creatures[d].loot) : -1;
    if (l < 0) {
        con_out("-");
        return CMD_OK;
    }
    for (k = 0; k < g_loot[l].n; ++k) {
        sprintf(buf, "%-22.40s %d-%d  %d%%", item_name(g_loot[l].item[k]), g_loot[l].lo[k], g_loot[l].hi[k],
                g_loot[l].chance[k]);
        con_out(buf);
    }
    return CMD_OK;
}

static int c_cargo(ConCtx *c)
{
    int k, s = c->g->aboard;
    char buf[96];
    if (s < 0 || c->g->ships[s].chest < 0) {
        con_err(loc_get(s < 0 ? "con.not_aboard" : "msg.ship_no_cargo"));
        return CMD_ERR;
    }
    for (k = 0; k < CHEST_SLOTS; ++k) {
        const ItemStack *st = &c->g->chests[c->g->ships[s].chest].s[k];
        if (st->item >= 0) {
            sprintf(buf, "%2d %-24.40s x%d", k, item_name(st->item), st->count);
            con_out(buf);
        }
    }
    sprintf(buf, "%ld / %d kg", ship_load(c->g, s), g_ships[c->g->ships[s].def].capacity);
    con_good(buf);
    return CMD_OK;
}

static int c_portals(ConCtx *c)
{
    long r = 24, x, y, z;
    int pb = block_find("ancient_portal"), n = 0;
    Player *p = &c->g->p;
    char buf[96];
    if (c->argc >= 2 && (!con_arg_long(c, 1, &r) || r < 1 || r > 32)) return con_usage(c);
    for (y = CL_MAX(1L, p->y - 8); y <= CL_MIN((long)WORLD_H - 1, p->y + 8); ++y)
        for (z = p->z - r; z <= p->z + r; ++z)
            for (x = p->x - r; x <= p->x + r; ++x)
                if (world_get(&c->g->world, p->dim, x, y, z) == (cl_u8)pb && n < 20) {
                    sprintf(buf, "%ld,%ld,%ld  %ld m %s", x, y, z, mx_isqrt((x - p->x) * (x - p->x) + (z - p->z) * (z - p->z)),
                            dir8(x - p->x, z - p->z));
                    con_out(buf);
                    ++n;
                }
    if (!n) con_out("-");
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "survival", "player", "", "", NULL, PERM_ANY, c_survival, "status" },
    { "effects", "player", "", "", NULL, PERM_ANY, c_effects, NULL },
    { "skills", "player", "", "", NULL, PERM_ANY, c_skills, "xp" },
    { "carry", "player", "", "", NULL, PERM_ANY, c_carry, "weight" },
    { "noise", "player", "", "", NULL, PERM_ANY, c_noise, "stealth" },
    { "whereami", "position", "", "", NULL, PERM_ANY, c_whereami, NULL },
    { "compass", "position", "", "", NULL, PERM_ANY, c_compass, NULL },
    { "distance", "position", "<x> <z>", "nn", NULL, PERM_ANY, c_distance, NULL },
    { "look", "player", "", "", NULL, PERM_ANY, c_look, "examine" },
    { "playtime", "system", "", "", NULL, PERM_ANY, c_playtime, NULL },
    { "date", "time", "", "", NULL, PERM_ANY, c_date, "calendar" },
    { "moon", "time", "", "", NULL, PERM_ANY, c_moon, NULL },
    { "sun", "time", "", "", NULL, PERM_ANY, c_sun, NULL },
    { "forecast", "weather", "", "", NULL, PERM_ANY, c_forecast, NULL },
    { "weathers", "weather", "", "", NULL, PERM_ANY, c_weathers, NULL },
    { "light", "world", "", "", NULL, PERM_ANY, c_light, NULL },
    { "climate", "world", "[x z]", "nn", NULL, PERM_ANY, c_climate, NULL },
    { "height", "world", "[x z]", "nn", NULL, PERM_ANY, c_height, NULL },
    { "depth", "world", "", "", NULL, PERM_ANY, c_depth, NULL },
    { "biomeat", "world", "<x> <z>", "nn", NULL, PERM_ANY, c_biomeat, NULL },
    { "blockat", "world", "<x> <y> <z>", "nnn", NULL, PERM_ANY, c_blockat, NULL },
    { "survey", "world", "[radius 4-64]", "n", NULL, PERM_ANY, c_survey, NULL },
    { "map", "world", "[cells per letter]", "n", NULL, PERM_ANY, c_map, "biomemap" },
    { "count", "world", "<block> [radius 1-16]", "bn", NULL, PERM_CHEAT, c_count, NULL },
    { "orescan", "world", "[radius 1-16]", "n", NULL, PERM_CHEAT, c_orescan, NULL },
    { "ores", "world", "", "", NULL, PERM_ANY, c_ores, NULL },
    { "highest", "world", "", "", NULL, PERM_ANY, c_highest, NULL },
    { "biomeinfo", "world", "[biome]", "B", NULL, PERM_ANY, c_biomeinfo, NULL },
    { "loottables", "items", "", "", NULL, PERM_ANY, c_loottables, NULL },
    { "lootinfo", "items", "<table>", "*", NULL, PERM_ANY, c_lootinfo, NULL },
    { "effectlist", "player", "", "", NULL, PERM_ANY, c_effectlist, NULL },
    { "usedin", "crafting", "<item>", "i", NULL, PERM_ANY, c_usedin, "uses" },
    { "itemsearch", "items", "<text>", "*", NULL, PERM_ANY, c_itemsearch, "finditem" },
    { "blocksearch", "items", "<text>", "*", NULL, PERM_ANY, c_blocksearch, "findblock" },
    { "nearest", "creatures", "[creature]", "*", NULL, PERM_ANY, c_nearest, NULL },
    { "hostiles", "creatures", "", "", NULL, PERM_ANY, c_hostiles, NULL },
    { "lootof", "creatures", "<creature>", "*", NULL, PERM_ANY, c_lootof, NULL },
    { "cargo", "ships", "", "", NULL, PERM_ANY, c_cargo, "hold" },
    { "portals", "world", "[radius 1-32]", "n", NULL, PERM_ANY, c_portals, NULL }
};

CommandTable cmd_table_info(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
