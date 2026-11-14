/* Cliderness - commands/cmd_ship.c : ship, sailing and wind commands */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "gameplay/vehicles/ship.h"
#include "world/weather/weather.h"
#include "localization/locale.h"
#include "core/mathx.h"
#include "core/str.h"

static int need_aboard(ConCtx *c)
{
    if (c->g->aboard < 0) {
        con_err(loc_get("con.not_aboard"));
        return 0;
    }
    return 1;
}

static int c_ships(ConCtx *c)
{
    int i;
    char buf[160];
    (void)c;
    for (i = 0; i < g_ship_count; ++i) {
        const ShipDef *d = &g_ships[i];
        sprintf(buf, "%-11.11s hull %3d  oars %3d  sails %3d  %4d kg  waves %2d%%  %.30s", d->id, d->hull,
                d->oar_speed, d->sail_speed, d->capacity, d->wave_limit, shipdef_name(i));
        con_out(buf);
    }
    return CMD_OK;
}

static int c_boats(ConCtx *c)
{
    int i, n = 0;
    char buf[160];
    for (i = 0; i < MAX_SHIPS; ++i) {
        const Ship *s = &c->g->ships[i];
        if (!s->active) continue;
        sprintf(buf, "#%-2d %-11.11s %ld,%ld,%ld  hull %d/%d  %s  water %d%%%s", i, g_ships[s->def].id, s->x, s->y,
                s->z, s->hull, g_ships[s->def].hull, loc_get(ship_stage_key(ship_stage(c->g, i))),
                ship_flood_pct(c->g, i), c->g->aboard == i ? "  *" : "");
        con_out(buf);
        ++n;
    }
    sprintf(buf, "%d/%d", n, MAX_SHIPS);
    con_good(buf);
    return CMD_OK;
}

static int c_board(ConCtx *c)
{
    if (c->g->aboard >= 0) {
        con_err(loc_get("con.already_aboard"));
        return CMD_ERR;
    }
    if (!ship_board(c->g)) {
        con_err(loc_get("con.no_ship"));
        return CMD_ERR;
    }
    return CMD_OK;
}

static int c_disembark(ConCtx *c)
{
    if (!need_aboard(c)) return CMD_ERR;
    ship_disembark(c->g);
    return c->g->aboard < 0 ? CMD_OK : CMD_ERR;
}

static int c_throttle(ConCtx *c)
{
    long t;
    Ship *s;
    if (!need_aboard(c)) return CMD_ERR;
    s = &c->g->ships[c->g->aboard];
    if (c->argc < 2 || !con_arg_long(c, 1, &t) || t < -1 || t > 3) {
        return con_usage(c);
    }
    if (t < 0 && g_ships[s->def].oar_speed == 0) {
        con_err(loc_get("con.no_oars"));
        return CMD_ERR;
    }
    s->throttle = (signed char)t;
    s->anchored = 0;
    return CMD_OK;
}

static int c_anchor(ConCtx *c)
{
    Ship *s;
    int v;
    if (!need_aboard(c)) return CMD_ERR;
    s = &c->g->ships[c->g->aboard];
    v = !s->anchored;
    if (c->argc >= 2 && !con_arg_onoff(c, 1, s->anchored, &v)) {
        return con_usage(c);
    }
    s->anchored = (cl_u8)v;
    if (v) s->throttle = 0;
    con_good(loc_get(v ? "msg.anchor_down" : "msg.anchor_up"));
    return CMD_OK;
}

static int c_shipinfo(ConCtx *c)
{
    int i = c->g->aboard;
    char buf[160];
    const Ship *s;
    const ShipDef *d;
    static const char *const dirs[4] = { "N", "E", "S", "W" };
    if (i < 0) {
        long dx, dz;
        dir_delta(c->g->p.facing, &dx, &dz);
        i = ship_at(c->g, c->g->p.dim, c->g->p.x + dx, c->g->p.y, c->g->p.z + dz);
    }
    if (i < 0) {
        con_err(loc_get("con.no_ship"));
        return CMD_ERR;
    }
    s = &c->g->ships[i];
    d = &g_ships[s->def];
    con_good(shipdef_name(s->def));
    sprintf(buf, "%s %d/%d (%s)", loc_get("ship.hull"), s->hull, d->hull, loc_get(ship_stage_key(ship_stage(c->g, i))));
    con_out(buf);
    sprintf(buf, "%s %d/%ld l (%d%%)", loc_get("ship.water"), s->water, ship_max_water(c->g, i), ship_flood_pct(c->g, i));
    con_out(buf);
    sprintf(buf, "%s %ld/%d kg", loc_get("ship.load"), ship_load(c->g, i), d->capacity);
    con_out(buf);
    sprintf(buf, "%s %s  %s %d  %s %d", loc_get("ship.heading"), dirs[s->heading & 3], loc_get("ship.throttle"),
            (int)s->throttle, loc_get("ship.speed"), ship_speed(c->g, i));
    con_out(buf);
    sprintf(buf, "%s %d%%  %s %ld", loc_get("ship.wind"), ship_wind_pct(c->g, i), loc_get("ship.travelled"), s->moved);
    con_out(buf);
    sprintf(buf, "%s %d/%d  %s %d", loc_get("ship.crew"), (int)s->crew, (int)d->berths, loc_get("ship.drift"),
            ship_drift_speed(c->g, i));
    con_out(buf);
    if (s->anchored) con_out(loc_get("msg.anchor_down"));
    return CMD_OK;
}

