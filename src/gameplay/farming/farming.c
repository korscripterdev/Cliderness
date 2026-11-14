/*
 * Cliderness - gameplay/farming/farming.c
 * Tilling, planting and growth of crops and saplings.
 *
 * Every planted block whose definition has grow_to is registered as a Crop.
 * Once per game minute due crops advance one stage. Stage time:
 *   grow_minutes x 2 in winter, x 3/4 while it rains, no growth while the
 *   block above is solid (no light). Saplings become a trunk (grow_to) with a
 *   canopy block (grow_top) above.
 */
#include <string.h>
#include "gameplay/game.h"
#include "content/content.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/log.h"
#include "core/str.h"

static long stage_minutes(Game *g, int block)
{
    ClideDate d;
    long m = g_blocks[block].grow_minutes;
    const WeatherDef *w = weather_cur(&g->weather);
    if (m <= 0) {
        m = 60;
    }
    ctime_date(&g->time, &d);
    if (d.season == SEASON_WINTER) {
        m *= 2;
    }
    if (w->precip == PRECIP_RAIN) {
        m = m * 3 / 4;
    }
    return m;
}

int farm_register(Game *g, int dim, long x, long y, long z)
{
    int i, free_slot = -1;
    cl_u8 b = world_get(&g->world, dim, x, y, z);
    if (!g_blocks[b].grow_to) {
        return 0;
    }
    for (i = 0; i < MAX_CROPS; ++i) {
        Crop *c = &g->crops[i];
        if (c->active && c->x == x && c->y == y && c->z == z && c->dim == dim) {
            return 1;
        }
        if (!c->active && free_slot < 0) {
            free_slot = i;
        }
    }
    if (free_slot < 0) {
        return 0;
    }
    {
        Crop *c = &g->crops[free_slot];
        c->x = x;
        c->y = y;
        c->z = z;
        c->dim = (cl_u8)dim;
        c->active = 1;
        c->next_minute = ctime_total_minutes(&g->time) + stage_minutes(g, b);
    }
    return 1;
}

static int grow_one(Game *g, Crop *c)
{
    cl_u8 b = world_get(&g->world, c->dim, c->x, c->y, c->z);
    const BlockDef *bd = &g_blocks[b];
    cl_u8 next;
    if (!bd->grow_to) {
        c->active = 0;           /* harvested or replaced */
        return 0;
    }
    if (blk_solid(world_get(&g->world, c->dim, c->x, c->y + 1, c->z)) && !bd->grow_top) {
        c->next_minute += 30;    /* no light: try again later */
        return 0;
    }
    next = bd->grow_to;
    if (bd->grow_top) {
        if (world_get(&g->world, c->dim, c->x, c->y + 1, c->z) != BLOCK_AIR) {
            c->next_minute += 60;
            return 0;
        }
        world_set(&g->world, c->dim, c->x, c->y + 1, c->z, bd->grow_top, 0);
    }
    world_set(&g->world, c->dim, c->x, c->y, c->z, next, 0);
    if (g_blocks[next].grow_to) {
        c->next_minute = ctime_total_minutes(&g->time) + stage_minutes(g, next);
    } else {
        c->active = 0;
    }
    g->unsaved = 1;
    return 1;
}

void farm_minute(Game *g)
{
    long now = ctime_total_minutes(&g->time);
    int i;
    for (i = 0; i < MAX_CROPS; ++i) {
        Crop *c = &g->crops[i];
        if (c->active && now >= c->next_minute) {
            grow_one(g, c);
        }
    }
}

int farm_grow_all(Game *g, long radius)
{
    int i, n = 0, guard;
    for (i = 0; i < MAX_CROPS; ++i) {
        Crop *c = &g->crops[i];
        long dx = c->x - g->p.x, dz = c->z - g->p.z;
        if (!c->active || dx * dx + dz * dz > radius * radius) {
            continue;
        }
        for (guard = 0; guard < 8 && c->active; ++guard) {
            if (grow_one(g, c)) {
                ++n;
            } else {
                break;
            }
        }
    }
    return n;
}

int farm_count(const Game *g)
{
    int i, n = 0;
    for (i = 0; i < MAX_CROPS; ++i) {
        n += g->crops[i].active;
    }
    return n;
}

int farm_till(Game *g)
{
    long dx, dz, x, y, z;
    cl_u8 b;
    ItemStack *st = inv_selected(&g->inv);
    int farmland = block_find("farmland");
    if (farmland < 0 || st->item < 0 || g_items[st->item].tool_type != TOOL_HOE) {
        return 0;
    }
    if (!game_can_edit_world(g)) {
        log_add(ATTR(C_YELLOW, C_BLACK), loc_get("msg.cannot_edit"));
        return 1;
    }
    dir_delta(g->p.facing, &dx, &dz);
    x = g->p.x + dx;
    z = g->p.z + dz;
    y = g->p.y - 1;
    if (world_get(&g->world, g->p.dim, x, g->p.y, z) != BLOCK_AIR &&
        !(g_blocks[world_get(&g->world, g->p.dim, x, g->p.y, z)].flags & BF_REPLACEABLE)) {
        log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get("msg.cannot_till"));
        return 1;
    }
    b = world_get(&g->world, g->p.dim, x, y, z);
    if (b != g_cb.grass && b != g_cb.dirt && !str_ieq(g_blocks[b].id, "podzol") &&
        !str_ieq(g_blocks[b].id, "snowy_grass") && !str_ieq(g_blocks[b].id, "mud")) {
        log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get("msg.cannot_till"));
        return 1;
    }
    /* clear grass growing on top */
    world_set(&g->world, g->p.dim, x, g->p.y, z, BLOCK_AIR, 0);
    world_set(&g->world, g->p.dim, x, y, z, (cl_u8)farmland, META_PLACED);
    if (g->mode != GM_CREATIVE && g_items[st->item].durability > 0) {
        st->dur = (cl_s16)(st->dur - 1);
        if (st->dur <= 0) {
            log_add(ATTR(C_YELLOW, C_BLACK), loc_fmt("msg.tool_broke", item_name(st->item), NULL, NULL));
            inv_remove_slot(&g->inv, g->inv.selected, 1);
        }
    }
    log_add(ATTR(C_LIGHTGREEN, C_BLACK), loc_get("msg.tilled"));
    g->unsaved = 1;
    return 1;
}
