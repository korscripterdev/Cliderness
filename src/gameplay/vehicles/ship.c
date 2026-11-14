/* Cliderness - gameplay/vehicles/ship.c : ships - loading, placing, sailing, damage */
#include <stdio.h>
#include <string.h>
#include "gameplay/game.h"
#include "gameplay/vehicles/ship.h"
#include "content/content.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/ini.h"
#include "core/log.h"
#include "core/mathx.h"
#include "core/str.h"

ShipDef g_ships[MAX_SHIP_DEFS];
int g_ship_count = 0;

/* ---------------------------------------------------------------- loader */

typedef struct SCtx {
    int cur;
    char *err;
    size_t errn;
    char items[MAX_SHIP_DEFS][3][24];   /* item, repair, wreck ids */
} SCtx;

static int serr(SCtx *c, const char *what, const char *v, int line)
{
    char num[12];
    str_from_long(num, (long)line);
    str_copy(c->err, "ships.def:", c->errn);
    str_cat(c->err, num, c->errn);
    str_cat(c->err, ": bad ", c->errn);
    str_cat(c->err, what, c->errn);
    str_cat(c->err, " ", c->errn);
    str_cat(c->err, v ? v : "", c->errn);
    return 1;
}

static int scb(void *user, const char *type, const char *id, const char *key, const char *v, int line)
{
    SCtx *c = (SCtx *)user;
    ShipDef *d;
    long n;
    if (!key) {
        if (!str_ieq(type, "ship")) return serr(c, "section", type, line);
        if (g_ship_count >= MAX_SHIP_DEFS) return serr(c, "count", id, line);
        c->cur = g_ship_count++;
        d = &g_ships[c->cur];
        memset(d, 0, sizeof(*d));
        memset(c->items[c->cur], 0, sizeof(c->items[c->cur]));
        str_copy(d->id, id, sizeof(d->id));
        str_copy(d->name, id, sizeof(d->name));
        str_copy(c->items[c->cur][0], id, 24);
        d->length = 1;
        d->width = 1;
        d->glyph = 240;
        d->color = C_BROWN;
        d->sail_glyph = 244;
        d->crew = 1;
        d->hull = 40;
        d->oar_speed = 30;
        d->capacity = 300;
        d->wave_limit = 50;
        d->wreck_count = 1;
        return 0;
    }
    d = &g_ships[c->cur];
    if (str_ieq(key, "name")) {
        str_copy(d->name, v, sizeof(d->name));
    } else if (str_ieq(key, "item")) {
        str_copy(c->items[c->cur][0], v, 24);
    } else if (str_ieq(key, "repair")) {
        str_copy(c->items[c->cur][1], v, 24);
    } else if (str_ieq(key, "wreck")) {
        str_copy(c->items[c->cur][2], v, 24);
    } else if (str_ieq(key, "color")) {
        int col = color_from_name(v);
        if (col < 0) return serr(c, "colour", v, line);
        d->color = (cl_u8)col;
    } else {
        if (!str_to_long(v, &n) || n < 0 || n > 30000) return serr(c, key, v, line);
        if (str_ieq(key, "length")) { if (n < 1 || n > 9) return serr(c, key, v, line); d->length = (cl_u8)n; }
        else if (str_ieq(key, "width")) { if (n != 1 && n != 3) return serr(c, key, v, line); d->width = (cl_u8)n; }
        else if (str_ieq(key, "glyph")) d->glyph = (cl_u8)n;
        else if (str_ieq(key, "sail_glyph")) d->sail_glyph = (cl_u8)n;
        else if (str_ieq(key, "crew")) d->crew = (cl_u8)CL_MAX(1L, n);
        else if (str_ieq(key, "hull")) d->hull = (cl_s16)CL_MAX(1L, n);
        else if (str_ieq(key, "oar_speed")) d->oar_speed = (cl_s16)n;
        else if (str_ieq(key, "sail_speed")) d->sail_speed = (cl_s16)n;
        else if (str_ieq(key, "capacity")) d->capacity = (cl_s16)CL_MAX(10L, n);
        else if (str_ieq(key, "cargo")) d->cargo = (cl_u8)CL_MIN(n, (long)CHEST_SLOTS);
        else if (str_ieq(key, "wave_limit")) d->wave_limit = (cl_s16)n;
        else if (str_ieq(key, "wreck_count")) d->wreck_count = (cl_s16)n;
        else return serr(c, "key", key, line);
    }
    return 0;
}

