/* Cliderness - ui/scr_overlay.c : pause menu, inventory, character, console, sky */
#include <stdio.h>
#include <string.h>
#include "ui/app.h"
#include "ui/widgets.h"
#include "gameplay/game.h"
#include "content/content.h"
#include "world/sky/sky.h"
#include "localization/locale.h"
#include "commands/console.h"
#include "input/keybind.h"
#include "renderer/cp437.h"
#include "core/log.h"
#include "core/str.h"
#include "core/version.h"

/* ------------------------------------------------------------ pause (Esc) */

static int g_pause_sel = 0;

static void pause_enter(void)
{
    g_pause_sel = 0;
}

static void pause_draw(Framebuffer *fb)
{
    const char *items[6];
    items[0] = loc_get("pause.resume");
    items[1] = loc_get("pause.save");
    items[2] = loc_get("menu.help");
    items[3] = loc_get("menu.settings");
    items[4] = loc_get("pause.save_quit");
    items[5] = loc_get("pause.quit_nosave");
    ui_frame(fb, 22, 5, 36, 13, loc_get("pause.title"), UI_BOX);
    fb_text_center(fb, 6, loc_get("pause.frozen"), ATTR(C_LIGHTCYAN, C_BLUE));
    menu_draw(fb, 24, 8, 32, items, 6, g_pause_sel, UI_BOX, UI_BOXSEL);
    fb_text_center(fb, 15, CLIDE_NAME " - Created by " CLIDE_CREATOR, ATTR(C_YELLOW, C_BLUE));
    ui_hint(fb, loc_get("hint.pause"));
}

static void pause_key(const ClideKeyEvent *ev)
{
    if (ev->key == CK_ESC) {
        app_pop();
        return;
    }
    if (ev->key == CK_F1 || ev->key == '?') {
        app_open_help("pause");
        return;
    }
    if (!menu_key(&g_pause_sel, 6, ev)) {
        return;
    }
    switch (g_pause_sel) {
    case 0: app_pop(); break;
    case 1: app_save_world(); app_pop(); break;
    case 2: app_open_help("start"); break;
    case 3: app_push(&scr_settings); break;
    case 4: app_close_world(1); app_reset(&scr_menu); break;
    default: app_close_world(0); app_reset(&scr_menu); break;
    }
}

const Screen scr_pause = { "pause", 1, 0, pause_enter, pause_key, pause_draw };

/* ------------------------------------------------------------ inventory (Tab) */

static int g_inv_sel = 0;
static int g_inv_pick = -1;
static TextInput g_inv_search;
static int g_inv_searching = 0;
static int g_inv_filter = -1;   /* item type or -1 */

static int inv_visible(int slot)
{
    const ItemStack *s = &g_game.inv.s[slot];
    if (!g_inv_search.buf[0] && g_inv_filter < 0) {
        return 1;
    }
    if (s->item < 0) {
        return 0;
    }
    if (g_inv_filter >= 0 && g_items[s->item].type != g_inv_filter) {
        return 0;
    }
    return !g_inv_search.buf[0] || str_icontains(item_name(s->item), g_inv_search.buf) ||
           str_icontains(g_items[s->item].id, g_inv_search.buf);
}

static void inv_enter(void)
{
    g_inv_sel = g_game.inv.selected;
    g_inv_pick = -1;
    g_inv_searching = 0;
    ti_set(&g_inv_search, "", 20, 0);
}

