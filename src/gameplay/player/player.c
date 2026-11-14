/* Cliderness - gameplay/player/player.c */
#include <string.h>
#include "gameplay/player/player.h"
#include "core/mathx.h"
#include "core/str.h"

static const char *const g_stat_ids[ST_COUNT] = {
    "strength", "dexterity", "intelligence", "perception",
    "constitution", "endurance", "willpower", "luck"
};
static const char *const g_stat_short[ST_COUNT] = {
    "str", "dex", "int", "per", "con", "end", "wil", "lck"
};
static const char *const g_stat_keys[ST_COUNT] = {
    "stat.str", "stat.dex", "stat.int", "stat.per", "stat.con", "stat.end", "stat.wil", "stat.lck"
};
static const char *const g_skill_ids[SK_COUNT] = {
    "mining", "woodcutting", "building", "crafting", "melee", "ranged",
    "swimming", "sailing", "fishing", "cooking", "stealth", "exploration"
};
static const char *const g_move_keys[] = {
    "move.walk", "move.sprint", "move.crouch", "move.swim", "move.fly", "move.idle"
};

void player_init(Player *p)
{
    int i;
    memset(p, 0, sizeof(*p));
    for (i = 0; i < ST_COUNT; ++i) {
        p->stats[i] = 10;
    }
    p->hp_max = 100;
    p->hp = 100;
    p->stamina_max = 1000;
    p->stamina = 1000;
    p->hunger = 900;
    p->thirst = 900;
    p->fatigue = 0;
    p->body_temp = 370;
    p->env_temp = 18;
    p->facing = DIR_S;
    p->pending_dir = -1;
    p->move_mode = MOVE_IDLE;
    p->visibility = 100;
}

long player_level_xp(int level)
{
    return 10L * (long)level * (long)level;
}

int player_skill_level(const Player *p, int skill)
{
    long xp = p->skill_xp[skill];
    long lvl = mx_isqrt(xp / 10L);
    return (int)(lvl > SKILL_MAX_LEVEL ? SKILL_MAX_LEVEL : lvl);
}

long player_carry_capacity(const Player *p)
{
    return 20000L + (long)p->stats[ST_STR] * 3000L;
}

const char *player_stat_key(int st)
{
    return g_stat_keys[st];
}

const char *player_skill_key(int sk)
{
    static char buf[SK_COUNT][24];
    str_copy(buf[sk], "skill.", sizeof(buf[sk]));
    str_cat(buf[sk], g_skill_ids[sk], sizeof(buf[sk]));
    return buf[sk];
}

const char *player_move_key(int mode)
{
    return g_move_keys[(mode >= 0 && mode <= MOVE_IDLE) ? mode : 0];
}

int player_find_stat(const char *name)
{
    int i;
    for (i = 0; i < ST_COUNT; ++i) {
        if (str_ieq(name, g_stat_ids[i]) || str_ieq(name, g_stat_short[i])) {
            return i;
        }
    }
    return -1;
}

int player_find_skill(const char *name)
{
    int i;
    for (i = 0; i < SK_COUNT; ++i) {
        if (str_ieq(name, g_skill_ids[i])) {
            return i;
        }
    }
    return -1;
}

void dir_delta(int dir, long *dx, long *dz)
{
    *dx = 0;
    *dz = 0;
    switch (dir & 3) {
    case DIR_N: *dz = -1; break;
    case DIR_E: *dx = 1; break;
    case DIR_S: *dz = 1; break;
    default: *dx = -1; break;
    }
}