int ships_load(const char *root, char *err, size_t errn)
{
    char path[160];
    static SCtx c;
    int r, i;
    g_ship_count = 0;
    memset(&c, 0, sizeof(c));
    c.err = err;
    c.errn = errn;
    str_copy(path, root, sizeof(path));
    str_cat(path, "/vehicles/ships.def", sizeof(path));
    r = ini_parse_file(path, scb, &c);
    if (r < 0) {
        str_copy(err, "cannot open ", errn);
        str_cat(err, path, errn);
        return 0;
    }
    if (r > 0) {
        return 0;
    }
    for (i = 0; i < g_ship_count; ++i) {
        ShipDef *d = &g_ships[i];
        d->item = (cl_s16)item_find(c.items[i][0]);
        d->repair_item = (cl_s16)(c.items[i][1][0] ? item_find(c.items[i][1]) : -1);
        d->wreck_item = (cl_s16)(c.items[i][2][0] ? item_find(c.items[i][2]) : -1);
        if (d->item < 0 || (c.items[i][1][0] && d->repair_item < 0) || (c.items[i][2][0] && d->wreck_item < 0)) {
            str_copy(err, "ships.def: unknown item for ", errn);
            str_cat(err, d->id, errn);
            return 0;
        }
    }
    return 1;
}

int shipdef_find(const char *id)
{
    int i;
    const char *colon = strchr(id, ':');
    if (colon) id = colon + 1;
    for (i = 0; i < g_ship_count; ++i) {
        if (str_ieq(g_ships[i].id, id)) return i;
    }
    return -1;
}

int shipdef_for_item(int item)
{
    int i;
    for (i = 0; i < g_ship_count; ++i) {
        if (item >= 0 && g_ships[i].item == item) return i;
    }
    return -1;
}

const char *shipdef_name(int s)
{
    return (s >= 0 && s < g_ship_count) ? item_name(g_ships[s].item) : "?";
}

const char *ship_stage_key(int stage)
{
    static const char *const k[3] = { "ship.intact", "ship.damaged", "ship.critical" };
    return k[(stage >= 0 && stage < 3) ? stage : 0];
}

/* --------------------------------------------------------------- runtime */

static cl_u8 at(Game *g, int dim, long x, long y, long z)
{
    return world_get(&g->world, dim, x, y, z);
}

static int heading_angle(int dir)
{
    return ((dir + 3) & 3) * 64;   /* N 192, E 0, S 64, W 128 */
}

/* footprint cell k (0 = stern .. len-1 = bow), lateral offset w (-1, 0, 1) */
static void cell_of(const ShipDef *d, long x, long z, int heading, int k, int w, long *cx, long *cz)
{
    long dx, dz;
    int off = k - d->length / 2;
    dir_delta(heading, &dx, &dz);
    *cx = x + dx * off - dz * w;
    *cz = z + dz * off + dx * w;
}

static int lateral(const ShipDef *d, int k)
{
    /* large ships are 3 wide except at bow and stern */
    return (d->width == 3 && k > 0 && k < d->length - 1) ? 1 : 0;
}