static void inv_draw(Framebuffer *fb)
{
    Game *g = &g_game;
    int i, row = 0;
    char buf[96];
    fb_clear(fb, UI_TEXT);
    ui_frame(fb, 0, 0, FB_W, FB_H - 1, loc_get("inv.title"), ATTR(C_LIGHTGRAY, C_BLACK));
    for (i = 0; i < INV_SLOTS; ++i) {
        const ItemStack *s = &g->inv.s[i];
        int col = i < 18 ? 0 : 1;
        int y = 2 + (i % 18);
        int x = 2 + col * 24;
        cl_u8 a = (i == g_inv_sel) ? UI_SEL : (i == g_inv_pick ? ATTR(C_BLACK, C_YELLOW) : UI_TEXT);
        if (!inv_visible(i)) {
            a = UI_DIM;
        }
        fb_fill(fb, x, y, 23, 1, ' ', a);
        sprintf(buf, "%c%2d", i < INV_HOTBAR ? '*' : ' ', i < INV_HOTBAR ? i + 1 : i + 1);
        fb_text(fb, x, y, buf, i < INV_HOTBAR && a == UI_TEXT ? ATTR(C_YELLOW, C_BLACK) : a, 3);
        if (s->item >= 0 && inv_visible(i)) {
            fb_put(fb, x + 4, y, g_items[s->item].glyph, i == g_inv_sel ? a : ATTR(g_items[s->item].fg, C_BLACK));
            fb_text(fb, x + 6, y, item_name(s->item), a, 13);
            sprintf(buf, "%3d", s->count);
            fb_text(fb, x + 20, y, buf, a, 3);
        }
        (void)row;
    }
    /* details */
    fb_vline(fb, 50, 1, FB_H - 3, G_VLINE, UI_DIM);
    if (g_inv_sel < INV_SLOTS) {
        const ItemStack *s = &g->inv.s[g_inv_sel];
        int y = 2;
        if (s->item >= 0) {
            const ItemDef *d = &g_items[s->item];
            fb_text(fb, 52, y++, item_name(s->item), UI_TITLE, 26);
            sprintf(buf, "Cliderness:%s", d->id);
            fb_text(fb, 52, y++, buf, UI_DIM, 26);
            fb_text(fb, 52, y++, loc_get_or(d->type == ITEM_BLOCK ? "itype.block" : d->type == ITEM_TOOL ? "itype.tool" :
                                            d->type == ITEM_FOOD ? "itype.food" : d->type == ITEM_WEAPON ? "itype.weapon" :
                                            d->type == ITEM_ARMOR ? "itype.armor" : d->type == ITEM_MATERIAL ? "itype.material" :
                                            d->type == ITEM_RELIC ? "itype.relic" : "itype.resource", item_type_name(d->type)),
                    UI_TEXT, 26);
            sprintf(buf, "%s %d / %d", loc_get("inv.count"), s->count, d->max_stack);
            fb_text(fb, 52, y++, buf, UI_TEXT, 26);
            sprintf(buf, "%s %ld g", loc_get("inv.weight"), (long)d->weight * s->count);
            fb_text(fb, 52, y++, buf, UI_TEXT, 26);
            sprintf(buf, "%s %ld cL", loc_get("inv.volume"), (long)d->volume * s->count);
            fb_text(fb, 52, y++, buf, UI_TEXT, 26);
            sprintf(buf, "%s %s", loc_get("inv.in_water"), loc_get(d->density < 1000 ? "inv.floats" : "inv.sinks"));
            fb_text(fb, 52, y++, buf, UI_TEXT, 26);
            if (d->durability > 0) {
                sprintf(buf, "%s %d/%d", loc_get("inv.durability"), s->dur, d->durability);
                fb_text(fb, 52, y, buf, UI_TEXT, 26);
                fb_bar(fb, 52, y + 1, 20, s->dur, d->durability, ATTR(C_LIGHTGREEN, C_BLACK));
                y += 2;
            }
            if (d->food || d->water) {
                sprintf(buf, "%s +%d  %s +%d", loc_get("hud.hunger"), d->food / 10, loc_get("hud.thirst"), d->water / 10);
                fb_text(fb, 52, y++, buf, UI_GOOD, 26);
            }
            if (d->type == ITEM_TOOL) {
                sprintf(buf, "%s %d%%", loc_get("inv.power"), d->tool_power);
                fb_text(fb, 52, y++, buf, UI_TEXT, 26);
            }
        } else {
            fb_text(fb, 52, y, loc_get("inv.empty_slot"), UI_DIM, 26);
        }
    }
    {
        {
            static const char *const eqk[INV_EQUIP] = { "eq.head", "eq.body", "eq.legs", "eq.feet", "eq.back" };
            int k;
            fb_text(fb, 52, 11, loc_get("inv.equipment"), UI_KEY, 26);
            for (k = 0; k < INV_EQUIP; ++k) {
                const ItemStack *e = &g->inv.equip[k];
                cl_u8 a = (g_inv_sel == INV_SLOTS + k) ? UI_SEL : UI_TEXT;
                fb_fill(fb, 52, 12 + k, 27, 1, ' ', a);
                fb_text(fb, 52, 12 + k, loc_get(eqk[k]), a == UI_SEL ? a : UI_DIM, 8);
                fb_text(fb, 60, 12 + k, e->item >= 0 ? item_name(e->item) : "-", a, 19);
            }
            sprintf(buf, "%s %d%%  %s %+d", loc_get("inv.defense"), inv_defense(&g->inv),
                    loc_get("inv.insulation"), inv_insulation(&g->inv));
            fb_text(fb, 52, 17, buf, UI_TEXT, 27);
        }
        sprintf(buf, "%s %ld.%01ld/%ld kg", loc_get("hud.carry"), inv_weight(&g->inv) / 1000L,
                (inv_weight(&g->inv) % 1000L) / 100L, game_carry_capacity(g) / 1000L);
        fb_text(fb, 52, 18, buf, inv_weight(&g->inv) > game_carry_capacity(g) ? UI_BAD : UI_TEXT, 26);
        sprintf(buf, "%s %ld L", loc_get("inv.volume"), inv_volume(&g->inv) / 100L);
        fb_text(fb, 52, 19, buf, UI_TEXT, 26);
        if (g_inv_searching || g_inv_search.buf[0]) {
            sprintf(buf, "/%s%s", g_inv_search.buf, g_inv_searching ? "_" : "");
            fb_text(fb, 52, 21, buf, UI_KEY, 26);
        }
        if (g_inv_filter >= 0) {
            fb_text(fb, 52, 22, item_type_name(g_inv_filter), UI_KEY, 26);
        }
    }
    ui_hint(fb, loc_get("hint.inventory"));
}

