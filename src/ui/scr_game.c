/*
 * Cliderness - ui/scr_game.c
 * Main play screen (80x25):
 *   cols 0-55  rows 0-18  world view (top-down, camera centred on the player)
 *   col  56               separator
 *   cols 57-79 rows 0-18  status panel (3 pages, P to switch)
 *   row  19               hotbar
 *   rows 20-24            event log
 */
#include <stdio.h>
#include <string.h>
#include "ui/app.h"
#include "ui/widgets.h"
#include "gameplay/game.h"
#include "content/content.h"
#include "world/biomes/biome.h"
#include "world/weather/weather.h"
#include "localization/locale.h"
#include "commands/console.h"
#include "input/keybind.h"
#include "renderer/cp437.h"
#include "audio/audio.h"
#include "core/log.h"
#include "core/rng.h"
#include "core/mathx.h"
#include "core/str.h"
#include "core/mem.h"

#define VIEW_W 56
#define VIEW_H 19
#define PANEL_X 57
#define PANEL_W 23
#define MAX_LIGHTS 32

typedef struct LightSrc {
    long x, z;
    int r;
} LightSrc;

static int light_level(LightSrc *ls, int n, long x, long z)
{
    int i, best = 0;
    for (i = 0; i < n; ++i) {
        long dx = x - ls[i].x, dz = z - ls[i].z;
        long d2 = dx * dx + dz * dz;
        long r2 = (long)ls[i].r * ls[i].r;
        if (d2 <= r2) {
            int v = (int)(100L - d2 * 100L / (r2 + 1));
            if (v > best) {
                best = v;
            }
        }
    }
    return best;
}

