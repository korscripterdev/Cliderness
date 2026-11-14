/*
 * Cliderness - gameplay/survival/survival.c
 * Hunger, thirst, fatigue, stamina, body temperature, HP, skill experience.
 *
 * Per game minute (survival/adventure, not god):
 *   hunger  -0.5 (x2 if the player sprinted this minute)
 *   thirst  -0.7 (x1.5 when the environment is above 28 C)
 *   fatigue +0.4
 *   body temperature drifts toward the environment outside 10..28 C,
 *     rate = |env - limit| * 10 / (20 + WIL) tenths of a degree
 *   damage: starving -1 HP / 2 min, dehydrated -1 HP / min,
 *           body temp < 35.0 or > 39.5 C -1 HP / min
 *   regeneration: +1 HP / 3 min when hunger and thirst > 600
 */
#include "gameplay/game.h"
#include "content/content.h"
#include "world/biomes/biome.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/log.h"
#include "core/mathx.h"
#include "content/effects.h"

#define XP_CAP_PER_MINUTE 60

static int survival_on(const Game *g)
{
    return (g->mode == GM_SURVIVAL || g->mode == GM_ADVENTURE) && !g->p.god;
}

static void acc_step(int *acc, int *value, int tenths, int lo, int hi)
{
    *acc += tenths;
    while (*acc >= 10) {
        *acc -= 10;
        *value += (tenths > 0) ? 1 : -1;
    }
    while (*acc <= -10) {
        *acc += 10;
        *value -= 1;
    }
    *value = CL_CLAMP(*value, lo, hi);
}

int surv_env_temp(Game *g)
{
    Player *p = &g->p;
    int biome = world_biome(&g->world, p->dim, p->x, p->z);
    const BiomeDef *b = &g_biomes[biome];
    ClideDate d;
    int t = b->base_temp;
    int light = ctime_daylight(&g->time);
    int surface = world_gen_height(&g->world, p->dim, p->x, p->z);
    static const int season_delta[4] = { 0, 7, -1, -10 };
    ctime_date(&g->time, &d);
    t += season_delta[d.season & 3];
    t += (light - 50) / 8;
    if (p->y > 80) {
        t -= (int)((p->y - 80) / 5);
    }
    if (game_is_covered(g, p->x, p->y, p->z) && p->y < surface - 3) {
        /* deep underground: stable temperature */
        t = 12 + (int)((surface - p->y) / 16);
    } else {
        t += weather_cur(&g->weather)->temp_delta;
    }
    if (blk_liquid(world_get(&g->world, p->dim, p->x, p->y - 1, p->z))) {
        t -= 4;
    }
    t += fx_temp(g);
    {
        /* clothing keeps you warm in the cold; metal armour is cold */
        int ins = inv_insulation(&g->inv);
        if (ins > 0 && t < 20) {
            t = CL_MIN(20, t + ins);
        } else if (ins < 0) {
            t += ins;
        }
    }
    return t;
}

void surv_damage(Game *g, int amount, const char *cause_key)
{
    Player *p = &g->p;
    int i;
    if (p->dead || p->god || g->mode == GM_CREATIVE || g->mode == GM_SPECTATOR || amount <= 0) {
        return;
    }
    p->hp -= amount;
    if (p->hp > 0) {
        log_add(ATTR(C_LIGHTRED, C_BLACK), loc_fmt("msg.hurt", loc_get(cause_key), NULL, NULL));
        return;
    }
    p->hp = 0;
    p->dead = 1;
    p->mining = 0;
    ++g->deaths;
    g->death_x = p->x;
    g->death_y = p->y;
    g->death_z = p->z;
    g->death_dim = p->dim;
    log_add(ATTR(C_LIGHTRED, C_BLACK), loc_fmt("msg.died", loc_get(cause_key), NULL, NULL));
    /* drop everything where the player died */
    for (i = 0; i < INV_SLOTS + INV_EQUIP; ++i) {
        ItemStack *s = i < INV_SLOTS ? &g->inv.s[i] : &g->inv.equip[i - INV_SLOTS];
        if (s->item >= 0 && ent_spawn(g, p->dim, p->x, p->y, p->z, s->item, s->count, s->dur) >= 0) {
            s->item = -1;
            s->count = 0;
        }
    }
    fx_clear(g);
    log_add(ATTR(C_YELLOW, C_BLACK), loc_get("msg.press_respawn"));
}

void skill_gain(Game *g, int skill, int xp)
{
    Player *p = &g->p;
    int before, after;
    if (xp <= 0 || skill < 0 || skill >= SK_COUNT) {
        return;
    }
    xp = xp * (100 + p->stats[ST_INT] * 3) / 130;
    if (xp < 1) {
        xp = 1;
    }
    if (p->xp_minute + xp > XP_CAP_PER_MINUTE) {
        xp = XP_CAP_PER_MINUTE - p->xp_minute;
        if (xp <= 0) {
            return;
        }
    }
    p->xp_minute += xp;
    before = player_skill_level(p, skill);
    p->skill_xp[skill] += xp;
    after = player_skill_level(p, skill);
    if (after > before) {
        log_add(ATTR(C_LIGHTCYAN, C_BLACK),
                loc_fmt("msg.skill_up", loc_get(player_skill_key(skill)), NULL, NULL));
    }
}