static void inv_key(const ClideKeyEvent *ev)
{
    Game *g = &g_game;
    if (g_inv_searching) {
        if (ev->key == CK_ENTER || ev->key == CK_ESC) {
            g_inv_searching = 0;
            if (ev->key == CK_ESC) {
                ti_set(&g_inv_search, "", 20, 0);
            }
            return;
        }
        ti_key(&g_inv_search, ev);
        return;
    }
    if (ev->key == CK_ESC || ev->key == CK_TAB || ev->key == 'i' || ev->key == 'I') {
        app_pop();
        return;
    }
    if (g_inv_sel >= INV_SLOTS) {
        /* equipment rows */
        if (ev->key == CK_UP) g_inv_sel = g_inv_sel > INV_SLOTS ? g_inv_sel - 1 : INV_SLOTS - 1;
        else if (ev->key == CK_DOWN) g_inv_sel = g_inv_sel + 1 < INV_SLOTS + INV_EQUIP ? g_inv_sel + 1 : 0;
        else if (ev->key == CK_LEFT || ev->key == CK_RIGHT) g_inv_sel = 18;
        else if (ev->key == CK_ENTER || ev->key == 'w' || ev->key == 'W') {
            int eq = g_inv_sel - INV_SLOTS;
            int it = g->inv.equip[eq].item;
            if (it >= 0 && inv_unequip(&g->inv, eq)) {
                log_add(UI_TEXT, loc_fmt("msg.unequipped", item_name(it), NULL, NULL));
            }
        }
        return;
    }
    if (ev->key == CK_UP) g_inv_sel = (g_inv_sel + INV_SLOTS - 1) % INV_SLOTS;
    else if (ev->key == CK_DOWN) g_inv_sel = (g_inv_sel + 1) % INV_SLOTS;
    else if (ev->key == CK_LEFT || ev->key == CK_RIGHT) g_inv_sel = g_inv_sel >= 18 ? INV_SLOTS : g_inv_sel + 18;
    else if (ev->key == 'w' || ev->key == 'W') {
        int it = g->inv.s[g_inv_sel].item;
        if (it >= 0 && g_items[it].equip && inv_equip(&g->inv, g_inv_sel)) {
            log_add(UI_GOOD, loc_fmt("msg.equipped", item_name(it), NULL, NULL));
        }
    }
    else if (ev->key == CK_PGUP || ev->key == CK_HOME) g_inv_sel = 0;
    else if (ev->key == CK_PGDN || ev->key == CK_END) g_inv_sel = INV_SLOTS - 1;
    else if (ev->key == CK_ENTER) {
        if (g_inv_pick < 0) {
            if (g->inv.s[g_inv_sel].item >= 0) {
                g_inv_pick = g_inv_sel;
            }
        } else {
            inv_move(&g->inv, g_inv_pick, g_inv_sel);
            g_inv_pick = -1;
        }
    } else if (ev->key == 'q' && !(ev->mods & KM_CTRL)) {
        game_drop(g, g_inv_sel, 0);
    } else if (ev->key == 'Q' || (ev->key == 'q' && (ev->mods & KM_CTRL))) {
        game_drop(g, g_inv_sel, 1);
    } else if (ev->key == 's' || ev->key == 'S') {
        inv_sort(&g->inv);
    } else if (ev->key == 'x' || ev->key == 'X') {
        inv_split(&g->inv, g_inv_sel);
    } else if (ev->key == '/') {
        g_inv_searching = 1;
    } else if (ev->key == 'f' || ev->key == 'F') {
        g_inv_filter = g_inv_filter + 1 >= ITEM_TYPE_COUNT ? -1 : g_inv_filter + 1;
    } else if (ev->key == 'u' || ev->key == 'U') {
        if (g_inv_sel < INV_HOTBAR) {
            g->inv.selected = g_inv_sel;
            game_use(g);
        }
    } else if (ev->key >= '1' && ev->key <= '9') {
        inv_swap(&g->inv, g_inv_sel, (int)(ev->key - '1'));
        g_inv_sel = (int)(ev->key - '1');
    } else if (ev->key == CK_F1 || ev->key == '?') {
        app_open_help("inventory");
    }
}

