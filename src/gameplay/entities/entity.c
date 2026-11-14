/*
 * Cliderness - gameplay/entities/entity.c
 * Item entity physics (every 4 ticks): fall through air; in a liquid, items
 * lighter than water (density < 1000) rise to the surface and float, heavier
 * items sink to the bottom. Identical items on the same cell merge.
 */
#include <stdio.h>
#include <string.h>
#include "gameplay/game.h"
#include "content/content.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/log.h"

int ent_spawn(Game *g, int dim, long x, long y, long z, int item, int count, int dur)
{
    int i;
    if (item < 0 || item >= g_item_count || count <= 0) {
        return -1;
    }
    for (i = 0; i < MAX_ITEM_ENTS; ++i) {
        ItemEntity *e = &g->ents[i];
        if (e->active && e->item == item && e->dim == dim && e->x == x && e->y == y && e->z == z &&
            g_items[item].max_stack > 1 && e->count + count <= g_items[item].max_stack) {
            e->count = (cl_s16)(e->count + count);
            e->age = 0;
            return i;
        }
    }
    for (i = 0; i < MAX_ITEM_ENTS; ++i) {
        ItemEntity *e = &g->ents[i];
        if (!e->active) {
            memset(e, 0, sizeof(*e));
            e->active = 1;
            e->dim = (cl_u8)dim;
            e->x = x;
            e->y = y;
            e->z = z;
            e->item = (cl_s16)item;
            e->count = (cl_s16)count;
            e->dur = (cl_s16)dur;
            return i;
        }
    }
    return -1;
}

static void ent_physics(Game *g, ItemEntity *e)
{
    World *w = &g->world;
    int dense = g_items[e->item].density >= 1000;
    cl_u8 here = world_get(w, e->dim, e->x, e->y, e->z);
    cl_u8 below = world_get(w, e->dim, e->x, e->y - 1, e->z);
    e->in_water = (cl_u8)(blk_liquid(here) || blk_liquid(below));
    if (blk_solid(here)) {
        /* buried by a placed block: pop up */
        if (e->y < WORLD_H - 1) {
            ++e->y;
        }
        return;
    }
    if (blk_liquid(here)) {
        if (dense) {
            if (!blk_solid(below) && e->y > 0) {
                --e->y;
            }
            e->floating = 0;
        } else {
            /* rise to the surface */
            if (e->y < WORLD_H - 1) {
                ++e->y;
            }
        }
        return;
    }
    if (blk_liquid(below)) {
        if (dense) {
            --e->y;
            e->floating = 0;
        } else {
            e->floating = 1;
        }
        return;
    }
    if (!blk_solid(below) && e->y > 0) {
        --e->y;
    }
    e->floating = 0;
}

void ent_tick(Game *g)
{
    int i;
    int phys = (g->tick % 4L) == 0;
    for (i = 0; i < MAX_ITEM_ENTS; ++i) {
        ItemEntity *e = &g->ents[i];
        if (!e->active) {
            continue;
        }
        ++e->age;
        if (e->age > ITEM_DESPAWN_TICKS) {
            e->active = 0;
            continue;
        }
        if (phys) {
            ent_physics(g, e);
        }
    }
}

int ent_pickup(Game *g, long x, long y, long z)
{
    int i, picked = 0;
    for (i = 0; i < MAX_ITEM_ENTS; ++i) {
        ItemEntity *e = &g->ents[i];
        int left;
        if (!e->active || e->dim != g->p.dim || e->x != x || e->z != z ||
            (e->y != y && e->y != y - 1)) {
            continue;
        }
        left = inv_add(&g->inv, e->item, e->count, e->dur);
        if (left < e->count) {
            char num[12];
            sprintf(num, "%d", e->count - left);
            log_add(ATTR(C_LIGHTGREEN, C_BLACK), loc_fmt("msg.picked_up", item_name(e->item), num, NULL));
            ++picked;
        }
        if (left == 0) {
            e->active = 0;
        } else {
            e->count = (cl_s16)left;
            log_add(ATTR(C_YELLOW, C_BLACK), loc_get("msg.inventory_full"));
        }
    }
    return picked;
}

int ent_count(const Game *g)
{
    int i, n = 0;
    for (i = 0; i < MAX_ITEM_ENTS; ++i) {
        if (g->ents[i].active) {
            ++n;
        }
    }
    return n;
}

/* ent_hostile_near / ent_interact live in mob.c (creatures). */

int ent_kill_all(Game *g)
{
    int i, n = 0;
    for (i = 0; i < MAX_ITEM_ENTS; ++i) {
        if (g->ents[i].active) {
            g->ents[i].active = 0;
            ++n;
        }
    }
    return n;
}