/* can the hull lie at (x, y, z) with heading? ignore = ship index to skip */
static int fits(Game *g, int def, int dim, long x, long y, long z, int heading, int ignore)
{
    const ShipDef *d = &g_ships[def];
    int k, w, i;
    for (k = 0; k < d->length; ++k) {
        int lw = lateral(d, k);
        for (w = -lw; w <= lw; ++w) {
            long cx, cz;
            cl_u8 b, below;
            cell_of(d, x, z, heading, k, w, &cx, &cz);
            b = at(g, dim, cx, y, cz);
            below = at(g, dim, cx, y - 1, cz);
            if (blk_solid(b) || blk_liquid(b) || !blk_liquid(below) || below == (cl_u8)g_cb.lava) {
                return 0;
            }
            for (i = 0; i < MAX_SHIPS; ++i) {
                if (i != ignore && g->ships[i].active && g->ships[i].dim == dim &&
                    g->ships[i].y == y && ship_covers(g, i, cx, cz)) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

int ship_covers(Game *g, int idx, long x, long z)
{
    const Ship *s = &g->ships[idx];
    const ShipDef *d = &g_ships[s->def];
    int k, w;
    long r = d->length / 2 + 2;
    if (mx_abs(x - s->x) > r || mx_abs(z - s->z) > r) {
        return 0;
    }
    for (k = 0; k < d->length; ++k) {
        int lw = lateral(d, k);
        for (w = -lw; w <= lw; ++w) {
            long cx, cz;
            cell_of(d, s->x, s->z, s->heading, k, w, &cx, &cz);
            if (cx == x && cz == z) return 1;
        }
    }
    return 0;
}

/* glyph role of a cell: 0 none, 1 hull, 2 bow, 3 mast */
int ship_cell_role(Game *g, int idx, long x, long z)
{
    const Ship *s = &g->ships[idx];
    const ShipDef *d = &g_ships[s->def];
    int k, w;
    for (k = 0; k < d->length; ++k) {
        int lw = lateral(d, k);
        for (w = -lw; w <= lw; ++w) {
            long cx, cz;
            cell_of(d, s->x, s->z, s->heading, k, w, &cx, &cz);
            if (cx == x && cz == z) {
                if (k == d->length - 1 && d->length > 1) return 2;
                if (d->sail_speed > 0 && k == d->length / 2 && w == 0) return 3;
                return 1;
            }
        }
    }
    return 0;
}

int ship_at(Game *g, int dim, long x, long y, long z)
{
    int i;
    for (i = 0; i < MAX_SHIPS; ++i) {
        const Ship *s = &g->ships[i];
        if (s->active && s->dim == dim && (s->y == y || s->y == y + 1 || s->y == y - 1) && ship_covers(g, i, x, z)) {
            return i;
        }
    }
    return -1;
}

int ship_count(const Game *g)
{
    int i, n = 0;
    for (i = 0; i < MAX_SHIPS; ++i) n += g->ships[i].active;
    return n;
}

int ship_stage(const Game *g, int idx)
{
    const Ship *s = &g->ships[idx];
    long max = g_ships[s->def].hull;
    if ((long)s->hull * 3 > max * 2) return SHIP_INTACT;
    if ((long)s->hull * 3 > max) return SHIP_DAMAGED;
    return SHIP_CRITICAL;
}

long ship_max_water(const Game *g, int idx)
{
    return CL_MAX(10L, (long)g_ships[g->ships[idx].def].capacity / 4);
}

int ship_flood_pct(const Game *g, int idx)
{
    return (int)CL_MIN(100L, (long)g->ships[idx].water * 100L / ship_max_water(g, idx));
}

/* kg carried: cargo + people (+ the player's inventory when aboard) */
long ship_load(Game *g, int idx)
{
    const Ship *s = &g->ships[idx];
    long grams = 0;
    int k;
    if (s->chest >= 0 && g->chests[s->chest].active) {
        const Chest *c = &g->chests[s->chest];
        for (k = 0; k < CHEST_SLOTS; ++k) {
            if (c->s[k].item >= 0) {
                grams += (long)g_items[c->s[k].item].weight * c->s[k].count;
            }
        }
    }
    if (g->aboard == idx) {
        grams += inv_weight(&g->inv) + 75000L;
    }
    return grams / 1000L;
}

int ship_wind_pct(Game *g, int idx)
{
    const Ship *s = &g->ships[idx];
    int diff = (g->weather.wind_dir - heading_angle(s->heading)) & 255;
    int angle;
    if (diff > 128) diff = 256 - diff;
    angle = diff < 24 ? 100 : diff < 56 ? 75 : diff < 96 ? 45 : 15;
    return weather_cur(&g->weather)->wind * angle / 100;
}

/* current speed in cells per 100 ticks */
int ship_speed(Game *g, int idx)
{
    const Ship *s = &g->ships[idx];
    const ShipDef *d = &g_ships[s->def];
    long oars = 0, sails = 0, v;
    if (s->anchored || s->throttle == 0 || g->aboard != idx) {
        return 0;
    }
    if (s->throttle < 0) {
        return d->oar_speed / 3;
    }
    if (g->p.stamina >= 4 * s->throttle || g->mode != GM_SURVIVAL || g->p.god) {
        oars = (long)d->oar_speed * s->throttle / 3;
    }
    if (d->sail_speed > 0) {
        sails = (long)d->sail_speed * s->throttle / 3 * ship_wind_pct(g, idx) / 100;
    }
    v = CL_MAX(oars, sails);
    v = v * (100 + player_skill_level(&g->p, SK_SAILING)) / 100;
    if (ship_load(g, idx) > d->capacity) {
        v /= 2;
    }
    v = v * (100 - ship_flood_pct(g, idx) / 2) / 100;
    return (int)v;
}

int ship_spawn(Game *g, int def, int dim, long x, long y, long z, int heading, int hull)
{
    int i, c;
    if (def < 0 || def >= g_ship_count) {
        return -1;
    }
    for (i = 0; i < MAX_SHIPS; ++i) {
        Ship *s = &g->ships[i];
        if (!s->active) {
            memset(s, 0, sizeof(*s));
            s->active = 1;
            s->def = (cl_s16)def;
            s->dim = (cl_u8)dim;
            s->x = x;
            s->y = y;
            s->z = z;
            s->heading = (cl_u8)(heading & 3);
            s->hull = (cl_s16)(hull > 0 ? CL_MIN(hull, (int)g_ships[def].hull) : g_ships[def].hull);
            s->chest = -1;
            if (g_ships[def].cargo > 0) {
                c = chest_find(g, CHEST_DIM_SHIP + i, 0, 0, 0);
                if (c < 0) c = chest_create(g, CHEST_DIM_SHIP + i, 0, 0, 0);
                s->chest = (cl_s16)c;
            }
            return i;
        }
    }
    return -1;
}

static void board_player(Game *g, int idx)
{
    Ship *s = &g->ships[idx];
    g->aboard = idx;
    g->p.x = s->x;
    g->p.y = s->y;
    g->p.z = s->z;
    g->p.mining = 0;
    g->p.facing = s->heading;
}

void ship_remove(Game *g, int idx, int wreck)
{
    Ship *s = &g->ships[idx];
    const ShipDef *d = &g_ships[s->def];
    int k;
    if (!s->active) {
        return;
    }
    if (s->chest >= 0 && g->chests[s->chest].active) {
        Chest *c = &g->chests[s->chest];
        for (k = 0; k < CHEST_SLOTS; ++k) {
            if (c->s[k].item >= 0 && wreck) {
                ent_spawn(g, s->dim, s->x, s->y, s->z, c->s[k].item, c->s[k].count, c->s[k].dur);
            }
            c->s[k].item = -1;
            c->s[k].count = 0;
        }
        chest_remove(g, s->chest);
    }
    if (wreck && d->wreck_item >= 0 && d->wreck_count > 0) {
        ent_spawn(g, s->dim, s->x, s->y, s->z, d->wreck_item, d->wreck_count, 0);
    }
    if (g->aboard == idx) {
        g->aboard = -1;
    }
    s->active = 0;
}

/* place the ship item in hand onto the water in front of the player */
int ship_place(Game *g)
{
    ItemStack *st = inv_selected(&g->inv);
    int def = shipdef_for_item(st->item), idx;
    long dx, dz, x, z, y;
    Player *p = &g->p;
    if (def < 0) {
        return 0;
    }
    dir_delta(p->facing, &dx, &dz);
    /* the stern starts next to the player */
    x = p->x + dx * (1 + g_ships[def].length / 2);
    z = p->z + dz * (1 + g_ships[def].length / 2);
    for (y = p->y + 1; y >= p->y - 3; --y) {
        if (fits(g, def, p->dim, x, y, z, p->facing, -1)) {
            idx = ship_spawn(g, def, p->dim, x, y, z, p->facing, st->dur);
            if (idx < 0) {
                log_add(ATTR(C_YELLOW, C_BLACK), loc_get("msg.ship_limit"));
                return 1;
            }
            inv_remove_slot(&g->inv, g->inv.selected, 1);
            log_add(ATTR(C_LIGHTGREEN, C_BLACK), loc_fmt("msg.ship_placed", shipdef_name(def), NULL, NULL));
            g->unsaved = 1;
            return 1;
        }
    }
    log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_fmt("msg.ship_need_water", shipdef_name(def), NULL, NULL));
    return 1;
}

/* pick an empty ship back up as an item (F on a ship while ashore) */
int ship_pickup(Game *g, int idx)
{
    Ship *s = &g->ships[idx];
    const ShipDef *d = &g_ships[s->def];
    int k;
    if (g->aboard >= 0) {
        return 0;
    }
    if (s->chest >= 0) {
        for (k = 0; k < CHEST_SLOTS; ++k) {
            if (g->chests[s->chest].s[k].item >= 0) {
                log_add(ATTR(C_YELLOW, C_BLACK), loc_get("msg.ship_cargo_full"));
                return 1;
            }
        }
    }
    if (inv_add(&g->inv, d->item, 1, s->hull) > 0) {
        ent_spawn(g, g->p.dim, g->p.x, g->p.y, g->p.z, d->item, 1, s->hull);
    }
    log_add(ATTR(C_LIGHTGREEN, C_BLACK), loc_fmt("msg.ship_picked", shipdef_name(s->def), NULL, NULL));
    ship_remove(g, idx, 0);
    g->unsaved = 1;
    return 1;
}

int ship_board(Game *g)
{
    long dx, dz;
    int idx;
    dir_delta(g->p.facing, &dx, &dz);
    idx = ship_at(g, g->p.dim, g->p.x + dx, g->p.y, g->p.z + dz);
    if (idx < 0) idx = ship_at(g, g->p.dim, g->p.x, g->p.y, g->p.z);
    if (idx < 0) {
        return 0;
    }
    board_player(g, idx);
    log_add(ATTR(C_LIGHTCYAN, C_BLACK), loc_fmt("msg.ship_boarded", shipdef_name(g->ships[idx].def), NULL, NULL));
    if (ship_load(g, idx) > g_ships[g->ships[idx].def].capacity) {
        log_add(ATTR(C_LIGHTRED, C_BLACK), loc_get("msg.ship_overloaded"));
    }
    return 1;
}

int ship_disembark(Game *g)
{
    Ship *s;
    const ShipDef *d;
    int k, w, dir, pass;
    if (g->aboard < 0) {
        return 0;
    }
    s = &g->ships[g->aboard];
    d = &g_ships[s->def];
    /* pass 0: land next to the hull, pass 1: jump into the water */
    for (pass = 0; pass < 2; ++pass) {
        for (k = 0; k < d->length; ++k) {
            int lw = lateral(d, k);
            for (w = -lw; w <= lw; ++w) {
                long cx, cz;
                cell_of(d, s->x, s->z, s->heading, k, w, &cx, &cz);
                for (dir = 0; dir < 4; ++dir) {
                    long dx, dz, nx, nz, ny;
                    dir_delta(dir, &dx, &dz);
                    nx = cx + dx;
                    nz = cz + dz;
                    if (ship_covers(g, g->aboard, nx, nz)) continue;
                    for (ny = s->y + 1; ny >= s->y - 1; --ny) {
                        cl_u8 b = at(g, s->dim, nx, ny, nz), below = at(g, s->dim, nx, ny - 1, nz);
                        if (blk_solid(b) || blk_solid(at(g, s->dim, nx, ny + 1, nz))) continue;
                        if ((pass == 0 && blk_solid(below)) || (pass == 1 && !blk_liquid(b) && blk_liquid(below))) {
                            g->p.x = nx;
                            g->p.y = ny;
                            g->p.z = nz;
                            g->aboard = -1;
                            log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get(pass == 0 ? "msg.ship_left" : "msg.ship_jumped"));
                            return 1;
                        }
                    }
                }
            }
        }
    }
    log_add(ATTR(C_YELLOW, C_BLACK), loc_get("msg.ship_no_exit"));
    return 1;
}

/* movement keys while aboard: forward = faster, back = slower / reverse,
 * sideways = turn the bow that way (needs room) */
void ship_steer(Game *g, int dir)
{
    Ship *s = &g->ships[g->aboard];
    const ShipDef *d = &g_ships[s->def];
    dir &= 3;
    if (dir == s->heading) {
        if (s->throttle < 3) ++s->throttle;
    } else if (dir == ((s->heading + 2) & 3)) {
        if (s->throttle > (d->oar_speed > 0 ? -1 : 0)) --s->throttle;
    } else if (fits(g, s->def, s->dim, s->x, s->y, s->z, dir, g->aboard)) {
        s->heading = (cl_u8)dir;
        s->acc = 0;
        g->p.facing = dir;
    } else if (g->blocked_msg == 0) {
        log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get("msg.ship_no_turn"));
        g->blocked_msg = 20;
    }
    if (s->anchored && s->throttle != 0) {
        s->anchored = 0;
        log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get("msg.anchor_up"));
    }
}