const Screen scr_inventory = { "inventory", 0, 1, inv_enter, inv_key, inv_draw };

/* ------------------------------------------------------------ character (C) */

static void stats_draw(Framebuffer *fb)
{
    Player *p = &g_game.p;
    int i;
    char buf[96];
    static const char *const effect_keys[ST_COUNT] = {
        "stat.str_fx", "stat.dex_fx", "stat.int_fx", "stat.per_fx",
        "stat.con_fx", "stat.end_fx", "stat.wil_fx", "stat.lck_fx"
    };
    fb_clear(fb, UI_TEXT);
    ui_frame(fb, 0, 0, FB_W, FB_H - 1, loc_get("stats.title"), ATTR(C_LIGHTGRAY, C_BLACK));
    fb_text(fb, 2, 2, loc_get("stats.attributes"), UI_TITLE, 36);
    for (i = 0; i < ST_COUNT; ++i) {
        sprintf(buf, "%3d", p->stats[i]);
        fb_text(fb, 2, 4 + i * 2, loc_get(player_stat_key(i)), UI_HI, 14);
        fb_text(fb, 16, 4 + i * 2, buf, UI_GOOD, 4);
        fb_text(fb, 4, 5 + i * 2, loc_get(effect_keys[i]), UI_DIM, 34);
    }
    fb_vline(fb, 40, 1, FB_H - 3, G_VLINE, UI_DIM);
    fb_text(fb, 42, 2, loc_get("stats.skills"), UI_TITLE, 36);
    for (i = 0; i < SK_COUNT; ++i) {
        int lvl = player_skill_level(p, i);
        long cur = p->skill_xp[i] - player_level_xp(lvl);
        long need = player_level_xp(lvl + 1) - player_level_xp(lvl);
        sprintf(buf, "%3d", lvl);
        fb_text(fb, 42, 4 + i, loc_get(player_skill_key(i)), UI_TEXT, 16);
        fb_text(fb, 58, 4 + i, buf, UI_HI, 4);
        fb_bar(fb, 63, 4 + i, 14, cur, need, ATTR(C_LIGHTCYAN, C_BLACK));
    }
    fb_text(fb, 42, 17, loc_get("stats.xp_rule"), UI_DIM, 36);
    fb_text(fb, 42, 18, loc_get("stats.xp_rule2"), UI_DIM, 36);
    ui_hint(fb, loc_get("hint.back"));
}