void surv_tick(Game *g)
{
    Player *p = &g->p;
    int regen;
    p->stamina_max = 1000;
    if (p->fatigue > 800) {
        p->stamina_max = 600;
    }
    if (p->move_mode != MOVE_SPRINT && p->move_mode != MOVE_SWIM && !ship_rowing(g)) {
        regen = 3 + p->stats[ST_END] / 5;
        if (p->move_mode == MOVE_IDLE || p->move_mode == MOVE_CROUCH) {
            regen += 2;
        }
        if (p->hunger < 150 || p->thirst < 150) {
            regen /= 2;
        }
        regen = regen * fx_regen(g) / 100;
        p->stamina += regen;
    }
    if (p->stamina > p->stamina_max) {
        p->stamina = p->stamina_max;
    }
    if (!survival_on(g)) {
        p->stamina = p->stamina_max;
    }
    {
        int light = ctime_daylight(&g->time);
        ClideDate d;
        int moon;
        ctime_date(&g->time, &d);
        moon = 4 - mx_abs(d.moon_phase - 4);
        if (light < 6 + moon * 3) {
            light = 6 + moon * 3;
        }
        p->visibility = light * weather_cur(&g->weather)->visibility / 100;
    }
}

void surv_minute(Game *g)
{
    Player *p = &g->p;
    int env;
    p->xp_minute = 0;
    env = surv_env_temp(g);
    p->env_temp = env;
    if (!survival_on(g)) {
        p->hunger = CL_MAX(p->hunger, 500);
        p->thirst = CL_MAX(p->thirst, 500);
        p->body_temp = 370;
        p->sprinted = 0;
        return;
    }
    if (p->dead) {
        return;
    }
    fx_minute(g);
    if (p->dead) {
        return;
    }
    /* wet: swimming, or rain while outdoors */
    if ((g->aboard < 0 && blk_liquid(world_get(&g->world, p->dim, p->x, p->y - 1, p->z))) ||
        (weather_cur(&g->weather)->precip == PRECIP_RAIN && !game_is_covered(g, p->x, p->y, p->z))) {
        fx_add(g, effect_find("wet"), 20);
    }
    /* long hypothermia causes a cold */
    if (p->body_temp < 355) {
        if (++p->cold_minutes >= 30) {
            p->cold_minutes = 0;
            fx_add(g, effect_find("common_cold"), 0);
        }
    } else if (p->cold_minutes > 0) {
        --p->cold_minutes;
    }
    acc_step(&p->acc_hunger, &p->hunger, p->sprinted > 0 ? -10 : -5, 0, 1000);
    acc_step(&p->acc_thirst, &p->thirst, env > 28 ? -10 : -7, 0, 1000);
    acc_step(&p->acc_fatigue, &p->fatigue, 4, 0, 1000);
    p->stamina -= weather_cur(&g->weather)->stamina;
    if (p->stamina < 0) {
        p->stamina = 0;
    }
    p->sprinted = 0;
    if (env < 10) {
        p->body_temp -= (10 - env) * 10 / (20 + p->stats[ST_WIL]);
    } else if (env > 28) {
        p->body_temp += (env - 28) * 10 / (20 + p->stats[ST_WIL]);
    } else if (p->body_temp < 370) {
        p->body_temp += 2;
        if (p->body_temp > 370) p->body_temp = 370;
    } else if (p->body_temp > 370) {
        p->body_temp -= 2;
        if (p->body_temp < 370) p->body_temp = 370;
    }
    p->body_temp = CL_CLAMP(p->body_temp, 300, 420);
    if (p->hunger == 0 && (g->last_minute & 1)) {
        surv_damage(g, 1, "cause.hunger");
    }
    if (p->thirst == 0) {
        surv_damage(g, 1, "cause.thirst");
    }
    if (p->body_temp < 350) {
        surv_damage(g, 1, "cause.cold");
    } else if (p->body_temp > 395) {
        surv_damage(g, 1, "cause.heat");
    }
    if (!p->dead && p->hunger > 600 && p->thirst > 600 && p->hp < p->hp_max) {
        if (++p->acc_regen >= 3) {
            p->acc_regen = 0;
            ++p->hp;
        }
    }
    if (p->hunger == 150 || p->thirst == 150) {
        log_add(ATTR(C_YELLOW, C_BLACK), loc_get(p->hunger == 150 ? "msg.hungry" : "msg.thirsty"));
    }
}