static void damage_hull(Game *g, int idx, int dmg, int sfx)
{
    Ship *s = &g->ships[idx];
    int before = ship_stage(g, idx);
    s->hull = (cl_s16)CL_MAX(0, s->hull - dmg);
    g->sfx = sfx;
    if (ship_stage(g, idx) != before && g->aboard == idx) {
        log_add(ATTR(ship_stage(g, idx) == SHIP_CRITICAL ? C_LIGHTRED : C_YELLOW, C_BLACK),
                loc_fmt("msg.ship_stage", shipdef_name(s->def), loc_get(ship_stage_key(ship_stage(g, idx))), NULL));
        g->sfx = SFX_CRACK;
    }
}

static void sink(Game *g, int idx)
{
    Ship *s = &g->ships[idx];
    if (g->aboard == idx) {
        log_add(ATTR(C_LIGHTRED, C_BLACK), loc_fmt("msg.ship_sank", shipdef_name(s->def), NULL, NULL));
        g->sfx = SFX_SPLASH;
    }
    ship_remove(g, idx, 1);
}

static void move_one(Game *g, int idx, int back)
{
    Ship *s = &g->ships[idx];
    long dx, dz;
    int v = ship_speed(g, idx);
    dir_delta(s->heading, &dx, &dz);
    if (back) {
        dx = -dx;
        dz = -dz;
    }
    if (!fits(g, s->def, s->dim, s->x + dx, s->y, s->z + dz, s->heading, idx)) {
        /* aground or against another hull */
        if (v > 30 && g->mode != GM_CREATIVE && g->mode != GM_SPECTATOR) {
            damage_hull(g, idx, v / 20, SFX_CRACK);
            log_add(ATTR(C_YELLOW, C_BLACK), loc_get("msg.ship_aground"));
        } else if (g->blocked_msg == 0) {
            log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get("msg.ship_blocked"));
            g->blocked_msg = 20;
        }
        s->throttle = 0;
        s->acc = 0;
        return;
    }
    s->x += dx;
    s->z += dz;
    ++s->moved;
    if (g->aboard == idx) {
        const ShipDef *d = &g_ships[s->def];
        g->p.x = s->x;
        g->p.z = s->z;
        (void)d;
        if (g->mode == GM_SURVIVAL && !g->p.god && ship_rowing(g)) {
            g->p.stamina = CL_MAX(0, g->p.stamina - 4 * mx_abs(s->throttle));
        }
        if ((s->moved & 7) == 0) {
            skill_gain(g, SK_SAILING, 1);
        }
        g->p.noise = 30;
    }
}

