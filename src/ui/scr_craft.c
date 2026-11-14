/*
 * Cliderness - ui/scr_craft.c : crafting / smelting / cooking and chest screens.
 * Neither screen pauses the game (only help and the pause menu do).
 */
#include <stdio.h>
#include <string.h>
#include "ui/app.h"
#include "ui/widgets.h"
#include "gameplay/game.h"
#include "content/content.h"
#include "content/recipes.h"
#include "localization/locale.h"
#include "renderer/cp437.h"
#include "core/str.h"

/* ------------------------------------------------------------ crafting */

static int g_cr_list[MAX_RECIPES];
static int g_cr_n = 0;
static int g_cr_sel = 0;
static int g_cr_top = 0;
static int g_cr_only_ok = 0;
static int g_cr_cat = -1;
static TextInput g_cr_query;
static int g_cr_search = 0;

static void craft_rebuild(void)
{
    int i;
    g_cr_n = 0;
    for (i = 0; i < g_recipe_count; ++i) {
        const RecipeDef *r = &g_recipes[i];
        if (g_cr_cat >= 0 && r->category != g_cr_cat) continue;
        if (g_cr_only_ok && craft_check(&g_game, i) != CRAFT_OK) continue;
        if (g_cr_query.buf[0] && !str_icontains(item_name(r->result), g_cr_query.buf) &&
            !str_icontains(r->id, g_cr_query.buf)) continue;
        g_cr_list[g_cr_n++] = i;
    }
    if (g_cr_sel >= g_cr_n) {
        g_cr_sel = g_cr_n ? g_cr_n - 1 : 0;
    }
}

static void craft_enter(void)
{
    g_cr_sel = 0;
    g_cr_top = 0;
    g_cr_search = 0;
    ti_set(&g_cr_query, "", 20, 0);
    craft_rebuild();
}

static const char *reason_key(int r)
{
    switch (r) {
    case CRAFT_NEED_ITEMS: return "craft.need_items";
    case CRAFT_NEED_STATION: return "craft.need_station";
    case CRAFT_NEED_FUEL: return "craft.need_fuel";
    case CRAFT_NEED_SKILL: return "craft.need_skill";
    default: return "craft.ready";
    }
}

