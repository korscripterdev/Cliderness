/*
 * Cliderness - gameplay/crafting/crafting.c
 * Crafting, smelting, cooking and repairs driven by data/recipes.
 *
 * A recipe needs: all ingredients in the inventory, its station block within
 * 3 cells (same level +-1), enough fuel units (furnace/campfire recipes) and
 * the minimum skill level. Creative mode crafts without consuming anything.
 * Experience: (sum of ingredient counts + 2) for the recipe's skill.
 */
#include <stdio.h>
#include <string.h>
#include "gameplay/game.h"
#include "content/content.h"
#include "content/recipes.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/log.h"

long game_carry_capacity(Game *g)
{
    return player_carry_capacity(&g->p) + inv_bag_capacity(&g->inv);
}

int craft_station_near(Game *g, int station)
{
    long dx, dy, dz;
    if (station == STATION_NONE) {
        return 1;
    }
    for (dy = -1; dy <= 1; ++dy) {
        for (dz = -3; dz <= 3; ++dz) {
            for (dx = -3; dx <= 3; ++dx) {
                cl_u8 b = world_get(&g->world, g->p.dim, g->p.x + dx, g->p.y + dy, g->p.z + dz);
                if (g_blocks[b].station == station) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int craft_fuel_units(const Inventory *inv)
{
    int i, units = 0;
    for (i = 0; i < INV_SLOTS; ++i) {
        if (inv->s[i].item >= 0 && g_items[inv->s[i].item].fuel > 0) {
            units += g_items[inv->s[i].item].fuel * inv->s[i].count;
        }
    }
    return units;
}

/* burn the cheapest fuel items first until `units` are covered */
static void burn_fuel(Inventory *inv, int units)
{
    while (units > 0) {
        int i, best = -1;
        for (i = 0; i < INV_SLOTS; ++i) {
            int it = inv->s[i].item;
            if (it >= 0 && g_items[it].fuel > 0 &&
                (best < 0 || g_items[it].fuel < g_items[inv->s[best].item].fuel)) {
                best = i;
            }
        }
        if (best < 0) {
            return;
        }
        units -= g_items[inv->s[best].item].fuel;
        inv_remove_slot(inv, best, 1);
    }
}

int craft_check(Game *g, int r)
{
    const RecipeDef *rc;
    int k;
    if (r < 0 || r >= g_recipe_count) {
        return CRAFT_NEED_ITEMS;
    }
    rc = &g_recipes[r];
    if (player_skill_level(&g->p, rc->skill) < rc->min_skill && g->mode != GM_CREATIVE) {
        return CRAFT_NEED_SKILL;
    }
    if (!craft_station_near(g, rc->station) && g->mode != GM_CREATIVE) {
        return CRAFT_NEED_STATION;
    }
    if (g->mode == GM_CREATIVE) {
        return CRAFT_OK;
    }
    for (k = 0; k < rc->nin; ++k) {
        if (inv_count(&g->inv, rc->in_item[k]) < rc->in_count[k]) {
            return CRAFT_NEED_ITEMS;
        }
    }
    if (rc->fuel > 0) {
        /* fuel items that are also ingredients are counted once */
        int units = craft_fuel_units(&g->inv);
        for (k = 0; k < rc->nin; ++k) {
            units -= g_items[rc->in_item[k]].fuel * rc->in_count[k];
        }
        if (units < rc->fuel) {
            return CRAFT_NEED_FUEL;
        }
    }
    return CRAFT_OK;
}

int craft_max(Game *g, int r)
{
    const RecipeDef *rc = &g_recipes[r];
    int k, n = 999;
    if (craft_check(g, r) != CRAFT_OK) {
        return 0;
    }
    if (g->mode == GM_CREATIVE) {
        return 64;
    }
    for (k = 0; k < rc->nin; ++k) {
        int m = inv_count(&g->inv, rc->in_item[k]) / rc->in_count[k];
        if (m < n) {
            n = m;
        }
    }
    if (rc->fuel > 0) {
        int m = craft_fuel_units(&g->inv) / (rc->fuel + 1);
        if (m < n) {
            n = m > 0 ? m : 1;
        }
    }
    return n;
}

int craft_do(Game *g, int r)
{
    const RecipeDef *rc;
    int k, res, left, xp = 2;
    res = craft_check(g, r);
    if (res != CRAFT_OK) {
        return res;
    }
    rc = &g_recipes[r];
    if (g->mode != GM_CREATIVE) {
        for (k = 0; k < rc->nin; ++k) {
            inv_remove(&g->inv, rc->in_item[k], rc->in_count[k]);
            xp += rc->in_count[k];
        }
        if (rc->fuel > 0) {
            burn_fuel(&g->inv, rc->fuel);
        }
    }
    left = inv_add(&g->inv, rc->result, rc->count, 0);
    if (left > 0) {
        ent_spawn(g, g->p.dim, g->p.x, g->p.y, g->p.z, rc->result, left, 0);
        log_add(ATTR(C_YELLOW, C_BLACK), loc_get("msg.inventory_full"));
    }
    {
        char num[8];
        sprintf(num, "%d", rc->count);
        log_add(ATTR(C_LIGHTGREEN, C_BLACK), loc_fmt("msg.crafted", item_name(rc->result), num, NULL));
    }
    if (g->mode != GM_CREATIVE) {
        skill_gain(g, rc->skill, xp);
    }
    g->unsaved = 1;
    return CRAFT_OK;
}

int repair_check(Game *g, int slot)
{
    ItemStack *st;
    const ItemDef *d;
    if (slot < 0 || slot >= INV_SLOTS) {
        return CRAFT_NEED_ITEMS;
    }
    st = &g->inv.s[slot];
    if (st->item < 0) {
        return CRAFT_NEED_ITEMS;
    }
    d = &g_items[st->item];
    if (d->durability <= 0 || d->repair_item < 0 || st->dur >= d->durability) {
        return CRAFT_NEED_ITEMS;
    }
    if (!craft_station_near(g, STATION_ANVIL) && !craft_station_near(g, STATION_WORKBENCH)) {
        return CRAFT_NEED_STATION;
    }
    if (inv_count(&g->inv, d->repair_item) < 1) {
        return CRAFT_NEED_ITEMS;
    }
    return CRAFT_OK;
}

int repair_do(Game *g, int slot)
{
    int res = repair_check(g, slot);
    ItemStack *st;
    const ItemDef *d;
    long add;
    if (res != CRAFT_OK) {
        return res;
    }
    st = &g->inv.s[slot];
    d = &g_items[st->item];
    inv_remove(&g->inv, d->repair_item, 1);
    add = (long)d->durability * (long)(d->repair_pct ? d->repair_pct : 25) / 100L;
    /* better crafting skill repairs more */
    add = add * (100L + (long)player_skill_level(&g->p, SK_CRAFTING) * 2L) / 100L;
    st->dur = (cl_s16)CL_MIN((long)d->durability, (long)st->dur + add);
    log_add(ATTR(C_LIGHTGREEN, C_BLACK), loc_fmt("msg.repaired", item_name(st->item), NULL, NULL));
    skill_gain(g, SK_CRAFTING, 3);
    return CRAFT_OK;
}