static void stats_key(const ClideKeyEvent *ev)
{
    if (ev->key == CK_ESC || ev->key == 'c' || ev->key == 'C' || ev->key == CK_ENTER) {
        app_pop();
    } else if (ev->key == CK_F1 || ev->key == '?') {
        app_open_help("stats");
    }
}

const Screen scr_stats = { "stats", 0, 1, NULL, stats_key, stats_draw };

/* ------------------------------------------------------------ console (/) */

static TextInput g_con_in;
static int g_con_hist = -1;
static int g_con_scroll = 0;

static void console_enter(void)
{
    ti_set(&g_con_in, "", 150, 1);
    g_con_hist = -1;
    g_con_scroll = 0;
}

static void console_draw(Framebuffer *fb)
{
    int i, y;
    int rows = 12;
    fb_fill(fb, 0, FB_H - rows - 2, FB_W, rows + 2, ' ', ATTR(C_LIGHTGRAY, C_BLACK));
    fb_hline(fb, 0, FB_H - rows - 2, FB_W, G_DHLINE, ATTR(C_YELLOW, C_BLACK));
    fb_text(fb, 2, FB_H - rows - 2, loc_get("con.title"), ATTR(C_YELLOW, C_BLACK), 40);
    for (i = 0; i < rows; ++i) {
        const LogEntry *e = log_get(rows - 1 - i + g_con_scroll);
        y = FB_H - rows - 1 + i;
        if (e) {
            fb_text(fb, 1, y, e->text, e->attr, FB_W - 2);
        }
    }
    fb_fill(fb, 0, FB_H - 1, FB_W, 1, ' ', ATTR(C_WHITE, C_BLUE));
    fb_text(fb, 0, FB_H - 1, "/", ATTR(C_YELLOW, C_BLUE), 1);
    fb_text(fb, 1, FB_H - 1, g_con_in.buf, ATTR(C_WHITE, C_BLUE), FB_W - 3);
    if ((g_app.frame / 8) % 2) {
        int cx = 1 + utf8_width(g_con_in.buf);
        if (cx < FB_W) {
            fb_put(fb, cx, FB_H - 1, '_', ATTR(C_YELLOW, C_BLUE));
        }
    }
}

