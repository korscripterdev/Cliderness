/* Cliderness - world/terrain/terrain.c */
#include <string.h>
#include "world/terrain/terrain.h"
#include "world/biomes/biome.h"
#include "world/structures/structure.h"
#include "content/content.h"
#include "core/noise.h"
#include "core/rng.h"
#include "core/mathx.h"

#define MAX_ORES 32

static int g_ores[MAX_ORES];
static int g_nores = 0;

void terrain_init_ores(void)
{
    int i;
    g_nores = 0;
    for (i = 1; i < g_block_count && g_nores < MAX_ORES; ++i) {
        if (g_blocks[i].ore_chance > 0) {
            g_ores[g_nores++] = i;
        }
    }
}

static int spread(long v, long lo, long hi)
{
    long r = (v - lo) * 100L / (hi - lo);
    return (int)CL_CLAMP(r, 0L, 100L);
}

int terrain_height(cl_u32 seed, long x, long z)
{
    long cont = fbm2(seed, x, z, 8, 4);
    long det = fbm2(CL_U32(seed + 11UL), x, z, 5, 3);
    long mnt = noise2(CL_U32(seed + 23UL), x, z, 7);
    long h = 64L + (cont - 1900L) * 40L / 1000L + (det - 2048L) * 10L / 2048L;
    if (mnt > 2600L) {
        h += (mnt - 2600L) * 36L / 1495L;
    }
    /* rivers: a narrow band along the 2048 contour of a large noise field,
     * cut down to below sea level so that they fill with water and flow
     * into lakes and the sea */
    if (h >= SEA_LEVEL - 1 && h <= SEA_LEVEL + 14) {
        long r = noise2(CL_U32(seed + 404UL), x, z, 9);
        long d = mx_abs(r - 2048L);
        if (d < 34L) {
            long bed = SEA_LEVEL - 2 - (34L - d) / 12L;
            /* banks slope down towards the channel */
            if (h > bed) {
                h = bed;
            }
        } else if (d < 70L && h > SEA_LEVEL + 1) {
            long bank = SEA_LEVEL + 1 + (d - 34L) / 6L;
            if (h > bank) {
                h = bank;
            }
        }
    }
    return (int)CL_CLAMP(h, 6L, (long)(WORLD_H - 8));
}

void terrain_gen_column(cl_u32 seed, int dim, ColumnData *col)
{
    int lx, lz;
    for (lz = 0; lz < CHUNK_SIZE; ++lz) {
        for (lx = 0; lx < CHUNK_SIZE; ++lx) {
            long x = col->cx * CHUNK_SIZE + lx;
            long z = col->cz * CHUNK_SIZE + lz;
            int i = lz * CHUNK_SIZE + lx;
            cl_u32 s = CL_U32(seed + (cl_u32)dim * 7919UL);
            int h = terrain_height(s, x, z);
            int t = spread(fbm2(CL_U32(s + 101UL), x, z, 9, 3), 1150L, 2950L);
            int m = spread(fbm2(CL_U32(s + 202UL), x, z, 8, 3), 1150L, 2950L);
            int sp = spread(noise2(CL_U32(s + 303UL), x, z, 7), 600L, 3500L);
            int kind;
            if (h > 80) {
                t -= (h - 80) / 2;
                if (t < 0) {
                    t = 0;
                }
            }
            kind = h < SEA_LEVEL - 1 ? BK_OCEAN : (h <= SEA_LEVEL + 1 ? BK_COAST : BK_LAND);
            if (dim == DIM_ANCIENT) {
                kind = BK_ANCIENT;
            }
            col->height[i] = (cl_u8)h;
            col->temp[i] = (cl_u8)t;
            col->humid[i] = (cl_u8)m;
            col->biome[i] = (cl_u8)biome_select(kind, h, t, m, sp);
        }
    }
    col->valid = 1;
}

static int is_cave(cl_u32 seed, long x, int y, long z)
{
    long a = noise3(CL_U32(seed + 77UL), x, (long)y * 2L, z, 4);
    if (a > 2950L) {
        return 1;
    }
    /* thin winding tunnels */
    {
        long b = noise3(CL_U32(seed + 91UL), x, (long)y * 3L, z, 5);
        long c = noise3(CL_U32(seed + 97UL), x, (long)y * 3L, z, 5);
        if (mx_abs(b - 2048L) < 70L && mx_abs(c - 2048L) < 200L) {
            return 1;
        }
    }
    return 0;
}