static void craft_draw(Framebuffer *fb)
{
    Game *g = &g_game;
    int i, rows = 19;
    char buf[96];
    fb_clear(fb, UI_TEXT);
    ui_frame(fb, 0, 0, FB_W, FB_H - 1, loc_get("craft.title"), ATTR(C_LIGHTGRAY, C_BLACK));
    sprintf(buf, "%s: %s", loc_get(g_cr_only_ok ? "craft.tab_can" : "craft.tab_all"),
            g_cr_cat >= 0 ? loc_get(recipe_category_key(g_cr_cat)) : "*");
    fb_text(fb, 2, 1, buf, UI_KEY, 34);
    if (g_cr_sel < g_cr_top) g_cr_top = g_cr_sel;
    if (g_cr_sel >= g_cr_top + rows) g_cr_top = g_cr_sel - rows + 1;
    for (i = 0; i < rows && g_cr_top + i < g_cr_n; ++i) {
        int r = g_cr_list[g_cr_top + i];
        const RecipeDef *rc = &g_recipes[r];
        int ok = craft_check(g, r) == CRAFT_OK;
        cl_u8 a = (g_cr_top + i == g_cr_sel) ? UI_SEL : (ok ? UI_HI : UI_DIM);
        fb_fill(fb, 1, 2 + i, 34, 1, ' ', a);
        fb_put(fb, 2, 2 + i, g_items[rc->result].glyph, (g_cr_top + i == g_cr_sel) ? a : ATTR(g_items[rc->result].fg, C_BLACK));
        fb_text(fb, 4, 2 + i, item_name(rc->result), a, 26);
        if (rc->count > 1) {
            sprintf(buf, "x%d", rc->count);
            fb_text(fb, 31, 2 + i, buf, a, 4);
        }
    }
    if (g_cr_n == 0) {
        fb_text(fb, 3, 3, loc_get("help.no_results"), UI_DIM, 30);
    }
    fb_vline(fb, 36, 1, FB_H - 3, G_VLINE, UI_DIM);
    if (g_cr_n > 0) {
        int r = g_cr_list[g_cr_sel];
        const RecipeDef *rc = &g_recipes[r];
        int y = 2, k, res = craft_check(g, r);
        fb_text(fb, 38, y++, item_name(rc->result), UI_TITLE, 40);
        sprintf(buf, "[%s]", loc_get(recipe_category_key(rc->category)));
        fb_text(fb, 38, y++, buf, UI_DIM, 40);
        ++y;
        fb_text(fb, 38, y++, loc_get("craft.ingredients"), UI_KEY, 40);
        for (k = 0; k < rc->nin; ++k) {
            int have = inv_count(&g->inv, rc->in_item[k]);
            char nm[40];
            str_copy(nm, item_name(rc->in_item[k]), sizeof(nm));
            sprintf(buf, "%3d/%-3d %s", have, rc->in_count[k], nm);
            fb_text(fb, 39, y++, buf, have >= rc->in_count[k] ? UI_GOOD : UI_BAD, 40);
        }
        if (rc->fuel) {
            sprintf(buf, "%s %d/%d", loc_get("craft.fuel"), craft_fuel_units(&g->inv), rc->fuel);
            fb_text(fb, 39, y++, buf, craft_fuel_units(&g->inv) >= rc->fuel ? UI_GOOD : UI_BAD, 40);
        }
        ++y;
        sprintf(buf, "%s %s", loc_get("craft.station"), loc_get(station_key(rc->station)));
        fb_text(fb, 38, y++, buf, craft_station_near(g, rc->station) ? UI_GOOD : UI_BAD, 40);
        sprintf(buf, "%s %s %d/%d", loc_get("craft.skill"), loc_get(player_skill_key(rc->skill)),
                player_skill_level(&g->p, rc->skill), rc->min_skill);
        fb_text(fb, 38, y++, buf, player_skill_level(&g->p, rc->skill) >= rc->min_skill ? UI_TEXT : UI_BAD, 40);
        ++y;
        fb_text(fb, 38, y++, loc_get(reason_key(res)), res == CRAFT_OK ? UI_GOOD : UI_BAD, 40);
        if (res == CRAFT_OK) {
            sprintf(buf, "%s %d", loc_get("craft.max"), craft_max(g, r));
            fb_text(fb, 38, y++, buf, UI_TEXT, 40);
        }
    }
    {
        ItemStack *st = inv_selected(&g->inv);
        if (st->item >= 0 && g_items[st->item].durability > 0 && g_items[st->item].repair_item >= 0) {
            sprintf(buf, "R: %s (%d/%d) + %s", loc_get("craft.repair"), st->dur, g_items[st->item].durability,
                    item_name(g_items[st->item].repair_item));
            fb_text(fb, 38, 21, buf, repair_check(g, g->inv.selected) == CRAFT_OK ? UI_KEY : UI_DIM, 41);
        }
    }
    if (g_cr_search || g_cr_query.buf[0]) {
        sprintf(buf, "/%s%s", g_cr_query.buf, g_cr_search ? "_" : "");
        fb_text(fb, 2, 22, buf, UI_KEY, 34);
    }
    ui_hint(fb, loc_get("hint.craft"));
}

static void craft_key(const ClideKeyEvent *ev)
{
    Game *g = &g_game;
    if (g_cr_search) {
        if (ev->key == CK_ENTER || ev->key == CK_ESC) {
            g_cr_search = 0;
            if (ev->key == CK_ESC) ti_set(&g_cr_query, "", 20, 0);
        } else {
            ti_key(&g_cr_query, ev);
        }
        craft_rebuild();
        return;
    }
    if (ev->key == CK_ESC || ev->key == 'k' || ev->key == 'K') {
        app_pop();
        return;
    }
    if (ev->key == CK_UP && g_cr_sel > 0) --g_cr_sel;
    else if (ev->key == CK_DOWN && g_cr_sel + 1 < g_cr_n) ++g_cr_sel;
    else if (ev->key == CK_PGUP) g_cr_sel = g_cr_sel > 10 ? g_cr_sel - 10 : 0;
    else if (ev->key == CK_PGDN) g_cr_sel = CL_MIN(g_cr_n - 1, g_cr_sel + 10);
    else if (ev->key == CK_TAB) { g_cr_only_ok = !g_cr_only_ok; craft_rebuild(); }
    else if (ev->key == 'c' || ev->key == 'C') { g_cr_cat = g_cr_cat + 1 >= RCAT_COUNT ? -1 : g_cr_cat + 1; craft_rebuild(); }
    else if (ev->key == '/') g_cr_search = 1;
    else if ((ev->key == CK_ENTER || ev->key == CK_SPACE) && g_cr_n > 0) {
        craft_do(g, g_cr_list[g_cr_sel]);
        if (g_cr_only_ok) craft_rebuild();
    } else if ((ev->key == 'a' || ev->key == 'A') && g_cr_n > 0) {
        int n = craft_max(g, g_cr_list[g_cr_sel]);
        while (n-- > 0 && craft_do(g, g_cr_list[g_cr_sel]) == CRAFT_OK) {
        }
        if (g_cr_only_ok) craft_rebuild();
    } else if (ev->key == 'r' || ev->key == 'R') {
        repair_do(g, g->inv.selected);
    } else if (ev->key == CK_F1 || ev->key == '?') {
        app_open_help("crafting");
    }
}