static int c_bail(ConCtx *c)
{
    ItemStack *st;
    int item;
    if (!need_aboard(c)) return CMD_ERR;
    /* bail even when holding the repair material */
    st = inv_selected(&c->g->inv);
    item = st->item;
    st->item = -1;
    ship_use(c->g);
    st->item = (cl_s16)item;
    return CMD_OK;
}

static int c_hull(ConCtx *c)
{
    Ship *s;
    long v;
    if (!need_aboard(c)) return CMD_ERR;
    s = &c->g->ships[c->g->aboard];
    if (c->argc < 3 || !str_ieq(c->argv[1], "set") || !con_arg_long(c, 2, &v) || v < 0 || v > 100) {
        return con_usage(c);
    }
    s->hull = (cl_s16)((long)g_ships[s->def].hull * v / 100L);
    c->g->cheated = 1;
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_spawnship(ConCtx *c)
{
    int def, idx;
    ItemStack *st;
    ItemStack keep;
    if (c->argc < 2) {
        return con_usage(c);
    }
    def = shipdef_find(c->argv[1]);
    if (def < 0) {
        con_err(loc_fmt("con.unknown_ship", c->argv[1], NULL, NULL));
        return CMD_ERR;
    }
    /* place it like the player would, from a temporary item in hand */
    st = inv_selected(&c->g->inv);
    keep = *st;
    st->item = g_ships[def].item;
    st->count = 1;
    st->dur = g_ships[def].hull;
    idx = ship_count(c->g);
    ship_place(c->g);
    if (ship_count(c->g) == idx) {
        *st = keep;
        return CMD_ERR;
    }
    *st = keep;
    c->g->cheated = 1;
    return CMD_OK;
}

static int c_sinkship(ConCtx *c)
{
    int i = c->g->aboard;
    if (i < 0) {
        con_err(loc_get("con.not_aboard"));
        return CMD_ERR;
    }
    c->g->ships[i].water = (cl_s16)ship_max_water(c->g, i);
    c->g->cheated = 1;
    return CMD_OK;
}

static int c_wind(ConCtx *c)
{
    static const char *const names[8] = { "E", "SE", "S", "SW", "W", "NW", "N", "NE" };
    char buf[96];
    WeatherState *w = &c->g->weather;
    if (c->argc >= 2) {
        static const char *const dirs[8] = { "e", "se", "s", "sw", "w", "nw", "n", "ne" };
        int k;
        long a = -1;
        if (!c->g->cheats) {
            con_err(loc_get("con.no_cheats"));
            return CMD_DENIED;
        }
        for (k = 0; k < 8; ++k) {
            if (str_ieq(c->argv[1], dirs[k])) a = k * 32L;
        }
        if (a < 0 && (!con_arg_long(c, 1, &a) || a < 0 || a > 255)) {
            return con_usage(c);
        }
        w->wind_dir = (int)a;
        c->g->cheated = 1;
    }
    sprintf(buf, "%s %d%%, %s %s (%d)", loc_get("ship.wind"), weather_cur(w)->wind, loc_get("ship.wind_to"),
            names[((w->wind_dir + 16) / 32) & 7], w->wind_dir);
    con_out(buf);
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "ships", "ships", "", "", NULL, PERM_ANY, c_ships, NULL },
    { "boats", "ships", "", "", NULL, PERM_ANY, c_boats, "fleet" },
    { "board", "ships", "", "", NULL, PERM_ANY, c_board, "embark" },
    { "disembark", "ships", "", "", NULL, PERM_ANY, c_disembark, "leaveship" },
    { "throttle", "ships", "<-1..3>", "n", NULL, PERM_ANY, c_throttle, "sail" },
    { "anchor", "ships", "[on|off]", "o", NULL, PERM_ANY, c_anchor, NULL },
    { "shipinfo", "ships", "", "", NULL, PERM_ANY, c_shipinfo, "hullinfo" },
    { "bail", "ships", "", "", NULL, PERM_ANY, c_bail, NULL },
    { "hull", "ships", "set <0-100>", "xn", "set", PERM_CHEAT, c_hull, NULL },
    { "spawnship", "ships", "<ship>", "*", NULL, PERM_CHEAT, c_spawnship, NULL },
    { "sinkship", "ships", "", "", NULL, PERM_CHEAT, c_sinkship, NULL },
    { "wind", "weather", "[direction|0-255]", "x", "n|ne|e|se|s|sw|w|nw", PERM_ANY, c_wind, NULL }
};

CommandTable cmd_table_ship(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
