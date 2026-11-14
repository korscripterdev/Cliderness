/*
 * Cliderness - world/structures/structure.h
 * Procedural structures (data/structures/structures.def).
 *
 * Structures are pure functions of the seed, like terrain: the world is split
 * into regions (128 blocks for underground, 192 for surface structures); a
 * region holds a structure with probability `rarity` %. terrain_block() asks
 * structure_block() for an override per cell, so chunk borders stay seamless.
 *
 *   mine   - abandoned mine: two crossing corridors plus two branches, wooden
 *            ceiling beams every 6 cells, torches, loot crates at the ends
 *   ruins  - broken stone ring on the surface with a crate in the middle
 *   city   - forgotten city (Ancient Dimension): walled grid of houses
 *
 * Loot tables ([loot <id>]) fill a crate the first time it is opened.
 */
#ifndef CLIDE_WORLD_STRUCTURE_H
#define CLIDE_WORLD_STRUCTURE_H

#include "core/types.h"

#define MAX_STRUCTS 16
#define MAX_LOOT    16
#define LOOT_ENTRIES 12

enum { ST_MINE = 0, ST_RUINS, ST_CITY, ST_TYPE_COUNT };

typedef struct StructDef {
    char  id[24];
    char  name[28];
    cl_u8 type;
    cl_u8 dim;
    cl_u8 rarity;        /* percent of regions */
    cl_u8 ymin, ymax;    /* mines: corridor height range */
    cl_u8 length;        /* half length of the main corridors */
    cl_u8 wall, support, light, crate, floor;
} StructDef;

typedef struct LootDef {
    char   id[16];
    int    n;
    cl_s16 item[LOOT_ENTRIES];
    cl_u8  lo[LOOT_ENTRIES], hi[LOOT_ENTRIES], chance[LOOT_ENTRIES];
} LootDef;

extern StructDef g_structs[MAX_STRUCTS];
extern int g_struct_count;
extern LootDef g_loot[MAX_LOOT];
extern int g_loot_count;

int  structures_load(const char *root, char *err, size_t errn);
int  struct_find(const char *id);
int  loot_find(const char *id);
const char *struct_name(int s);
/* block override for a generated cell, or -1 for "no structure here" */
int  structure_block(cl_u32 seed, int dim, int surface_h, long x, int y, long z);
/* nearest structure of index s around (x,z): returns 1 and its centre */
int  structure_locate(cl_u32 seed, int s, long x, long z, long *ox, long *oy, long *oz);

#endif
