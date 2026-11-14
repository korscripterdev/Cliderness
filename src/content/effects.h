/*
 * Cliderness - content/effects.h
 * Data-driven status effects (data/effects/effects.def): injuries, diseases
 * and buffs. Each effect applies per-game-minute deltas while it lasts.
 */
#ifndef CLIDE_CONTENT_EFFECTS_H
#define CLIDE_CONTENT_EFFECTS_H

#include "core/types.h"

#define MAX_EFFECTS 24

typedef struct EffectDef {
    char   id[20];
    char   name[28];
    cl_s16 hp;            /* per game minute (negative = damage) */
    cl_s16 hunger;        /* per minute on the 0..1000 meter */
    cl_s16 thirst;
    cl_s16 stamina;       /* per minute */
    cl_s16 temp;          /* environment temperature offset, deg C */
    cl_s16 move;          /* extra ticks per step */
    cl_s16 regen;         /* stamina regeneration percent (100 = normal) */
    cl_s16 duration;      /* default minutes */
    cl_u8  harmful;
    cl_u8  color;
} EffectDef;

extern EffectDef g_effects[MAX_EFFECTS];
extern int g_effect_count;

int  effects_load(const char *root, char *err, size_t errn);
int  effect_find(const char *id);
const char *effect_name(int e);

#endif