void ship_tick(Game *g)
{
    int i;
    for (i = 0; i < MAX_SHIPS; ++i) {
        Ship *s = &g->ships[i];
        const ShipDef *d;
        if (!s->active) continue;
        d = &g_ships[s->def];
        if (g->aboard == i) {
            if (g->p.dead || g->p.dim != s->dim) {
                g->aboard = -1;
            } else {
                g->p.x = s->x;
                g->p.y = s->y;
                g->p.z = s->z;
            }
        }
        {
            int v = ship_speed(g, i);
            s->acc = (cl_s16)(s->acc + v);
            while (s->acc >= 100 && s->active && s->throttle != 0) {
                s->acc = (cl_s16)(s->acc - 100);
                move_one(g, i, s->throttle < 0);
            }
            if (v == 0) {
                s->acc = 0;
            }
        }
        if ((g->tick % 20L) != 0) continue;
        /* once per second: the sea takes its toll (only near the player,
         * where the world is simulated) */
        if (s->dim != g->p.dim || mx_abs(s->x - g->p.x) > 128 || mx_abs(s->z - g->p.z) > 128) continue;
        /* creative and spectator: ships take no damage, like the player */
        if (g->mode == GM_CREATIVE || g->mode == GM_SPECTATOR) continue;
        {
            const WeatherDef *w = weather_cur(&g->weather);
            int stage = ship_stage(g, i);
            int over = w->wind - d->wave_limit;
            long load = ship_load(g, i);
            if (stage == SHIP_DAMAGED) s->water = (cl_s16)(s->water + 1);
            if (stage == SHIP_CRITICAL) s->water = (cl_s16)(s->water + 4);
            if (over > 0 && s->dim == g->p.dim && rng_range(&g->rng, 100) < over) {
                damage_hull(g, i, 1, SFX_CREAK);
                s->water = (cl_s16)(s->water + CL_MAX(1, over / 10));
            }
            if (w->precip == PRECIP_RAIN && rng_range(&g->rng, 10) == 0) {
                s->water = (cl_s16)(s->water + 1);
            }
            if (load > d->capacity && rng_range(&g->rng, 4) == 0) {
                s->water = (cl_s16)(s->water + 1);
            }
            if (s->hull <= 0 || s->water >= ship_max_water(g, i) || load + s->water > (long)d->capacity * 3 / 2) {
                sink(g, i);
                continue;
            }
            if (g->aboard == i && ship_flood_pct(g, i) >= 50 && (g->tick % 200L) == 0) {
                log_add(ATTR(C_LIGHTRED, C_BLACK), loc_get("msg.ship_flooding"));
            }
        }
    }
}