static void draw_world(Framebuffer *fb, Game *g)
{
    Player *p = &g->p;
    long ox = p->x - VIEW_W / 2;
    long oz = p->z - VIEW_H / 2;
    int sx, sy, i;
    int radius = game_light_radius(g);
    int covered = game_is_covered(g, p->x, p->y, p->z);
    const WeatherDef *wd = weather_cur(&g->weather);
    LightSrc lights[MAX_LIGHTS];
    int nl = 0;
    long r2 = (long)radius * radius;
    long fx, fz;
    dir_delta(p->facing, &fx, &fz);
    fx += p->x;
    fz += p->z;
    /* collect light sources around the view (same level and one below) */
    for (sy = -2; sy < VIEW_H + 2 && nl < MAX_LIGHTS; ++sy) {
        for (sx = -2; sx < VIEW_W + 2 && nl < MAX_LIGHTS; ++sx) {
            cl_u8 b = world_get(&g->world, p->dim, ox + sx, p->y, oz + sy);
            cl_u8 b2 = world_get(&g->world, p->dim, ox + sx, p->y - 1, oz + sy);
            int lv = g_blocks[b].light > g_blocks[b2].light ? g_blocks[b].light : g_blocks[b2].light;
            if (lv > 0) {
                lights[nl].x = ox + sx;
                lights[nl].z = oz + sy;
                lights[nl].r = lv / 2 + 1;
                ++nl;
            }
        }
    }
    for (sy = 0; sy < VIEW_H; ++sy) {
        for (sx = 0; sx < VIEW_W; ++sx) {
            long wx = ox + sx, wz = oz + sy;
            cl_u8 b = world_get(&g->world, p->dim, wx, p->y, wz);
            const BlockDef *bd = &g_blocks[b];
            int glyph, fg, bg, depth = 0;
            long dx = wx - p->x, dz = wz - p->z;
            long d2 = dx * dx + dz * dz;
            int lit;
            if (b != BLOCK_AIR) {
                glyph = bd->glyph;
                fg = bd->fg;
                bg = bd->bg;
                /* terrain one block higher with open space above is a step
                 * you can climb: draw its top face, brightened */
                if ((bd->flags & BF_SOLID) && (bd->category <= 2 || bd->category == 4)) {
                    cl_u8 above = world_get(&g->world, p->dim, wx, p->y + 1, wz);
                    if (!blk_solid(above)) {
                        glyph = bd->floor_glyph;
                        fg = bd->floor_fg | 8;
                        bg = bd->floor_bg;
                        if (above != BLOCK_AIR && (g_blocks[above].flags & BF_PLANT)) {
                            glyph = g_blocks[above].glyph;
                            fg = g_blocks[above].fg | 8;
                        }
                    } else if (g_blocks[above].category <= 2 || g_blocks[above].category == 4) {
                        /* cliff: shade by how high it rises */
                        glyph = blk_solid(world_get(&g->world, p->dim, wx, p->y + 2, wz)) ? G_SHADE3 : G_SHADE2;
                        fg = g_blocks[above].fg;
                    }
                }
            } else {
                int k;
                glyph = ' ';
                fg = C_DARKGRAY;
                bg = C_BLACK;
                for (k = 1; k <= 12; ++k) {
                    cl_u8 fbk = world_get(&g->world, p->dim, wx, p->y - k, wz);
                    if (fbk != BLOCK_AIR) {
                        const BlockDef *f = &g_blocks[fbk];
                        glyph = f->floor_glyph;
                        fg = f->floor_fg;
                        bg = f->floor_bg;
                        depth = k;
                        break;
                    }
                }
                if (depth == 0) {
                    glyph = G_MIDDOT;
                    fg = C_DARKGRAY;
                } else if (depth >= 2) {
                    /* lower ground: same glyph, darker; deep drops lose the bg */
                    fg = color_dim(fg);
                    if (fg == C_BLACK) {
                        fg = C_DARKGRAY;
                    }
                    if (depth >= 3 && bg != C_BLUE && bg != C_RED) {
                        bg = C_BLACK;
                    }
                }
            }
            /* lighting */
            lit = d2 <= r2 ? 100 : 0;
            if (lit && d2 * 2 > r2) {
                lit = 60;
            }
            if (nl) {
                int ll = light_level(lights, nl, wx, wz);
                if (ll > lit) {
                    lit = ll;
                }
            }
            if (lit == 0) {
                fb_put(fb, sx, sy, ' ', ATTR(C_BLACK, C_BLACK));
                continue;
            }
            if (lit < 70) {
                fg = color_dim(fg);
                bg = color_dim(bg);
                if (fg == C_BLACK && glyph != ' ') {
                    fg = C_DARKGRAY;
                }
            }
            /* precipitation overlay */
            if (wd->precip && !covered && p->dim == g->p.dim) {
                cl_u32 h = rng_hash3(g->seed, wx * 7 + (g->tick / 2L), wz - (g->tick / 2L), 3);
                if ((long)(h % 1000UL) < (long)wd->intensity * 2L && b == BLOCK_AIR) {
                    glyph = wd->precip == PRECIP_SNOW ? '*' : wd->precip == PRECIP_SAND ? G_SHADE1 :
                            wd->precip == PRECIP_ASH ? G_MIDDOT : wd->precip == PRECIP_HAIL ? 'o' :
                            wd->precip == PRECIP_STAR ? '+' : ((h >> 12) & 1UL) ? ',' : '\'';
                    fg = wd->precip == PRECIP_SNOW ? C_WHITE : wd->precip == PRECIP_SAND ? C_YELLOW :
                         wd->precip == PRECIP_ASH ? C_DARKGRAY : wd->precip == PRECIP_STAR ? C_LIGHTMAGENTA :
                         C_LIGHTCYAN;
                }
            }
            if (g->weather.flash > 0 && !covered) {
                bg = C_DARKGRAY;
            }
            /* facing / mining target highlight */
            if (wx == fx && wz == fz) {
                bg = p->mining ? C_RED : (bg == C_BLACK ? C_DARKGRAY : bg);
            }
            fb_put(fb, sx, sy, glyph, ATTR(fg, bg));
        }
    }
    /* item entities */
    for (i = 0; i < MAX_ITEM_ENTS; ++i) {
        const ItemEntity *e = &g->ents[i];
        long dx, dz;
        if (!e->active || e->dim != p->dim || (e->y != p->y && e->y != p->y - 1)) {
            continue;
        }
        dx = e->x - ox;
        dz = e->z - oz;
        if (dx < 0 || dz < 0 || dx >= VIEW_W || dz >= VIEW_H) {
            continue;
        }
        if ((e->x - p->x) * (e->x - p->x) + (e->z - p->z) * (e->z - p->z) > r2) {
            continue;
        }
        fb_put(fb, (int)dx, (int)dz, g_items[e->item].glyph,
               ATTR(g_items[e->item].fg, e->floating ? C_BLUE : ATTR_BG(fb->cells[dz * FB_W + dx].attr)));
    }
    /* the player */
    {
        int pf = p->dead ? C_LIGHTRED : p->crouch ? C_LIGHTGRAY :
                 p->move_mode == MOVE_SWIM ? C_LIGHTCYAN : C_WHITE;
        FbCell *c = fb_cell(fb, VIEW_W / 2, VIEW_H / 2);
        fb_put(fb, VIEW_W / 2, VIEW_H / 2, p->dead ? '%' : '@', ATTR(pf, ATTR_BG(c->attr)));
    }
}

