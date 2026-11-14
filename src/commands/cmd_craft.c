/* Cliderness - commands/cmd_craft.c : crafting, farming, storage, equipment, effects */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "content/recipes.h"
#include "content/effects.h"
#include "localization/locale.h"
#include "core/str.h"

static int c_craft(ConCtx *c)
{
    int r, i, made = 0, res = CRAFT_OK;
    long n = 1;
    char buf[64];
    if (c->argc < 2) {
        return con_usage(c);
    }
    r = recipe_find(c->argv[1]);
    if (r < 0) {
        /* allow the result item id as well */
        int it = item_find(c->argv[1]);
        for (i = 0; it >= 0 && i < g_recipe_count; ++i) {
            if (g_recipes[i].result == it) {
                r = i;
                break;
            }
        }
    }
    if (r < 0) {
        con_err(loc_fmt("con.unknown_recipe", c->argv[1], NULL, NULL));
        return CMD_ERR;
    }
    if (c->argc >= 3 && (!con_arg_long(c, 2, &n) || n < 1 || n > 999)) {
        return con_usage(c);
    }
    for (i = 0; i < n; ++i) {
        res = craft_do(c->g, r);
        if (res != CRAFT_OK) {
            break;
        }
        ++made;
    }
    if (made == 0) {
        static const char *const keys[] = {
            "craft.ready", "craft.need_items", "craft.need_station", "craft.need_fuel", "craft.need_skill", "craft.ready"
        };
        con_err(loc_get(keys[res]));
        return CMD_ERR;
    }
    sprintf(buf, "%d", made);
    con_good(loc_fmt("con.count", buf, NULL, NULL));
    return CMD_OK;
}

static int c_recipes(ConCtx *c)
{
    char line[160];
    int i, total = 0, only_ok = 0;
    const char *filter = "";
    if (c->argc >= 2) {
        if (str_ieq(c->argv[1], "available")) only_ok = 1;
        else filter = c->argv[1];
    }
    line[0] = '\0';
    for (i = 0; i < g_recipe_count; ++i) {
        const RecipeDef *r = &g_recipes[i];
        if (only_ok && craft_check(c->g, i) != CRAFT_OK) continue;
        if (*filter && !str_icontains(r->id, filter)) continue;
        ++total;
        if (strlen(line) + strlen(r->id) + 2 > 76) {
            con_out(line);
            line[0] = '\0';
        }
        str_cat(line, r->id, sizeof(line));
        str_cat(line, " ", sizeof(line));
    }
    if (line[0]) con_out(line);
    sprintf(line, "%d", total);
    con_out(loc_fmt("con.count", line, NULL, NULL));
    return CMD_OK;
}

static int c_recipe(ConCtx *c)
{
    int r, k;
    char buf[160], nm[40];
    const RecipeDef *rc;
    if (c->argc < 2 || (r = recipe_find(c->argv[1])) < 0) {
        if (c->argc >= 2) con_err(loc_fmt("con.unknown_recipe", c->argv[1], NULL, NULL));
        return c->argc < 2 ? con_usage(c) : CMD_ERR;
    }
    rc = &g_recipes[r];
    str_copy(nm, item_name(rc->result), sizeof(nm));
    sprintf(buf, "%s x%d  [%s] %s", nm, rc->count, loc_get(recipe_category_key(rc->category)),
            loc_get(station_key(rc->station)));
    con_good(buf);
    for (k = 0; k < rc->nin; ++k) {
        str_copy(nm, item_name(rc->in_item[k]), sizeof(nm));
        sprintf(buf, "  %d x %s (%d)", rc->in_count[k], nm, inv_count(&c->g->inv, rc->in_item[k]));
        con_out(buf);
    }
    if (rc->fuel) {
        sprintf(buf, "  %s %d", loc_get("craft.fuel"), rc->fuel);
        con_out(buf);
    }
    return CMD_OK;
}