/* U aboard: repair with the repair material in hand, otherwise bail water */
int ship_use(Game *g)
{
    Ship *s;
    const ShipDef *d;
    ItemStack *st = inv_selected(&g->inv);
    char num[12];
    if (g->aboard < 0) {
        return 0;
    }
    s = &g->ships[g->aboard];
    d = &g_ships[s->def];
    if (st->item >= 0 && st->item == d->repair_item && s->hull < d->hull) {
        int add = d->hull / 4 + player_skill_level(&g->p, SK_CRAFTING) / 2;
        s->hull = (cl_s16)CL_MIN((int)d->hull, s->hull + add);
        inv_remove_slot(&g->inv, g->inv.selected, 1);
        sprintf(num, "%d", (int)((long)s->hull * 100L / d->hull));
        log_add(ATTR(C_LIGHTGREEN, C_BLACK), loc_fmt("msg.ship_repaired", num, NULL, NULL));
        g->sfx = SFX_PLACE;
        return 1;
    }
    if (s->water <= 0) {
        log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get("msg.ship_dry"));
        return 1;
    }
    if (g->mode == GM_SURVIVAL && !g->p.god) {
        if (g->p.stamina < 20) {
            log_add(ATTR(C_YELLOW, C_BLACK), loc_get("msg.too_tired"));
            return 1;
        }
        g->p.stamina -= 20;
    }
    s->water = (cl_s16)CL_MAX(0, s->water - (st->item >= 0 && st->item == item_find("bucket") ? 30 : 10));
    sprintf(num, "%d", ship_flood_pct(g, g->aboard));
    log_add(ATTR(C_LIGHTCYAN, C_BLACK), loc_fmt("msg.ship_bailed", num, NULL, NULL));
    g->sfx = SFX_SPLASH;
    return 1;
}

int ship_rowing(Game *g)
{
    const Ship *s;
    const ShipDef *d;
    if (g->aboard < 0) {
        return 0;
    }
    s = &g->ships[g->aboard];
    d = &g_ships[s->def];
    return s->throttle != 0 && !s->anchored && d->oar_speed > 0 &&
           (d->sail_speed == 0 || s->throttle < 0 || ship_wind_pct(g, g->aboard) * d->sail_speed < d->oar_speed * 100);
}

/* R aboard: open the cargo hold */
int ship_open_cargo(Game *g)
{
    Ship *s;
    if (g->aboard < 0) {
        return 0;
    }
    s = &g->ships[g->aboard];
    if (s->chest < 0) {
        log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get("msg.ship_no_cargo"));
        return 1;
    }
    g->ui_request = UIREQ_CHEST;
    g->ui_arg = s->chest;
    return 1;
}