static void panel_line(Framebuffer *fb, int y, const char *label, const char *value, cl_u8 vattr)
{
    fb_text(fb, PANEL_X, y, label, UI_DIM, 8);
    fb_text(fb, PANEL_X + 8, y, value, vattr, PANEL_W - 8);
}

static void panel_bar(Framebuffer *fb, int y, const char *label, long v, long max, int color, int pct)
{
    char num[12];
    fb_text(fb, PANEL_X, y, label, UI_DIM, 8);
    fb_bar(fb, PANEL_X + 8, y, 9, v, max, ATTR(color, C_BLACK));
    if (pct) {
        sprintf(num, "%3ld%%", max ? v * 100L / max : 0L);
    } else {
        sprintf(num, "%4ld", v);
    }
    fb_text(fb, PANEL_X + 18, y, num, ATTR(color, C_BLACK), 5);
}

static int meter_color(long v, long max)
{
    long pct = max ? v * 100L / max : 0;
    return pct > 60 ? C_LIGHTGREEN : pct > 30 ? C_YELLOW : C_LIGHTRED;
}

static void draw_panel(Framebuffer *fb, Game *g)
{
    Player *p = &g->p;
    char buf[64];
    ClideDate d;
    int y = 0;
    int page = g_app.set.panel_page % 3;
    int biome = world_biome(&g->world, p->dim, p->x, p->z);
    fb_vline(fb, VIEW_W, 0, VIEW_H, G_VLINE, UI_DIM);
    ctime_date(&g->time, &d);
    if (p->y < world_gen_height(&g->world, p->dim, p->x, p->z) - 3 && game_is_covered(g, p->x, p->y, p->z)) {
        /* underground: classify the cave biome by depth and climate */
        int special = (int)(rng_hash3(g->seed, p->x >> 4, p->y >> 3, p->z >> 4) % 101UL);
        biome = biome_select(BK_CAVE, (int)p->y, world_climate_temp(&g->world, p->dim, p->x, p->z),
                             g_biomes[biome].humidity, special);
    }
    if (page == 0) {
        panel_bar(fb, y++, loc_get("hud.hp"), p->hp, p->hp_max, meter_color(p->hp, p->hp_max), 0);
        panel_bar(fb, y++, loc_get("hud.stamina"), p->stamina, 1000, meter_color(p->stamina, 1000), 1);
        panel_bar(fb, y++, loc_get("hud.hunger"), p->hunger, 1000, meter_color(p->hunger, 1000), 1);
        panel_bar(fb, y++, loc_get("hud.thirst"), p->thirst, 1000, meter_color(p->thirst, 1000), 1);
        panel_bar(fb, y++, loc_get("hud.fatigue"), p->fatigue, 1000, meter_color(1000 - p->fatigue, 1000), 1);
        sprintf(buf, "%d.%d%cC (%d%cC)", p->body_temp / 10, p->body_temp % 10, G_DEGREE, p->env_temp, G_DEGREE);
        /* the degree sign is a CP437 glyph: write it as UTF-8 */
        {
            char out[64];
            int k, o = 0;
            for (k = 0; buf[k] && o < 60; ++k) {
                if ((unsigned char)buf[k] == G_DEGREE) {
                    o += utf8_encode(0xB0L, out + o);
                } else {
                    out[o++] = buf[k];
                }
            }
            out[o] = '\0';
            panel_line(fb, y++, loc_get("hud.temp"), out,
                       ATTR(p->body_temp < 355 ? C_LIGHTCYAN : p->body_temp > 385 ? C_LIGHTRED : C_LIGHTGRAY, C_BLACK));
        }
        panel_line(fb, y++, loc_get("hud.move"), loc_get(player_move_key(p->move_mode)), UI_HI);
        sprintf(buf, "%d%%", p->noise);
        panel_line(fb, y++, loc_get("hud.noise"), buf, UI_TEXT);
        sprintf(buf, "%d%%", p->visibility);
        panel_line(fb, y++, loc_get("hud.visibility"), buf, UI_TEXT);
        sprintf(buf, "%ld.%ld/%ldkg", inv_weight(&g->inv) / 1000L, (inv_weight(&g->inv) % 1000L) / 100L,
                player_carry_capacity(p) / 1000L);
        panel_line(fb, y++, loc_get("hud.carry"), buf,
                   inv_weight(&g->inv) > player_carry_capacity(p) ? UI_BAD : UI_TEXT);
        panel_line(fb, y++, loc_get("hud.biome"), biome_name(biome), ATTR(g_biomes[biome].map_color == C_BLACK ? C_LIGHTGRAY : g_biomes[biome].map_color, C_BLACK));
        panel_line(fb, y++, loc_get("hud.weather"), weather_name(g->weather.cur), UI_TEXT);
        sprintf(buf, "%02d:%02d %s %d", d.hour, d.minute, loc_get(ctime_season_key(d.season)), d.day_of_month);
        panel_line(fb, y++, loc_get("hud.time"), buf, d.is_day ? ATTR(C_YELLOW, C_BLACK) : ATTR(C_LIGHTBLUE, C_BLACK));
        panel_line(fb, y++, loc_get("hud.dimension"), loc_get(p->dim == DIM_ANCIENT ? "dim.ancient" : "dim.overworld"), UI_TEXT);
        sprintf(buf, "%ld,%ld,%ld", p->x, p->y, p->z);
        panel_line(fb, y++, loc_get("hud.pos"), buf, UI_DIM);
        panel_line(fb, y++, loc_get("hud.mode"), loc_get(game_mode_key(g->mode)), UI_TEXT);
        {
            char flags[32];
            flags[0] = '\0';
            if (p->god) str_cat(flags, "GOD ", sizeof(flags));
            if (p->fly) str_cat(flags, "FLY ", sizeof(flags));
            if (p->noclip) str_cat(flags, "NOCLIP ", sizeof(flags));
            if (g->cheats) str_cat(flags, "CHEATS", sizeof(flags));
            fb_text(fb, PANEL_X, y++, flags, ATTR(C_LIGHTMAGENTA, C_BLACK), PANEL_W);
        }
    } else if (page == 1) {
        static const char *const dirs[4] = { "N", "E", "S", "W" };
        fb_text(fb, PANEL_X, y++, loc_get("hud.page_world"), UI_TITLE, PANEL_W);
        sprintf(buf, "%s %ld, %02d/%02d", loc_get("ui.year"), d.year, d.month, d.day_of_month);
        panel_line(fb, y++, loc_get("hud.date"), buf, UI_TEXT);
        panel_line(fb, y++, loc_get("hud.season"), loc_get(ctime_season_key(d.season)), UI_TEXT);
        panel_line(fb, y++, loc_get("hud.moon"), loc_get(ctime_moon_key(d.moon_phase)), UI_TEXT);
        sprintf(buf, "%02d:%02d / %02d:%02d", d.sunrise_min / 60, d.sunrise_min % 60, d.sunset_min / 60, d.sunset_min % 60);
        panel_line(fb, y++, loc_get("hud.sun"), buf, UI_TEXT);
        sprintf(buf, "%d%% %s", weather_cur(&g->weather)->wind,
                dirs[((g->weather.wind_dir + 32) / 64) & 3]);
        panel_line(fb, y++, loc_get("hud.wind"), buf, UI_TEXT);
        sprintf(buf, "%ld min", g->weather.minutes_left);
        panel_line(fb, y++, loc_get("hud.forecast"), buf, UI_TEXT);
        panel_line(fb, y++, loc_get("hud.facing"), dirs[p->facing & 3], UI_TEXT);
        sprintf(buf, "%d", world_gen_height(&g->world, p->dim, p->x, p->z));
        panel_line(fb, y++, loc_get("hud.surface"), buf, UI_TEXT);
        sprintf(buf, "%d", SEA_LEVEL);
        panel_line(fb, y++, loc_get("hud.sea"), buf, UI_TEXT);
        sprintf(buf, "%d", ent_count(g));
        panel_line(fb, y++, loc_get("hud.items"), buf, UI_TEXT);
        sprintf(buf, "%lu", (unsigned long)g->seed);
        panel_line(fb, y++, loc_get("ui.seed"), buf, UI_DIM);
    } else {
        int k;
        fb_text(fb, PANEL_X, y++, loc_get("hud.page_skills"), UI_TITLE, PANEL_W);
        for (k = 0; k < SK_COUNT && y < VIEW_H; ++k) {
            sprintf(buf, "%3d", player_skill_level(p, k));
            fb_text(fb, PANEL_X, y, loc_get(player_skill_key(k)), UI_DIM, 18);
            fb_text(fb, PANEL_X + 19, y++, buf, UI_TEXT, 4);
        }
    }
    if (p->mining && y < VIEW_H) {
        fb_text(fb, PANEL_X, VIEW_H - 1, loc_get("hud.mining"), ATTR(C_YELLOW, C_BLACK), 8);
        fb_bar(fb, PANEL_X + 8, VIEW_H - 1, 14, p->mine_progress, p->mine_total, ATTR(C_YELLOW, C_BLACK));
    }
    if (g_con_debug) {
        sprintf(buf, "t%ld c%d m%luK", g->tick, world_cached_chunks(&g->world), (unsigned long)(mem_in_use() / 1024UL));
        fb_text(fb, 0, 0, buf, ATTR(C_BLACK, C_YELLOW), VIEW_W);
    }
}

