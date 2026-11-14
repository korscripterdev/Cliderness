/*
 * Cliderness - world/biomes/biome.h
 * Data-driven biomes (data/biomes/<file>.def). The first biome (file order) whose
 * height / temperature / humidity / special ranges all contain the sampled
 * climate wins, so adding a biome needs no code change.
 */
#ifndef CLIDE_WORLD_BIOME_H
#define CLIDE_WORLD_BIOME_H

#include "core/types.h"

#define MAX_BIOMES 64

enum { BK_LAND = 0, BK_COAST, BK_OCEAN, BK_CAVE, BK_ANCIENT, BK_COUNT };

typedef struct BiomeDef {
    char   id[24];
    char   name[28];
    cl_u8  kind;
    cl_s16 h_lo, h_hi;          /* surface height range (absolute y) */
    cl_s16 t_lo, t_hi;          /* climate temperature 0..100 */
    cl_s16 m_lo, m_hi;          /* humidity 0..100 */
    cl_s16 s_lo, s_hi;          /* special/rarity noise 0..100 */
    cl_s16 base_temp;           /* deg C at noon, spring */
    cl_s16 humidity;            /* % */
    cl_u8  surface, subsurface, seabed, stone;
    cl_u8  tree, tree_top, tree2, plant, plant2, plant3;
    cl_u16 tree_density;        /* per 1000 surface cells */
    cl_u16 plant_density;
    cl_u8  freeze;              /* surface water freezes */
    cl_u8  map_color;
} BiomeDef;

extern BiomeDef g_biomes[MAX_BIOMES];
extern int g_biome_count;

int  biome_load(const char *root, char *err, size_t errn);
int  biome_find(const char *id);
/* Returns biome index (never fails: falls back to the first land biome). */
int  biome_select(int kind_hint, int height, int temp, int humid, int special);
const char *biome_name(int b);

#endif
