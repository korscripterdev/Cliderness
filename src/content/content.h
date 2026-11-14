/*
 * Cliderness - content/content.h
 * Data-driven block and item registry, loaded from data/blocks/<file>.def and
 * data/items/<file>.def (file lists in index.lst). Nothing here is hard-coded
 * except the reserved block 0 = air.
 */
#ifndef CLIDE_CONTENT_H
#define CLIDE_CONTENT_H

#include "core/types.h"

#define MAX_BLOCKS 256
#define MAX_ITEMS  512
#define ID_LEN     28
#define NAME_LEN   28

/* block flags */
#define BF_SOLID       0x0001
#define BF_LIQUID      0x0002
#define BF_PLANT       0x0004   /* non-solid decoration drawn at eye level */
#define BF_SLOW        0x0008   /* slows movement */
#define BF_HURT        0x0010   /* damages on contact */
#define BF_REPLACEABLE 0x0020   /* placing a block overwrites it */
#define BF_NOMINE      0x0040   /* cannot be mined (bedrock) */
#define BF_LIGHT       0x0080   /* emits light */
#define BF_ANCIENT     0x0100   /* belongs to the Ancient Dimension */

enum { TOOL_NONE = 0, TOOL_PICKAXE, TOOL_AXE, TOOL_SHOVEL, TOOL_HOE, TOOL_SWORD, TOOL_COUNT };

enum {
    ITEM_BLOCK = 0, ITEM_TOOL, ITEM_WEAPON, ITEM_ARMOR, ITEM_FOOD, ITEM_RESOURCE,
    ITEM_MATERIAL, ITEM_CONSUMABLE, ITEM_BUILDING, ITEM_RELIC, ITEM_TYPE_COUNT
};

typedef struct BlockDef {
    char   id[ID_LEN];
    char   name[NAME_LEN];
    cl_u8  glyph, fg, bg;
    cl_u8  floor_glyph, floor_fg, floor_bg;
    cl_u16 flags;
    cl_u8  hardness;       /* base break time in ticks (bare hands x3) */
    cl_u8  tool;           /* preferred tool */
    cl_s16 drop_item;      /* -1 = nothing */
    cl_u8  drop_count;
    cl_u8  light;
    /* ore generation (0 chance = not an ore) */
    cl_u8  ore_ymin, ore_ymax;
    cl_u16 ore_chance;     /* per 10000 host blocks */
    cl_u8  ore_host;
    cl_u8  category;       /* see block_category_name */
} BlockDef;

typedef struct ItemDef {
    char   id[ID_LEN];
    char   name[NAME_LEN];
    cl_u8  glyph, fg;
    cl_u8  type;
    cl_u8  tool_type;
    cl_s16 tool_power;     /* mining speed percent (100 = hands) */
    cl_s16 max_stack;
    cl_s16 weight;         /* grams per unit */
    cl_s16 volume;         /* centilitres per unit */
    cl_s16 density;        /* kg/m3; < 1000 floats in water */
    cl_s16 place_block;    /* -1 = not placeable */
    cl_s16 durability;     /* 0 = no durability */
    cl_s16 food;           /* hunger restored (0..1000 scale) */
    cl_s16 water;          /* thirst restored */
} ItemDef;

/* root of the data directory, set at start-up ("data") */
extern char g_data_root[128];

extern BlockDef g_blocks[MAX_BLOCKS];
extern ItemDef  g_items[MAX_ITEMS];
extern int g_block_count;
extern int g_item_count;

/* Load all content from <root>/blocks and <root>/items. Returns 1 on success;
 * on failure err receives a human readable message. */
int  content_load(const char *root, char *err, size_t errn);
void content_reset(void);

/* Lookup by id, accepts "stone", "cliderness:stone", "Cliderness:stone" */
int  block_find(const char *id);
int  item_find(const char *id);
const char *block_name(int b);  /* localised */
const char *item_name(int i);   /* localised */
const char *item_type_name(int t);

#define BLOCK_AIR 0
#define blk_solid(b)  ((g_blocks[(b)].flags & BF_SOLID) != 0)
#define blk_liquid(b) ((g_blocks[(b)].flags & BF_LIQUID) != 0)

/* Commonly used block ids, resolved after loading (may be -1 if a data
 * file omits them; terrain generation checks). */
typedef struct CoreBlocks {
    int air, bedrock, stone, deep_rock, dirt, grass, sand, gravel, water, lava, ice, snow;
    int log, leaves, tall_grass, cobble;
} CoreBlocks;
extern CoreBlocks g_cb;

#endif