static int c_effect(ConCtx *c)
{
    int e, i;
    long mins = 0;
    char buf[96];
    if (c->argc < 2 || str_ieq(c->argv[1], "list")) {
        int any = 0;
        for (i = 0; i < PLAYER_FX; ++i) {
            if (c->g->p.fx[i].id >= 0 && c->g->p.fx[i].minutes > 0) {
                sprintf(buf, "%s: %d min", g_effects[c->g->p.fx[i].id].id, c->g->p.fx[i].minutes);
                con_out(buf);
                any = 1;
            }
        }
        if (!any) con_out(loc_get("con.no_effects"));
        return CMD_OK;
    }
    if (!c->g->cheats) {
        con_err(loc_get("con.no_cheats"));
        return CMD_DENIED;
    }
    c->g->cheated = 1;
    if (str_ieq(c->argv[1], "clear")) {
        fx_clear(c->g);
        con_good(loc_get("con.done"));
        return CMD_OK;
    }
    if (!str_ieq(c->argv[1], "give") || c->argc < 3) {
        return con_usage(c);
    }
    e = effect_find(c->argv[2]);
    if (e < 0) {
        con_err(loc_fmt("con.unknown_effect", c->argv[2], NULL, NULL));
        return CMD_ERR;
    }
    if (c->argc >= 4 && (!con_arg_long(c, 3, &mins) || mins < 1 || mins > 30000)) {
        return con_usage(c);
    }
    fx_add(c->g, e, (int)mins);
    return CMD_OK;
}

static int c_cure(ConCtx *c)
{
    int i, n = 0;
    char buf[16];
    for (i = 0; i < PLAYER_FX; ++i) {
        int e = c->g->p.fx[i].id;
        if (e >= 0 && g_effects[e].harmful) {
            n += fx_remove(c->g, e);
        }
    }
    sprintf(buf, "%d", n);
    con_good(loc_fmt("con.cured_n", buf, NULL, NULL));
    return CMD_OK;
}

static int c_sleep(ConCtx *c)
{
    return game_sleep(c->g, 0) ? CMD_OK : CMD_ERR;
}

static int c_skipnight(ConCtx *c)
{
    return game_sleep(c->g, 1) ? CMD_OK : CMD_ERR;
}

static int c_grow(ConCtx *c)
{
    long r = 16;
    char buf[16];
    if (c->argc >= 2 && (!con_arg_long(c, 1, &r) || r < 1 || r > 256)) {
        return con_usage(c);
    }
    sprintf(buf, "%d", farm_grow_all(c->g, r));
    con_good(loc_fmt("con.grown", buf, NULL, NULL));
    return CMD_OK;
}

static int c_crops(ConCtx *c)
{
    char buf[16];
    sprintf(buf, "%d", farm_count(c->g));
    con_out(loc_fmt("con.count", buf, NULL, NULL));
    return CMD_OK;
}

static int eq_slot(const char *s)
{
    static const char *const names[INV_EQUIP] = { "head", "body", "legs", "feet", "back" };
    int i;
    for (i = 0; i < INV_EQUIP; ++i) {
        if (str_ieq(s, names[i])) return i;
    }
    return -1;
}

static int c_equip(ConCtx *c)
{
    int it, i;
    if (c->argc < 2) {
        return con_usage(c);
    }
    it = item_find(c->argv[1]);
    for (i = 0; it >= 0 && i < INV_SLOTS; ++i) {
        if (c->g->inv.s[i].item == it && inv_equip(&c->g->inv, i)) {
            con_good(loc_fmt("msg.equipped", item_name(it), NULL, NULL));
            return CMD_OK;
        }
    }
    con_err(loc_get("con.cannot_equip"));
    return CMD_ERR;
}

