/*
 * Cliderness - gameplay/dimension/portal.c : travel between the Overworld
 * and the Ancient Dimension.
 *
 * A portal is a block (ancient_portal, placed from a Portal Core). Pressing E
 * on it moves the player to the same x/z in the other dimension, onto the
 * nearest dry land within 48 cells, and builds a return portal next to the
 * arrival point unless one already stands within 8 cells.
 */
#include <string.h>
#include "gameplay/game.h"
#include "content/content.h"
#include "world/biomes/biome.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/log.h"
#include "core/mathx.h"
#include "core/str.h"

static int portal_block(void)
{
    return block_find("ancient_portal");
}

const char *dim_key(int dim)
{
    return dim == DIM_ANCIENT ? "dim.ancient" : "dim.overworld";
}

int dim_find(const char *name)
{
    if (str_ieq(name, "overworld") || str_ieq(name, "0") || str_ieq(name, loc_get("dim.overworld"))) {
        return DIM_OVERWORLD;
    }
    if (str_ieq(name, "ancient") || str_ieq(name, "1") || str_ieq(name, loc_get("dim.ancient"))) {
        return DIM_ANCIENT;
    }
    return -1;
}

/* a dry standing cell at column (x, z) of dim: returns y or -1 */
static long dry_spot(Game *g, int dim, long x, long z)
{
    long top = world_top(&g->world, dim, x, z, WORLD_H - 3);
    cl_u8 b = world_get(&g->world, dim, x, top, z);
    /* look through plants and other non-solid decoration */
    while (top > 4 && !blk_solid(b) && !blk_liquid(b)) {
        --top;
        b = world_get(&g->world, dim, x, top, z);
    }
    if (top < 4 || !blk_solid(b) || blk_liquid(b) || (g_blocks[b].flags & BF_HURT) ||
        (g_blocks[b].flags & BF_SLOW) || b == (cl_u8)block_find("void_stone")) {
        return -1;   /* water, lava, sharp blocks, leaves, rift floors */
    }
    return top + 1;
}

static int portal_near(Game *g, int dim, long x, long y, long z, int r)
{
    int pb = portal_block();
    long dx, dy, dz;
    for (dy = -3; dy <= 3; ++dy) {
        for (dz = -r; dz <= r; ++dz) {
            for (dx = -r; dx <= r; ++dx) {
                if (world_get(&g->world, dim, x + dx, y + dy, z + dz) == (cl_u8)pb) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int portal_travel(Game *g, int target, int build_return)
{
    Player *p = &g->p;
    long x = p->x, z = p->z, y = -1;
    long r, k;
    int pb = portal_block();
    if (target != DIM_OVERWORLD && target != DIM_ANCIENT) {
        return 0;
    }
    /* search rings around the same x/z for dry land */
    for (r = 0; r <= 48 && y < 0; r += 2) {
        for (k = -r; k <= r && y < 0; k += 2) {
            long cand[4][2];
            int c;
            cand[0][0] = p->x + k; cand[0][1] = p->z - r;
            cand[1][0] = p->x + k; cand[1][1] = p->z + r;
            cand[2][0] = p->x - r; cand[2][1] = p->z + k;
            cand[3][0] = p->x + r; cand[3][1] = p->z + k;
            for (c = 0; c < 4 && y < 0; ++c) {
                y = dry_spot(g, target, cand[c][0], cand[c][1]);
                if (y >= 0) {
                    x = cand[c][0];
                    z = cand[c][1];
                }
            }
        }
    }
    if (y < 0) {
        /* no land: make a small platform at sea level */
        y = SEA_LEVEL + 1;
        world_set(&g->world, target, x, y - 1, z, (cl_u8)(target == DIM_ANCIENT ? block_find("ancient_stone") : g_cb.stone), 0);
    }
    /* head room */
    world_set(&g->world, target, x, y, z, BLOCK_AIR, 0);
    world_set(&g->world, target, x, y + 1, z, BLOCK_AIR, 0);
    if (build_return && pb > 0 && !portal_near(g, target, x, y, z, 8)) {
        /* the return portal stands east of the arrival point on solid ground */
        if (!blk_solid(world_get(&g->world, target, x + 1, y - 1, z))) {
            world_set(&g->world, target, x + 1, y - 1, z, (cl_u8)(target == DIM_ANCIENT ? block_find("ancient_stone") : g_cb.stone), 0);
        }
        world_set(&g->world, target, x + 1, y, z, (cl_u8)pb, META_PLACED);
    }
    game_teleport(g, x, y, z);
    p->dim = target;
    p->facing = DIR_E;
    g->weather.minutes_left = 0;   /* the other sky brings its own weather */
    skill_gain(g, SK_EXPLORATION, target == DIM_ANCIENT ? 20 : 5);
    log_add(ATTR(C_LIGHTMAGENTA, C_BLACK), loc_fmt("msg.dim_arrive", loc_get(dim_key(target)), NULL, NULL));
    g->sfx = SFX_THUNDER;
    g->unsaved = 1;
    return 1;
}