cl_u8 terrain_block(cl_u32 seed, int dim, const ColumnData *col, int index, long x, int y, long z)
{
    int h = col->height[index];
    const BiomeDef *b = &g_biomes[col->biome[index]];
    cl_u32 s = CL_U32(seed + (cl_u32)dim * 7919UL);
    if (y <= 0) {
        return (cl_u8)g_cb.bedrock;
    }
    if (y > 2 && g_struct_count > 0) {
        int sb = structure_block(seed, dim, h, x, y, z);
        if (sb >= 0) {
            return (cl_u8)sb;
        }
    }
    if (y <= 2) {
        return (cl_u8)((rng_hash3(s, x, y, z) % 3UL) < (cl_u32)(3 - y) ? g_cb.bedrock : g_cb.deep_rock);
    }
    if (y < h - 3) {
        int base = y < 24 ? g_cb.deep_rock : b->stone;
        int i;
        if (y > 3 && y < h - 5 && is_cave(s, x, y, z)) {
            if (y < 10) {
                return (cl_u8)g_cb.lava;
            }
            /* underground lakes: cave space below a local water line */
            if (y >= 12 && y <= 30 && noise2(CL_U32(s + 555UL), x, z, 5) < 1500L &&
                y <= 16 + (int)(noise2(CL_U32(s + 556UL), x, z, 6) * 10L / 4096L)) {
                return (cl_u8)g_cb.water;
            }
            return BLOCK_AIR;
        }
        for (i = 0; i < g_nores; ++i) {
            const BlockDef *o = &g_blocks[g_ores[i]];
            if (o->ore_host == base && y >= o->ore_ymin && y <= o->ore_ymax &&
                (long)(rng_hash3(CL_U32(s + (cl_u32)g_ores[i] * 131UL), x, y, z) % 10000UL) <
                    (long)o->ore_chance) {
                return (cl_u8)g_ores[i];
            }
        }
        return (cl_u8)base;
    }
    if (y < h) {
        return (cl_u8)(h < SEA_LEVEL ? b->seabed : b->subsurface);
    }
    if (y == h) {
        return (cl_u8)(h < SEA_LEVEL ? b->seabed : b->surface);
    }
    /* above the surface */
    {
        cl_u32 r = rng_hash3(CL_U32(s ^ 0x7EE7UL), x, 0, z);
        long roll = (long)(r % 1000UL);
        int tree = (h >= SEA_LEVEL && b->tree && roll < (long)b->tree_density);
        if (y < SEA_LEVEL) {
            if (y == h + 1 && b->plant && roll < (long)b->plant_density) {
                return b->plant;
            }
            if (y == SEA_LEVEL - 1 && b->freeze) {
                return (cl_u8)g_cb.ice;
            }
            return (cl_u8)g_cb.water;
        }
        if (y == h + 1) {
            if (tree) {
                return (cl_u8)((b->tree2 && ((r >> 12) & 3UL) == 0UL) ? b->tree2 : b->tree);
            }
            if (h >= SEA_LEVEL && roll < (long)(b->tree_density + b->plant_density)) {
                cl_u32 pick = (r >> 16) % 6UL;
                if (pick >= 4UL && b->plant3) {
                    return b->plant3;
                }
                if (pick >= 2UL && b->plant2) {
                    return b->plant2;
                }
                return b->plant;
            }
        }
        if (y == h + 2 && tree && b->tree_top) {
            return b->tree_top;
        }
    }
    return BLOCK_AIR;
}

void terrain_gen_chunk(cl_u32 seed, const ColumnData *col, Chunk *ch)
{
    int lx, lz;
    for (lz = 0; lz < CHUNK_SIZE; ++lz) {
        for (lx = 0; lx < CHUNK_SIZE; ++lx) {
            int i = lz * CHUNK_SIZE + lx;
            ch->blocks[i] = terrain_block(seed, ch->dim, col, i,
                                          ch->cx * CHUNK_SIZE + lx, ch->y,
                                          ch->cz * CHUNK_SIZE + lz);
            ch->meta[i] = blk_liquid(ch->blocks[i]) ? 15 : 0;
        }
    }
}