static int c_unequip(ConCtx *c)
{
    int k;
    if (c->argc < 2) {
        return con_usage(c);
    }
    if (str_ieq(c->argv[1], "all")) {
        for (k = 0; k < INV_EQUIP; ++k) inv_unequip(&c->g->inv, k);
        con_good(loc_get("con.done"));
        return CMD_OK;
    }
    k = eq_slot(c->argv[1]);
    if (k < 0) {
        return con_usage(c);
    }
    if (!inv_unequip(&c->g->inv, k)) {
        con_err(loc_get("con.cannot_equip"));
        return CMD_ERR;
    }
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_equipment(ConCtx *c)
{
    static const char *const eqk[INV_EQUIP] = { "eq.head", "eq.body", "eq.legs", "eq.feet", "eq.back" };
    int k;
    char buf[96];
    for (k = 0; k < INV_EQUIP; ++k) {
        const ItemStack *e = &c->g->inv.equip[k];
        char nm[40];
        str_copy(nm, e->item >= 0 ? item_name(e->item) : "-", sizeof(nm));
        sprintf(buf, "%-10s %s", loc_get(eqk[k]), nm);
        con_out(buf);
    }
    sprintf(buf, "%s %d%%  %s %+d C  +%ld kg", loc_get("inv.defense"), inv_defense(&c->g->inv),
            loc_get("inv.insulation"), inv_insulation(&c->g->inv), inv_bag_capacity(&c->g->inv) / 1000L);
    con_out(buf);
    return CMD_OK;
}

static int c_chests(ConCtx *c)
{
    int i, shown = 0;
    char buf[96];
    for (i = 0; i < MAX_CHESTS && shown < 10; ++i) {
        const Chest *ch = &c->g->chests[i];
        int k, n = 0;
        if (!ch->active) continue;
        for (k = 0; k < CHEST_SLOTS; ++k) n += ch->s[k].item >= 0;
        sprintf(buf, "#%d @ %ld %ld %ld: %d/%d", i, ch->x, ch->y, ch->z, n, CHEST_SLOTS);
        con_out(buf);
        ++shown;
    }
    sprintf(buf, "%d", chest_count(c->g));
    con_out(loc_fmt("con.count", buf, NULL, NULL));
    return CMD_OK;
}

static int c_fuel(ConCtx *c)
{
    char buf[16];
    sprintf(buf, "%d", craft_fuel_units(&c->g->inv));
    con_out(loc_fmt("con.fuel_units", buf, NULL, NULL));
    return CMD_OK;
}

static int c_stations(ConCtx *c)
{
    int s;
    char buf[96];
    buf[0] = '\0';
    for (s = 1; s < STATION_COUNT; ++s) {
        if (craft_station_near(c->g, s)) {
            str_cat(buf, loc_get(station_key(s)), sizeof(buf));
            str_cat(buf, " ", sizeof(buf));
        }
    }
    con_out(buf[0] ? buf : "-");
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "craft", "item", "<recipe> [count]", "xn", NULL, PERM_ANY, c_craft, "make" },
    { "recipes", "item", "[available|filter]", "x", "available", PERM_ANY, c_recipes, NULL },
    { "recipe", "item", "<recipe>", "", NULL, PERM_ANY, c_recipe, NULL },
    { "stations", "item", "", "", NULL, PERM_ANY, c_stations, NULL },
    { "fuel", "item", "", "", NULL, PERM_ANY, c_fuel, NULL },
    { "effect", "player", "<list|give|clear> [effect] [minutes]", "x", "list|give|clear", PERM_ANY, c_effect, NULL },
    { "cure", "player", "", "", NULL, PERM_CHEAT, c_cure, NULL },
    { "sleep", "player", "", "", NULL, PERM_ANY, c_sleep, NULL },
    { "skipnight", "time", "", "", NULL, PERM_CHEAT, c_skipnight, NULL },
    { "grow", "world", "[radius]", "n", NULL, PERM_CHEAT, c_grow, NULL },
    { "crops", "world", "", "", NULL, PERM_ANY, c_crops, NULL },
    { "equip", "equipment", "<item>", "i", NULL, PERM_ANY, c_equip, "wear" },
    { "unequip", "equipment", "<head|body|legs|feet|back|all>", "x", "head|body|legs|feet|back|all", PERM_ANY, c_unequip, NULL },
    { "equipment", "equipment", "", "", NULL, PERM_ANY, c_equipment, "gear" },
    { "chests", "item", "", "", NULL, PERM_ANY, c_chests, NULL }
};

CommandTable cmd_table_craft(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