static void console_key(const ClideKeyEvent *ev)
{
    if (ev->key == CK_ESC) {
        app_pop();
        return;
    }
    if (ev->key == CK_ENTER) {
        char line[CON_LINE];
        str_copy(line, g_con_in.buf, sizeof(line));
        ti_set(&g_con_in, "", 150, 1);
        g_con_hist = -1;
        g_con_scroll = 0;
        if (line[0]) {
            con_history_push(line);
            log_add(ATTR(C_WHITE, C_BLACK), line[0] == '/' ? line : loc_fmt("con.echo", line, NULL, NULL));
            con_exec(&g_game, g_app.p, line);
            if (g_con_request == CON_REQ_QUIT) {
                g_con_request = CON_REQ_NONE;
                app_close_world(1);
                app_reset(&scr_menu);
                return;
            }
            if (g_con_request >= CON_REQ_SKY && g_con_request <= CON_REQ_PANEL) {
                int req = g_con_request;
                g_con_request = CON_REQ_NONE;
                app_pop();   /* close the console, then open the requested screen */
                if (req == CON_REQ_SKY) app_push(&scr_sky);
                else if (req == CON_REQ_INVENTORY) app_push(&scr_inventory);
                else if (req == CON_REQ_CRAFT) app_push(&scr_craft);
                else if (req == CON_REQ_STATS) app_push(&scr_stats);
                else g_app.set.panel_page = g_con_arg;
                return;
            }
        } else {
            app_pop();
        }
        return;
    }
    if (ev->key == CK_TAB) {
        char line[CON_LINE];
        str_copy(line, g_con_in.buf, sizeof(line));
        if (con_complete(line, sizeof(line))) {
            ti_set(&g_con_in, line, 150, 1);
        }
        return;
    }
    if (ev->key == CK_UP || ev->key == CK_DOWN) {
        int n = con_history_count();
        if (n == 0) {
            return;
        }
        if (ev->key == CK_UP && g_con_hist < n - 1) {
            ++g_con_hist;
        } else if (ev->key == CK_DOWN && g_con_hist >= 0) {
            --g_con_hist;
        }
        ti_set(&g_con_in, g_con_hist >= 0 ? con_history_get(g_con_hist) : "", 150, 1);
        return;
    }
    if (ev->key == CK_PGUP) {
        if (g_con_scroll + 12 < log_count()) {
            g_con_scroll += 6;
        }
        return;
    }
    if (ev->key == CK_PGDN) {
        g_con_scroll = g_con_scroll > 6 ? g_con_scroll - 6 : 0;
        return;
    }
    if (ev->key == CK_F1) {
        app_open_help("commands");
        return;
    }
    ti_key(&g_con_in, ev);
}

const Screen scr_console = { "console", 0, 0, console_enter, console_key, console_draw };

/* ------------------------------------------------------------ sky (Right Shift) */

static int g_sky_names = 0;

static void sky_draw(Framebuffer *fb)
{
    ClideDate d;
    char buf[120];
    SkyInfo info;
    sky_render(fb, &g_game, 0, 0, FB_W, FB_H - 1, g_sky_names, &info);
    ctime_date(&g_game.time, &d);
    sprintf(buf, "%02d:%02d  ", d.hour, d.minute);
    str_cat(buf, loc_get(ctime_moon_key(d.moon_phase)), sizeof(buf));
    str_cat(buf, "  ", sizeof(buf));
    str_cat(buf, weather_name(g_game.weather.cur), sizeof(buf));
    if (info.solar_eclipse) {
        str_cat(buf, "  ", sizeof(buf));
        str_cat(buf, loc_get("sky.solar_eclipse"), sizeof(buf));
    }
    if (info.lunar_eclipse) {
        str_cat(buf, "  ", sizeof(buf));
        str_cat(buf, loc_get("sky.lunar_eclipse"), sizeof(buf));
    }
    if (info.comet) {
        str_cat(buf, "  ", sizeof(buf));
        str_cat(buf, loc_get("sky.comet"), sizeof(buf));
    }
    if (info.aurora) {
        str_cat(buf, "  ", sizeof(buf));
        str_cat(buf, loc_get("sky.aurora"), sizeof(buf));
    }
    ui_hint(fb, loc_get("hint.sky"));
    fb_text(fb, 1, 0, buf, ATTR(C_WHITE, ATTR_BG(fb->cells[0].attr)), FB_W - 2);
}

static void sky_key(const ClideKeyEvent *ev)
{
    int act = kb_action(ev, NULL);
    if (act == ACT_SKY || ev->key == 'q') {
        app_pop();
    } else if (ev->key == 'n' || ev->key == 'N') {
        g_sky_names = !g_sky_names;
    } else if (act == ACT_PAUSE) {
        app_push(&scr_pause);
    } else if (act == ACT_HELP) {
        app_open_help("sky");
    }
}

const Screen scr_sky = { "sky", 0, 1, NULL, sky_key, sky_draw };