static void draw_hotbar(Framebuffer *fb, Game *g)
{
    int i;
    int x = 0;
    fb_fill(fb, 0, 19, FB_W, 1, ' ', ATTR(C_LIGHTGRAY, C_BLUE));
    for (i = 0; i < INV_HOTBAR; ++i) {
        const ItemStack *s = &g->inv.s[i];
        cl_u8 a = i == g->inv.selected ? ATTR(C_BLACK, C_LIGHTGRAY) : ATTR(C_LIGHTGRAY, C_BLUE);
        char buf[8];
        fb_fill(fb, x, 19, 6, 1, ' ', a);
        buf[0] = (char)('1' + i);
        buf[1] = '\0';
        fb_text(fb, x, 19, buf, i == g->inv.selected ? a : ATTR(C_YELLOW, C_BLUE), 1);
        if (s->item >= 0) {
            fb_put(fb, x + 1, 19, g_items[s->item].glyph,
                   ATTR(g_items[s->item].fg == ATTR_BG(a) ? C_WHITE : g_items[s->item].fg, ATTR_BG(a)));
            if (s->count > 1) {
                sprintf(buf, "%d", s->count);
                fb_text(fb, x + 2, 19, buf, a, 3);
            } else if (g_items[s->item].durability > 0) {
                sprintf(buf, "%d", (int)((long)s->dur * 99L / g_items[s->item].durability));
                fb_text(fb, x + 2, 19, buf, a, 3);
            }
        }
        x += 6;
    }
    {
        const ItemStack *s = &g->inv.s[g->inv.selected];
        if (s->item >= 0) {
            fb_text(fb, x + 1, 19, item_name(s->item), ATTR(C_WHITE, C_BLUE), FB_W - x - 2);
        }
    }
}

