/*
 * Cliderness - gameplay/player/player.h
 * Player state: position, survival meters, attributes and skills.
 * Meters use 0..1000 integer scales (no floating point anywhere).
 */
#ifndef CLIDE_GAMEPLAY_PLAYER_H
#define CLIDE_GAMEPLAY_PLAYER_H

#include "core/types.h"

enum { ST_STR = 0, ST_DEX, ST_INT, ST_PER, ST_CON, ST_END, ST_WIL, ST_LCK, ST_COUNT };
enum {
    SK_MINING = 0, SK_WOODCUTTING, SK_BUILDING, SK_CRAFTING, SK_MELEE, SK_RANGED,
    SK_SWIMMING, SK_SAILING, SK_FISHING, SK_COOKING, SK_STEALTH, SK_EXPLORATION, SK_COUNT
};
enum { MOVE_WALK = 0, MOVE_SPRINT, MOVE_CROUCH, MOVE_SWIM, MOVE_FLY, MOVE_IDLE };
enum { DIR_N = 0, DIR_E, DIR_S, DIR_W };

#define SKILL_MAX_LEVEL 100
#define PLAYER_FX 8

typedef struct Player {
    long x, y, z;
    int  dim;
    int  facing;
    int  hp, hp_max;
    int  stamina, stamina_max;
    int  hunger;          /* satiety 1000 = full, 0 = starving */
    int  thirst;          /* hydration 1000 = full */
    int  fatigue;         /* 0 = rested, 1000 = exhausted */
    int  body_temp;       /* tenths of deg C (370 = 37.0) */
    int  env_temp;        /* deg C */
    int  move_mode;
    int  crouch;
    int  cooldown;
    int  pending_dir;
    int  pending_sprint;
    int  noise;           /* 0..100 */
    int  visibility;      /* 0..100 */
    int  sprinted;        /* sprint steps during the current minute */
    int  stats[ST_COUNT];
    long skill_xp[SK_COUNT];
    int  xp_minute;       /* xp gained this game minute (anti-farming cap) */
    long best_dist;       /* farthest distance from spawn (exploration xp) */
    /* mining action in progress */
    int  mining;
    long mx, my, mz;
    int  mine_progress, mine_total;
    /* cheat flags (independent from game mode) */
    int  god, noclip, fly;
    int  dead;
    long spawn_x, spawn_y, spawn_z;
    /* survival accumulators (tenths) */
    int  acc_hunger, acc_thirst, acc_fatigue, acc_regen;
    /* status effects (content/effects.h) */
    struct { cl_s16 id; cl_s16 minutes; } fx[PLAYER_FX];
    int  cold_minutes;      /* minutes spent hypothermic (common cold trigger) */
    int  sleeping;
} Player;

void player_init(Player *p);
int  player_skill_level(const Player *p, int skill);
/* xp needed to reach a level: 10 * level^2 */
long player_level_xp(int level);
long player_carry_capacity(const Player *p);  /* grams */
const char *player_stat_key(int st);
const char *player_skill_key(int sk);
const char *player_move_key(int mode);
int  player_find_stat(const char *name);
int  player_find_skill(const char *name);
void dir_delta(int dir, long *dx, long *dz);

#endif
