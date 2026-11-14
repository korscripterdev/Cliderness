/*
 * Cliderness - gameplay/entities/entity.h
 * Dropped item entities. Items in water float or sink depending on the
 * item density (kg/m3) compared with water (1000).
 */
#ifndef CLIDE_GAMEPLAY_ENTITY_H
#define CLIDE_GAMEPLAY_ENTITY_H

#include "core/types.h"

#ifdef CLIDE_TARGET_DOS16
#define MAX_ITEM_ENTS 96
#else
#define MAX_ITEM_ENTS 512
#endif
#define ITEM_DESPAWN_TICKS 24000L   /* 20 real minutes at 20 ticks/s */

typedef struct ItemEntity {
    long   x, y, z;
    long   age;
    cl_s16 item, count, dur;
    cl_u8  dim;
    cl_u8  active;
    cl_u8  in_water;
    cl_u8  floating;
} ItemEntity;

#endif