static void draw_log(Framebuffer *fb)
{
    int i;
    for (i = 0; i < 5; ++i) {
        const LogEntry *e = log_get(4 - i);
        if (!e) {
            continue;
        }
        fb_text(fb, 0, 20 + i, e->text,
                i >= 3 ? e->attr : ATTR(color_dim(ATTR_FG(e->attr)), C_BLACK), FB_W - 6);
        if (e->repeat > 1) {
            char rb[10];
            sprintf(rb, " x%d", e->repeat);
            fb_text(fb, FB_W - 6, 20 + i, rb, UI_DIM, 6);
        }
    }
}

static void game_draw(Framebuffer *fb)
{
    if (!g_game.active) {
        return;
    }
    draw_world(fb, &g_game);
    draw_panel(fb, &g_game);
    draw_hotbar(fb, &g_game);
    draw_log(fb);
}

static void game_key(const ClideKeyEvent *ev)
{
    int sprint = 0;
    int act = kb_action(ev, &sprint);
    Game *g = &g_game;
    if (!g->active) {
        return;
    }
    if (g->p.dead && act != ACT_PAUSE && act != ACT_HELP && act != ACT_CONSOLE) {
        if (act == ACT_INTERACT || ev->key == CK_ENTER) {
            game_respawn(g);
        }
        return;
    }
    switch (act) {
    case ACT_MOVE_N: game_request_move(g, DIR_N, sprint); break;
    case ACT_MOVE_E: game_request_move(g, DIR_E, sprint); break;
    case ACT_MOVE_S: game_request_move(g, DIR_S, sprint); break;
    case ACT_MOVE_W: game_request_move(g, DIR_W, sprint); break;
    case ACT_CROUCH: game_toggle_crouch(g); break;
    case ACT_SKY: app_push(&scr_sky); break;
    case ACT_INVENTORY: app_push(&scr_inventory); break;
    case ACT_DROP: game_drop(g, g->inv.selected, 0); break;
    case ACT_DROP_STACK: game_drop(g, g->inv.selected, 1); break;
    case ACT_STATS: app_push(&scr_stats); break;
    case ACT_INTERACT: game_interact(g); aud_play(SFX_PICKUP); break;
    case ACT_HELP: app_open_help("controls"); break;
    case ACT_PAUSE: app_push(&scr_pause); break;
    case ACT_CONSOLE: app_push(&scr_console); break;
    case ACT_MINE: game_mine(g); aud_play(SFX_MINE); break;
    case ACT_PLACE: game_place(g); aud_play(SFX_PLACE); break;
    case ACT_DIG_DOWN: game_dig_down(g); break;
    case ACT_BUILD_UP: game_build_up(g); break;
    case ACT_USE: game_use(g); break;
    case ACT_PANEL: g_app.set.panel_page = (g_app.set.panel_page + 1) % 3; break;
    default:
        if (act >= ACT_HOT1 && act <= ACT_HOT9) {
            game_select(g, act - ACT_HOT1);
        }
        break;
    }
}

const Screen scr_game = { "game", 0, 1, NULL, game_key, game_draw };