const Screen scr_craft = { "craft", 0, 1, craft_enter, craft_key, craft_draw };

/* ------------------------------------------------------------ chest */

static int g_ch_idx = -1;
static int g_ch_side = 0;      /* 0 chest, 1 inventory */
static int g_ch_sel[2] = { 0, 0 };

void app_open_chest(int idx)
{
    g_ch_idx = idx;
    g_ch_side = 0;
    g_ch_sel[0] = g_ch_sel[1] = 0;
    app_push(&scr_chest);
}

static ItemStack *side_stack(int side, int i)
{
    return side == 0 ? &g_game.chests[g_ch_idx].s[i] : &g_game.inv.s[i];
}

static int side_count(int side)
{
    return side == 0 ? CHEST_SLOTS : INV_SLOTS;
}

/* move a whole stack to the other side, merging where possible */
static void transfer(int side, int i)
{
    ItemStack *src = side_stack(side, i);
    int other = 1 - side, k;
    if (src->item < 0) {
        return;
    }
    for (k = 0; k < side_count(other) && src->count > 0; ++k) {
        ItemStack *d = side_stack(other, k);
        if (d->item == src->item && g_items[src->item].max_stack > 1 && d->count < g_items[src->item].max_stack) {
            int room = g_items[src->item].max_stack - d->count;
            int n = src->count < room ? src->count : room;
            d->count = (cl_s16)(d->count + n);
            src->count = (cl_s16)(src->count - n);
        }
    }
    for (k = 0; k < side_count(other) && src->count > 0; ++k) {
        ItemStack *d = side_stack(other, k);
        if (d->item < 0) {
            *d = *src;
            src->count = 0;
        }
    }
    if (src->count <= 0) {
        src->item = -1;
        src->count = 0;
        src->dur = 0;
    }
    g_game.unsaved = 1;
}

static void chest_draw(Framebuffer *fb)
{
    int side, i;
    char buf[64];
    fb_clear(fb, UI_TEXT);
    ui_frame(fb, 0, 0, FB_W, FB_H - 1, loc_get("chest.title"), ATTR(C_LIGHTGRAY, C_BLACK));
    if (g_ch_idx < 0 || !g_game.chests[g_ch_idx].active) {
        return;
    }
    for (side = 0; side < 2; ++side) {
        int x0 = side == 0 ? 2 : 41;
        int n = side_count(side);
        fb_text(fb, x0, 1, loc_get(side == 0 ? "chest.title" : "inv.title"), side == g_ch_side ? UI_TITLE : UI_DIM, 36);
        for (i = 0; i < n && i < 36; ++i) {
            ItemStack *s = side_stack(side, i);
            int col = i / 18, row = i % 18;
            int x = x0 + col * 19, y = 3 + row;
            cl_u8 a = (side == g_ch_side && i == g_ch_sel[side]) ? UI_SEL : UI_TEXT;
            fb_fill(fb, x, y, 18, 1, ' ', a);
            if (s->item >= 0) {
                fb_put(fb, x, y, g_items[s->item].glyph, a == UI_SEL ? a : ATTR(g_items[s->item].fg, C_BLACK));
                fb_text(fb, x + 2, y, item_name(s->item), a, 12);
                sprintf(buf, "%2d", s->count);
                fb_text(fb, x + 15, y, buf, a, 3);
            } else {
                fb_text(fb, x + 2, y, "-", a == UI_SEL ? a : UI_DIM, 2);
            }
        }
    }
    fb_vline(fb, 39, 1, FB_H - 3, G_VLINE, UI_DIM);
    ui_hint(fb, loc_get("hint.chest"));
}

static void chest_key(const ClideKeyEvent *ev)
{
    int n = side_count(g_ch_side);
    int *sel = &g_ch_sel[g_ch_side];
    if (ev->key == CK_ESC || ev->key == 'e' || ev->key == 'E') {
        app_pop();
        return;
    }
    if (g_ch_idx < 0 || !g_game.chests[g_ch_idx].active) {
        app_pop();
        return;
    }
    if (ev->key == CK_TAB || ev->key == CK_LEFT || ev->key == CK_RIGHT) {
        g_ch_side = 1 - g_ch_side;
    } else if (ev->key == CK_UP) {
        *sel = (*sel + n - 1) % n;
    } else if (ev->key == CK_DOWN) {
        *sel = (*sel + 1) % n;
    } else if (ev->key == CK_ENTER || ev->key == CK_SPACE) {
        transfer(g_ch_side, *sel);
    } else if (ev->key == 'a' || ev->key == 'A') {
        int i;
        for (i = 0; i < n; ++i) {
            transfer(g_ch_side, i);
        }
    }
}

const Screen scr_chest = { "chest", 0, 1, NULL, chest_key, chest_draw };
