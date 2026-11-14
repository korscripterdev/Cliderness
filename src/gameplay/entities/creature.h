/*
 * Cliderness - gameplay/entities/creature.h
 * Data-driven creatures (data/entities/creatures.def): animals, monsters, NPCs.
 *
 * AI = a state machine driven by goals. Every "think" (10 ticks) a creature
 * picks the most urgent goal and switches state:
 *   1 survive   hp below flee_pct          -> FLEE from the threat
 *   2 attack    hostile + perceives player  -> CHASE / ATTACK
 *   3 shelter   burns in daylight / rain    -> seek cover (stay covered)
 *   4 home      NPC at night                -> walk HOME, then SLEEP
 *   5 rest      diurnal at night, nocturnal by day -> SLEEP
 *   6 wander    otherwise                   -> WANDER / IDLE
 * Perception: sight radius (scaled by light and weather visibility) and
 * hearing (player noise % x hearing / 100 cells). Sleeping halves both.
 */
#ifndef CLIDE_GAMEPLAY_CREATURE_H
#define CLIDE_GAMEPLAY_CREATURE_H

#include "core/types.h"

#define MAX_CREATURE_DEFS 48
#define TRADE_MAX 3

enum { MK_ANIMAL = 0, MK_MONSTER, MK_NPC };
enum { AGG_NEVER = 0, AGG_DEFENSIVE, AGG_NIGHT, AGG_ALWAYS };
enum { ACTV_DAY = 0, ACTV_NIGHT, ACTV_ANY };
enum { SPAWN_SURFACE = 0, SPAWN_CAVE, SPAWN_WATER };
enum { MS_IDLE = 0, MS_WANDER, MS_CHASE, MS_ATTACK, MS_FLEE, MS_SLEEP, MS_HOME, MS_SHELTER, MS_COUNT };

typedef struct CreatureDef {
    char   id[24];
    char   name[28];
    cl_u8  glyph, color;
    cl_u8  kind, aggression, activity, habitat;
    cl_u8  dim;
    cl_u8  burns;          /* takes damage in daylight */
    cl_u8  aquatic;        /* lives only in water */
    cl_s16 hp, damage, defense;
    cl_s16 speed;          /* ticks per step */
    cl_s16 attack_cd;      /* ticks between attacks */
    cl_s16 sight, hearing;
    cl_s16 flee_pct;
    cl_s16 weight;         /* spawn weight */
    cl_u8  group_min, group_max;
    cl_s16 xp;
    cl_s16 effect;         /* status effect inflicted on hit (-1) */
    cl_u8  effect_chance;
    char   loot[16];
    char   biomes[96];     /* comma separated biome ids, empty = any */
    cl_s16 trade_in[TRADE_MAX], trade_in_n[TRADE_MAX];
    cl_s16 trade_out[TRADE_MAX], trade_out_n[TRADE_MAX];
    cl_u8  ntrades;
} CreatureDef;

extern CreatureDef g_creatures[MAX_CREATURE_DEFS];
extern int g_creature_count;

int  creatures_load(const char *root, char *err, size_t errn);
int  creature_find(const char *id);
const char *creature_name(int c);
const char *mob_state_key(int st);

#endif
