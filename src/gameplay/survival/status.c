/*
 * Cliderness - gameplay/survival/status.c
 * Status effects (injuries, illness, buffs) and sleeping.
 *
 * Triggers (implemented in game.c / survival.c):
 *   fall of 7+ blocks -> fracture, fall damage >= 16 -> bleeding,
 *   swimming or rain outdoors -> wet, 30 minutes below 35.5 C -> common cold,
 *   food with effect= (chance effect_chance) -> that effect,
 *   sleeping in a bed -> rested.
 */
#include <stdio.h>
#include <string.h>
#include "gameplay/game.h"
#include "content/content.h"
#include "content/effects.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/log.h"

void fx_clear(Game *g)
{
    int i;
    for (i = 0; i < PLAYER_FX; ++i) {
        g->p.fx[i].id = -1;
        g->p.fx[i].minutes = 0;
    }
}

int fx_has(const Game *g, int e)
{
    int i;
    for (i = 0; i < PLAYER_FX; ++i) {
        if (g->p.fx[i].id == e && g->p.fx[i].minutes > 0) {
            return 1;
        }
    }
    return 0;
}

void fx_add(Game *g, int e, int minutes)
{
    int i, free_slot = -1;
    if (e < 0 || e >= g_effect_count) {
        return;
    }
    if (g->p.god && g_effects[e].harmful) {
        return;
    }
    if (minutes <= 0) {
        minutes = g_effects[e].duration;
    }
    for (i = 0; i < PLAYER_FX; ++i) {
        if (g->p.fx[i].id == e && g->p.fx[i].minutes > 0) {
            if (g->p.fx[i].minutes < minutes) {
                g->p.fx[i].minutes = (cl_s16)minutes;
            }
            return;
        }
        if ((g->p.fx[i].id < 0 || g->p.fx[i].minutes <= 0) && free_slot < 0) {
            free_slot = i;
        }
    }
    if (free_slot < 0) {
        return;
    }
    g->p.fx[free_slot].id = (cl_s16)e;
    g->p.fx[free_slot].minutes = (cl_s16)minutes;
    log_add(ATTR(g_effects[e].harmful ? C_LIGHTRED : C_LIGHTGREEN, C_BLACK),
            loc_fmt("msg.effect_start", effect_name(e), NULL, NULL));
}

int fx_remove(Game *g, int e)
{
    int i, n = 0;
    for (i = 0; i < PLAYER_FX; ++i) {
        if (g->p.fx[i].id == e && g->p.fx[i].minutes > 0) {
            g->p.fx[i].id = -1;
            g->p.fx[i].minutes = 0;
            ++n;
        }
    }
    return n;
}

void fx_minute(Game *g)
{
    int i;
    Player *p = &g->p;
    for (i = 0; i < PLAYER_FX; ++i) {
        const EffectDef *e;
        if (p->fx[i].id < 0 || p->fx[i].minutes <= 0) {
            continue;
        }
        e = &g_effects[p->fx[i].id];
        if (e->hp < 0) {
            surv_damage(g, -e->hp, "cause.effect");
        } else if (e->hp > 0 && p->hp < p->hp_max) {
            p->hp = CL_MIN(p->hp_max, p->hp + e->hp);
        }
        p->hunger = CL_CLAMP(p->hunger + e->hunger, 0, 1000);
        p->thirst = CL_CLAMP(p->thirst + e->thirst, 0, 1000);
        p->stamina = CL_CLAMP(p->stamina + e->stamina, 0, p->stamina_max);
        if (--p->fx[i].minutes <= 0) {
            log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_fmt("msg.effect_end", effect_name(p->fx[i].id), NULL, NULL));
            p->fx[i].id = -1;
        }
    }
}

int fx_temp(const Game *g)
{
    int i, t = 0;
    for (i = 0; i < PLAYER_FX; ++i) {
        if (g->p.fx[i].id >= 0 && g->p.fx[i].minutes > 0) {
            t += g_effects[g->p.fx[i].id].temp;
        }
    }
    return t;
}

int fx_move(const Game *g)
{
    int i, m = 0;
    for (i = 0; i < PLAYER_FX; ++i) {
        if (g->p.fx[i].id >= 0 && g->p.fx[i].minutes > 0) {
            m += g_effects[g->p.fx[i].id].move;
        }
    }
    return m;
}

int fx_regen(const Game *g)
{
    int i;
    long r = 100;
    for (i = 0; i < PLAYER_FX; ++i) {
        if (g->p.fx[i].id >= 0 && g->p.fx[i].minutes > 0) {
            r = r * g_effects[g->p.fx[i].id].regen / 100L;
        }
    }
    return (int)CL_CLAMP(r, 10L, 400L);
}

/*
 * Sleep until morning (sunrise + 30 min) or until fatigue is gone.
 * Each skipped minute runs the normal world minute (weather, crops) and a
 * reduced survival minute: fatigue -2, hunger/thirst at half rate, +1 HP
 * every 2 minutes. Not possible while hostile creatures are near (entities).
 */
int game_sleep(Game *g, int force)
{
    Player *p = &g->p;
    ClideDate d;
    long now, target, n, i;
    int light = ctime_daylight(&g->time);
    if (!force && light > 30 && p->fatigue < 600) {
        log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get("msg.cannot_sleep"));
        return 0;
    }
    if (!force && ent_hostile_near(g, 12)) {
        log_add(ATTR(C_LIGHTRED, C_BLACK), loc_get("msg.monsters_near"));
        return 0;
    }
    ctime_date(&g->time, &d);
    now = ctime_minute_of_day(&g->time);
    target = d.sunrise_min + 30;
    n = (now < target) ? target - now : 1440 - now + target;
    if (n > 720) {
        n = 720;
    }
    if (light > 30) {
        n = CL_MIN(n, (long)(p->fatigue / 2));   /* a nap */
    }
    p->sleeping = 1;
    for (i = 0; i < n; ++i) {
        ctime_add_seconds(&g->time, 60L);
        ++g->last_minute;
        weather_minute(&g->weather, world_climate_temp(&g->world, p->dim, p->x, p->z), 50, p->dim);
        farm_minute(g);
        p->fatigue = CL_MAX(0, p->fatigue - 2);
        if (g->mode == GM_SURVIVAL || g->mode == GM_ADVENTURE) {
            if (i & 1) {
                p->hunger = CL_MAX(0, p->hunger - 1);
                if (p->hp < p->hp_max) ++p->hp;
            }
            if ((i % 3) == 0) {
                p->thirst = CL_MAX(0, p->thirst - 1);
            }
        }
    }
    g->last_minute = ctime_total_minutes(&g->time);
    p->sleeping = 0;
    p->stamina = p->stamina_max;
    fx_add(g, effect_find("rested"), 0);
    {
        char num[24];
        sprintf(num, "%ld", n / 60L);
        log_add(ATTR(C_LIGHTCYAN, C_BLACK), loc_fmt("msg.slept", num, NULL, NULL));
    }
    return 1;
}
