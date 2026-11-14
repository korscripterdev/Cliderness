/*
 * Cliderness - commands/cmd_act.c : player actions, inventory and storage,
 * waypoints, quick world controls, interface and system commands.
 * Action commands do exactly what the matching key does (accessibility,
 * scripting and play without a keyboard layout).
 */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "content/effects.h"
#include "world/weather/weather.h"
#include "world/sky/sky.h"
#include "gameplay/entities/creature.h"
#include "gameplay/vehicles/ship.h"
#include "input/keybind.h"
#include "help/help.h"
#include "audio/audio.h"
#include "renderer/fb.h"
#include "renderer/cp437.h"
#include "localization/locale.h"
#include "core/log.h"
#include "core/mathx.h"
#include "core/str.h"
#include "core/version.h"

static int dir_arg(const char *s)
{
    static const char *const d[4] = { "n", "e", "s", "w" };
    static const char *const l[4] = { "north", "east", "south", "west" };
    int i;
    for (i = 0; i < 4; ++i) {
        if (str_ieq(s, d[i]) || str_ieq(s, l[i])) return i;
    }
    return -1;
}

static int facing_chest(ConCtx *c)
{
    long dx, dz;
    Player *p = &c->g->p;
    dir_delta(p->facing, &dx, &dz);
    return chest_find(c->g, p->dim, p->x + dx, p->y, p->z + dz);
}

/* ------------------------------------------------------------ actions */

static int c_mine(ConCtx *c) { game_mine(c->g); return CMD_OK; }
static int c_place(ConCtx *c) { game_place(c->g); return CMD_OK; }
static int c_use(ConCtx *c) { game_use(c->g); return CMD_OK; }
static int c_interact(ConCtx *c) { game_interact(c->g); return CMD_OK; }

static int c_drop(ConCtx *c)
{
    game_drop(c->g, c->g->inv.selected, c->argc >= 2 && str_ieq(c->argv[1], "all"));
    return CMD_OK;
}

static int c_face(ConCtx *c)
{
    int d;
    if (c->argc < 2 || (d = dir_arg(c->argv[1])) < 0) return con_usage(c);
    if (c->g->aboard >= 0) {
        con_err(loc_get("con.on_ship"));
        return CMD_ERR;
    }
    c->g->p.facing = d;
    return CMD_OK;
}

static int c_step(ConCtx *c)
{
    int d;
    if (c->argc < 2 || (d = dir_arg(c->argv[1])) < 0) return con_usage(c);
    game_request_move(c->g, d, c->argc >= 3 && str_ieq(c->argv[2], "sprint"));
    return CMD_OK;
}

static int c_unstuck(ConCtx *c)
{
    Player *p = &c->g->p;
    long y;
    if (!blk_solid(world_get(&c->g->world, p->dim, p->x, p->y, p->z)) &&
        !blk_solid(world_get(&c->g->world, p->dim, p->x, p->y + 1, p->z))) {
        con_out(loc_get("con.not_stuck"));
        return CMD_OK;
    }
    for (y = p->y + 1; y < WORLD_H - 2; ++y) {
        if (!blk_solid(world_get(&c->g->world, p->dim, p->x, y, p->z)) &&
            !blk_solid(world_get(&c->g->world, p->dim, p->x, y + 1, p->z))) {
            p->y = y;
            con_good(loc_get("con.done"));
            return CMD_OK;
        }
    }
    return CMD_ERR;
}

/* eat the most nourishing food in the inventory */
static int c_eat(ConCtx *c)
{
    int k, best = -1, val = 0, keep = c->g->inv.selected;
    for (k = 0; k < INV_SLOTS; ++k) {
        int it = c->g->inv.s[k].item;
        if (it >= 0 && g_items[it].type == ITEM_FOOD && g_items[it].food > val &&
            (g_items[it].effect < 0 || g_items[it].effect == effect_find("well_fed"))) {
            val = g_items[it].food;
            best = k;
        }
    }
    if (best < 0) {
        con_err(loc_get("con.no_food"));
        return CMD_ERR;
    }
    c->g->inv.selected = best;
    game_use(c->g);
    c->g->inv.selected = keep;
    return CMD_OK;
}

/* use a medicine that cures one of the active effects */
static int c_treat(ConCtx *c)
{
    int k, keep = c->g->inv.selected;
    for (k = 0; k < INV_SLOTS; ++k) {
        int it = c->g->inv.s[k].item;
        if (it >= 0 && g_items[it].cure >= 0 && fx_has(c->g, g_items[it].cure)) {
            c->g->inv.selected = k;
            game_use(c->g);
            c->g->inv.selected = keep;
            return CMD_OK;
        }
    }
    con_err(loc_get("con.no_cure"));
    return CMD_ERR;
}

/* wear the best armour piece for every slot */
static int c_equipbest(ConCtx *c)
{
    int eq, k, n = 0;
    for (eq = 1; eq <= EQ_COUNT; ++eq) {
        int best = -1, cur = c->g->inv.equip[eq - 1].item, score = cur >= 0 ? g_items[cur].defense : -1;
        for (k = 0; k < INV_SLOTS; ++k) {
            int it = c->g->inv.s[k].item;
            if (it >= 0 && g_items[it].equip == eq && g_items[it].defense > score) {
                score = g_items[it].defense;
                best = k;
            }
        }
        if (best >= 0 && inv_equip(&c->g->inv, best)) ++n;
    }
    {
        char b[16];
        sprintf(b, "%d", n);
        con_good(loc_fmt("con.equipped_n", b, NULL, NULL));
    }
    return CMD_OK;
}

/* ------------------------------------------------------------ inventory */

static int c_sort(ConCtx *c)
{
    inv_sort(&c->g->inv);
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_stack(ConCtx *c)
{
    int a, b, n = 0;
    Inventory *inv = &c->g->inv;
    for (a = 0; a < INV_SLOTS; ++a) {
        for (b = a + 1; b < INV_SLOTS && inv->s[a].item >= 0; ++b) {
            if (inv->s[b].item == inv->s[a].item && g_items[inv->s[a].item].durability == 0) {
                int room = g_items[inv->s[a].item].max_stack - inv->s[a].count;
                int mv = CL_MIN(room, (int)inv->s[b].count);
                if (mv > 0) {
                    inv->s[a].count = (cl_s16)(inv->s[a].count + mv);
                    inv->s[b].count = (cl_s16)(inv->s[b].count - mv);
                    if (inv->s[b].count == 0) inv->s[b].item = -1;
                    ++n;
                }
            }
        }
    }
    {
        char buf[16];
        sprintf(buf, "%d", n);
        con_good(loc_fmt("con.merged", buf, NULL, NULL));
    }
    return CMD_OK;
}

static int c_swap(ConCtx *c)
{
    long a, b;
    if (c->argc < 3 || !con_arg_long(c, 1, &a) || !con_arg_long(c, 2, &b) || a < 1 || b < 1 || a > INV_SLOTS ||
        b > INV_SLOTS) {
        return con_usage(c);
    }
    inv_swap(&c->g->inv, (int)a - 1, (int)b - 1);
    return CMD_OK;
}

/* move items between the inventory and a chest (or the cargo hold) */
static int transfer(ConCtx *c, int chest, int to_chest)
{
    int item = -1, k, moved = 0;
    Chest *ch;
    char buf[32];
    if (chest < 0) {
        con_err(loc_get("con.no_chest"));
        return CMD_ERR;
    }
    ch = &c->g->chests[chest];
    if (c->argc >= 2 && !str_ieq(c->argv[1], "all") && (item = item_find(c->argv[1])) < 0) {
        return con_usage(c);
    }
    if (to_chest) {
        for (k = 0; k < INV_SLOTS; ++k) {
            ItemStack *st = &c->g->inv.s[k];
            int j;
            if (st->item < 0 || (item >= 0 && st->item != item)) continue;
            for (j = 0; j < CHEST_SLOTS && st->item >= 0; ++j) {
                ItemStack *d = &ch->s[j];
                if (d->item < 0) {
                    *d = *st;
                    moved += st->count;
                    st->item = -1;
                    st->count = 0;
                } else if (d->item == st->item && g_items[d->item].durability == 0 && d->count < g_items[d->item].max_stack) {
                    int mv = CL_MIN((int)(g_items[d->item].max_stack - d->count), (int)st->count);
                    d->count = (cl_s16)(d->count + mv);
                    st->count = (cl_s16)(st->count - mv);
                    moved += mv;
                    if (st->count == 0) st->item = -1;
                }
            }
        }
    } else {
        for (k = 0; k < CHEST_SLOTS; ++k) {
            ItemStack *st = &ch->s[k];
            int left;
            if (st->item < 0 || (item >= 0 && st->item != item)) continue;
            left = inv_add(&c->g->inv, st->item, st->count, st->dur);
            moved += st->count - left;
            st->count = (cl_s16)left;
            if (left == 0) st->item = -1;
        }
    }
    sprintf(buf, "%d", moved);
    con_good(loc_fmt("con.moved", buf, NULL, NULL));
    return CMD_OK;
}

static int c_deposit(ConCtx *c) { return transfer(c, facing_chest(c), 1); }
static int c_withdraw(ConCtx *c) { return transfer(c, facing_chest(c), 0); }

static int hold(ConCtx *c)
{
    if (c->g->aboard < 0) {
        con_err(loc_get("con.not_aboard"));
        return -2;
    }
    return c->g->ships[c->g->aboard].chest;
}

static int c_loadcargo(ConCtx *c)
{
    int h = hold(c);
    return h == -2 ? CMD_ERR : transfer(c, h, 1);
}

static int c_unload(ConCtx *c)
{
    int h = hold(c);
    return h == -2 ? CMD_ERR : transfer(c, h, 0);
}

static int c_chestinfo(ConCtx *c)
{
    int ch = facing_chest(c), k, n = 0;
    char buf[96];
    if (ch < 0) {
        con_err(loc_get("con.no_chest"));
        return CMD_ERR;
    }
    for (k = 0; k < CHEST_SLOTS; ++k) {
        const ItemStack *st = &c->g->chests[ch].s[k];
        if (st->item >= 0) {
            sprintf(buf, "%2d %-24.40s x%d", k + 1, item_name(st->item), st->count);
            con_out(buf);
            ++n;
        }
    }
    if (!n) con_out("-");
    return CMD_OK;
}

static int c_emptychest(ConCtx *c)
{
    int ch = facing_chest(c), k;
    if (ch < 0) {
        con_err(loc_get("con.no_chest"));
        return CMD_ERR;
    }
    for (k = 0; k < CHEST_SLOTS; ++k) {
        c->g->chests[ch].s[k].item = -1;
        c->g->chests[ch].s[k].count = 0;
    }
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_repairall(ConCtx *c)
{
    int k, n = 0;
    for (k = 0; k < INV_SLOTS; ++k) {
        ItemStack *st = &c->g->inv.s[k];
        if (st->item >= 0 && g_items[st->item].durability > 0 && st->dur < g_items[st->item].durability) {
            st->dur = g_items[st->item].durability;
            ++n;
        }
    }
    for (k = 0; k < INV_EQUIP; ++k) {
        ItemStack *st = &c->g->inv.equip[k];
        if (st->item >= 0 && g_items[st->item].durability > 0) {
            st->dur = g_items[st->item].durability;
            ++n;
        }
    }
    {
        char b[16];
        sprintf(b, "%d", n);
        con_good(loc_fmt("con.repaired_n", b, NULL, NULL));
    }
    return CMD_OK;
}

static int c_maxskills(ConCtx *c)
{
    int k;
    long lvl = 50;
    if (c->argc >= 2 && (!con_arg_long(c, 1, &lvl) || lvl < 0 || lvl > SKILL_MAX_LEVEL)) return con_usage(c);
    for (k = 0; k < SK_COUNT; ++k) {
        c->g->p.skill_xp[k] = player_level_xp((int)lvl);
    }
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_resetskills(ConCtx *c)
{
    int k;
    for (k = 0; k < SK_COUNT; ++k) c->g->p.skill_xp[k] = 0;
    con_good(loc_get("con.done"));
    return CMD_OK;
}

/* ------------------------------------------------------------ ships */

static int c_stop(ConCtx *c)
{
    if (c->g->aboard < 0) {
        con_err(loc_get("con.not_aboard"));
        return CMD_ERR;
    }
    c->g->ships[c->g->aboard].throttle = 0;
    return CMD_OK;
}

static int c_turn(ConCtx *c)
{
    int d;
    if (c->g->aboard < 0) {
        con_err(loc_get("con.not_aboard"));
        return CMD_ERR;
    }
    if (c->argc < 2 || (d = dir_arg(c->argv[1])) < 0) return con_usage(c);
    if (d == c->g->ships[c->g->aboard].heading || d == ((c->g->ships[c->g->aboard].heading + 2) & 3)) {
        con_err(loc_get("msg.ship_no_turn"));
        return CMD_ERR;
    }
    ship_steer(c->g, d);
    return c->g->ships[c->g->aboard].heading == d ? CMD_OK : CMD_ERR;
}

/* ------------------------------------------------------------ creatures and world */

static int c_ai(ConCtx *c)
{
    int v;
    if (c->argc >= 2) {
        if (!con_arg_onoff(c, 1, !c->g->ai_off, &v)) return con_usage(c);
        c->g->ai_off = !v;
    }
    con_out(loc_fmt("con.ai_state", loc_get(c->g->ai_off ? "con.off" : "con.on"), NULL, NULL));
    return CMD_OK;
}

static int c_killhostile(ConCtx *c)
{
    int i, n = 0;
    char b[16];
    for (i = 0; i < MAX_MOBS; ++i) {
        Mob *m = &c->g->mobs[i];
        if (m->active && (g_creatures[m->def].kind == MK_MONSTER || m->aggro)) {
            m->active = 0;
            ++n;
        }
    }
    sprintf(b, "%d", n);
    con_good(loc_fmt("con.killed", b, NULL, NULL));
    return CMD_OK;
}

static int c_mobcap(ConCtx *c)
{
    char b[64];
    sprintf(b, "%d / %d (%s)", mob_count(c->g), MAX_MOBS, loc_get(c->g->spawning ? "con.on" : "con.off"));
    con_out(b);
    return CMD_OK;
}

static int c_lightning(ConCtx *c)
{
    c->g->weather.flash = 6;
    c->g->sfx = SFX_THUNDER;
    return CMD_OK;
}

static int set_hour(ConCtx *c, int hour)
{
    ctime_set_time_of_day(&c->g->time, (long)hour * 3600L);
    c->g->last_minute = ctime_total_minutes(&c->g->time);
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_dawn(ConCtx *c) { return set_hour(c, 6); }
static int c_noon(ConCtx *c) { return set_hour(c, 12); }
static int c_dusk(ConCtx *c) { return set_hour(c, 19); }
static int c_midnight(ConCtx *c) { return set_hour(c, 0); }

static int force_weather(ConCtx *c, const char *id)
{
    int w = weather_find(id);
    if (w < 0 || (c->g->p.dim == DIM_ANCIENT && g_weather[w].ancient == 0)) {
        w = weather_find(c->g->p.dim == DIM_ANCIENT ? "ancient_storm" : id);
    }
    weather_force(&c->g->weather, w, 60);
    con_good(weather_name(w));
    return CMD_OK;
}

static int c_storm(ConCtx *c) { return force_weather(c, "thunderstorm"); }
static int c_calm(ConCtx *c) { return force_weather(c, "clear"); }

static int c_fluidinfo(ConCtx *c)
{
    long dx, dz;
    Player *p = &c->g->p;
    char b[64];
    cl_u8 bl;
    long y = p->y;
    dir_delta(p->facing, &dx, &dz);
    bl = world_get(&c->g->world, p->dim, p->x + dx, y, p->z + dz);
    if (!blk_liquid(bl)) {
        --y;
        bl = world_get(&c->g->world, p->dim, p->x + dx, y, p->z + dz);
    }
    if (!blk_liquid(bl)) {
        con_out("-");
        return CMD_OK;
    }
    sprintf(b, "%s %d/15", block_name(bl), fluid_level(&c->g->world, p->dim, p->x + dx, y, p->z + dz));
    con_out(b);
    return CMD_OK;
}

static int c_sponge(ConCtx *c)
{
    long r = 4, x, y, z, n = 0;
    Player *p = &c->g->p;
    char b[24];
    if (c->argc >= 2 && (!con_arg_long(c, 1, &r) || r < 1 || r > 8)) return con_usage(c);
    edit_begin(&c->g->journal);
    for (y = p->y - r; y <= p->y + r; ++y)
        for (z = p->z - r; z <= p->z + r; ++z)
            for (x = p->x - r; x <= p->x + r; ++x)
                if (blk_liquid(world_get(&c->g->world, p->dim, x, y, z))) {
                    edit_set(c->g, p->dim, x, y, z, BLOCK_AIR, 0);
                    ++n;
                }
    edit_end(&c->g->journal);
    sprintf(b, "%ld", n);
    con_good(loc_fmt("con.changed", b, NULL, NULL));
    return CMD_OK;
}

static int c_constellations(ConCtx *c)
{
    int i;
    char name[48];
    for (i = 0; i < 12; ++i) {
        con_out(sky_constellation_name(c->g->seed, i, name, sizeof(name)));
    }
    return CMD_OK;
}

static int c_deathpoint(ConCtx *c)
{
    char b[96];
    if (!c->g->deaths) {
        con_out("-");
        return CMD_OK;
    }
    sprintf(b, "%ld, %ld, %ld (%s), %ld", c->g->death_x, c->g->death_y, c->g->death_z,
            loc_get(dim_key(c->g->death_dim)), c->g->deaths);
    con_out(b);
    return CMD_OK;
}

/* ------------------------------------------------------------ waypoints */

static int wp_find(Game *g, const char *name)
{
    int i;
    for (i = 0; i < MAX_WAYPOINTS; ++i) {
        if (g->wp[i].active && str_ieq(g->wp[i].name, name)) return i;
    }
    return -1;
}

static int c_waypoint(ConCtx *c)
{
    Game *g = c->g;
    int i;
    char b[96];
    if (c->argc < 2 || str_ieq(c->argv[1], "list")) {
        int n = 0;
        for (i = 0; i < MAX_WAYPOINTS; ++i) {
            if (!g->wp[i].active) continue;
            sprintf(b, "%-15s %ld, %ld, %ld (%s)", g->wp[i].name, g->wp[i].x, g->wp[i].y, g->wp[i].z,
                    loc_get(dim_key(g->wp[i].dim)));
            con_out(b);
            ++n;
        }
        if (!n) con_out("-");
        return CMD_OK;
    }
    if (str_ieq(c->argv[1], "clear")) {
        for (i = 0; i < MAX_WAYPOINTS; ++i) g->wp[i].active = 0;
        return CMD_OK;
    }
    if (c->argc < 3) return con_usage(c);
    if (str_ieq(c->argv[1], "add") || str_ieq(c->argv[1], "set")) {
        i = wp_find(g, c->argv[2]);
        if (i < 0) {
            for (i = 0; i < MAX_WAYPOINTS && g->wp[i].active; ++i) {
            }
        }
        if (i >= MAX_WAYPOINTS) {
            con_err(loc_get("con.wp_full"));
            return CMD_ERR;
        }
        str_copy(g->wp[i].name, c->argv[2], sizeof(g->wp[i].name));
        g->wp[i].x = g->p.x;
        g->wp[i].y = g->p.y;
        g->wp[i].z = g->p.z;
        g->wp[i].dim = (cl_u8)g->p.dim;
        g->wp[i].active = 1;
        g->unsaved = 1;
        con_good(loc_fmt("con.wp_set", g->wp[i].name, NULL, NULL));
        return CMD_OK;
    }
    if (str_ieq(c->argv[1], "remove") || str_ieq(c->argv[1], "del")) {
        i = wp_find(g, c->argv[2]);
        if (i < 0) {
            con_err(loc_fmt("con.wp_unknown", c->argv[2], NULL, NULL));
            return CMD_ERR;
        }
        g->wp[i].active = 0;
        return CMD_OK;
    }
    return con_usage(c);
}

static int c_navigate(ConCtx *c)
{
    int i;
    long dx, dz;
    char b[128];
    if (c->argc < 2) return con_usage(c);
    i = wp_find(c->g, c->argv[1]);
    if (i < 0) {
        con_err(loc_fmt("con.wp_unknown", c->argv[1], NULL, NULL));
        return CMD_ERR;
    }
    if (c->g->wp[i].dim != c->g->p.dim) {
        con_out(loc_get(dim_key(c->g->wp[i].dim)));
        return CMD_OK;
    }
    dx = c->g->wp[i].x - c->g->p.x;
    dz = c->g->wp[i].z - c->g->p.z;
    sprintf(b, "%s: %ld m, x %+ld z %+ld, y %+ld", c->g->wp[i].name, mx_isqrt(dx * dx + dz * dz), dx, dz,
            c->g->wp[i].y - c->g->p.y);
    con_good(b);
    return CMD_OK;
}

static int c_gowaypoint(ConCtx *c)
{
    int i;
    if (c->argc < 2) return con_usage(c);
    i = wp_find(c->g, c->argv[1]);
    if (i < 0) {
        con_err(loc_fmt("con.wp_unknown", c->argv[1], NULL, NULL));
        return CMD_ERR;
    }
    game_teleport(c->g, c->g->wp[i].x, c->g->wp[i].y, c->g->wp[i].z);
    c->g->p.dim = c->g->wp[i].dim;
    return CMD_OK;
}

/* ------------------------------------------------------------ interface */

static int request(int req, int arg)
{
    g_con_request = req;
    g_con_arg = arg;
    return CMD_OK;
}

static int c_sky(ConCtx *c) { (void)c; return request(CON_REQ_SKY, 0); }
static int c_openinv(ConCtx *c) { (void)c; return request(CON_REQ_INVENTORY, 0); }
static int c_opencraft(ConCtx *c) { (void)c; return request(CON_REQ_CRAFT, 0); }
static int c_openstats(ConCtx *c) { (void)c; return request(CON_REQ_STATS, 0); }

static int c_panel(ConCtx *c)
{
    long n;
    if (c->argc < 2 || !con_arg_long(c, 1, &n) || n < 1 || n > 3) return con_usage(c);
    return request(CON_REQ_PANEL, (int)n - 1);
}

static int c_log(ConCtx *c)
{
    long n = 10, i;
    if (c->argc >= 2 && (!con_arg_long(c, 1, &n) || n < 1 || n > 40)) return con_usage(c);
    for (i = CL_MIN(n, (long)log_count()) - 1; i >= 0; --i) {
        const LogEntry *e = log_get((int)i);
        if (e) con_out(e->text);
    }
    return CMD_OK;
}

static int c_say(ConCtx *c)
{
    char buf[CON_LINE];
    int i;
    if (c->argc < 2) return con_usage(c);
    buf[0] = '\0';
    for (i = 1; i < c->argc; ++i) {
        if (i > 1) str_cat(buf, " ", sizeof(buf));
        str_cat(buf, c->argv[i], sizeof(buf));
    }
    log_add(ATTR(C_WHITE, C_BLACK), buf);
    return CMD_OK;
}

static int c_history(ConCtx *c)
{
    int i, n = con_history_count();
    (void)c;
    for (i = n - 1; i >= 0; --i) con_out(con_history_get(i));
    return CMD_OK;
}

static int c_aliases(ConCtx *c)
{
    int i;
    char buf[160];
    (void)c;
    for (i = 0; i < con_count(); ++i) {
        const Command *cmd = con_get(i);
        if (cmd->aliases) {
            sprintf(buf, "/%-14s %.120s", cmd->name, cmd->aliases);
            con_out(buf);
        }
    }
    return CMD_OK;
}

static int c_keys(ConCtx *c)
{
    int a;
    char buf[128], k[48];
    (void)c;
    for (a = 0; a < ACT_COUNT; ++a) {
        kb_describe(a, k, sizeof(k));
        sprintf(buf, "%-24.48s %s", loc_get(kb_action_key(a)), k);
        con_out(buf);
    }
    return CMD_OK;
}

static int c_topics(ConCtx *c)
{
    int i;
    char buf[96];
    (void)c;
    for (i = 0; i < g_help.count; ++i) {
        sprintf(buf, "%-16s %d %.60s", g_help.t[i].id, g_help.t[i].level, g_help.t[i].title);
        con_out(buf);
    }
    return CMD_OK;
}

static int c_helpsearch(ConCtx *c)
{
    int i, n = 0;
    char buf[96];
    if (c->argc < 2) return con_usage(c);
    for (i = 0; i < g_help.count; ++i) {
        if (help_matches(i, c->argv[1])) {
            sprintf(buf, "%-16s %.60s", g_help.t[i].id, g_help.t[i].title);
            con_out(buf);
            ++n;
        }
    }
    if (!n) con_out("-");
    return CMD_OK;
}

/* one contextual tip for the current situation */
static int c_hint(ConCtx *c)
{
    Game *g = c->g;
    Player *p = &g->p;
    const char *k = "hint.explore";
    if (p->hp * 3 < p->hp_max) k = "hint.low_hp";
    else if (p->hunger < 250) k = "hint.hungry";
    else if (p->thirst < 250) k = "hint.thirsty";
    else if (p->body_temp < 355) k = "hint.cold";
    else if (ent_hostile_near(g, 12)) k = "hint.danger";
    else if (g->aboard >= 0 && ship_flood_pct(g, g->aboard) > 30) k = "hint.flooding";
    else if (ctime_daylight(&g->time) < 20 && !game_is_covered(g, p->x, p->y, p->z)) k = "hint.night";
    else if (inv_count(&g->inv, item_find("oak_planks")) == 0 && inv_count(&g->inv, item_find("oak_log")) == 0) k = "hint.wood";
    con_good(loc_get(k));
    return CMD_OK;
}

static int c_tick(ConCtx *c)
{
    char b[64];
    sprintf(b, "tick %ld, %d TPS, %ld ms/tick", c->g->tick, TICKS_PER_SECOND, TICK_MS);
    con_out(b);
    return CMD_OK;
}

static int c_platform(ConCtx *c)
{
    con_out(c->plat ? c->plat->name : "?");
    return CMD_OK;
}

static int c_audio(ConCtx *c)
{
    char b[128];
    (void)c;
    sprintf(b, "%s, %s, %s %ld", aud_backend(), loc_get(aud_enabled() ? "con.on" : "con.off"),
            aud_pcm() ? "PCM" : "tones", aud_played());
    con_out(b);
    return CMD_OK;
}

static int c_sounds(ConCtx *c)
{
    int i;
    (void)c;
    for (i = 1; i < SFX_COUNT; ++i) con_out(aud_sfx_name(i));
    return CMD_OK;
}

static int c_playsound(ConCtx *c)
{
    int s;
    if (c->argc < 2 || (s = aud_find(c->argv[1])) < 0) return con_usage(c);
    c->g->sfx = s;
    return CMD_OK;
}

static int c_sound(ConCtx *c)
{
    int v = aud_enabled();
    if (c->argc >= 2 && !con_arg_onoff(c, 1, v, &v)) return con_usage(c);
    aud_set_enabled(v);
    con_out(loc_get(v ? "con.on" : "con.off"));
    return CMD_OK;
}

static int c_license(ConCtx *c)
{
    (void)c;
    con_out(CLIDE_LICENSE);
    con_out("This program comes with ABSOLUTELY NO WARRANTY. See LICENSE.");
    return CMD_OK;
}

static int c_creator(ConCtx *c)
{
    (void)c;
    con_good(CLIDE_CREATOR);
    return CMD_OK;
}

static int c_colors(ConCtx *c)
{
    int i;
    (void)c;
    for (i = 0; i < 16; ++i) {
        char b[32];
        sprintf(b, "%2d %s", i, color_name(i));
        log_add(ATTR(i, i == 0 ? C_LIGHTGRAY : C_BLACK), b);
    }
    return CMD_OK;
}

static int c_glyphs(ConCtx *c)
{
    int row, col;
    (void)c;
    for (row = 2; row < 16; ++row) {
        char line[160];
        int o = 0;
        o += sprintf(line, "%02X ", row * 16);
        for (col = 0; col < 16; ++col) {
            o += utf8_encode(cp437_to_unicode(row * 16 + col), line + o);
            line[o++] = ' ';
        }
        line[o] = '\0';
        con_out(line);
    }
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "mine", "actions", "", "", NULL, PERM_ANY, c_mine, "dig" },
    { "place", "actions", "", "", NULL, PERM_ANY, c_place, NULL },
    { "use", "actions", "", "", NULL, PERM_ANY, c_use, NULL },
    { "interact", "actions", "", "", NULL, PERM_ANY, c_interact, "e" },
    { "drop", "actions", "[all]", "x", "all", PERM_ANY, c_drop, NULL },
    { "face", "actions", "<n|e|s|w>", "x", "n|e|s|w", PERM_ANY, c_face, "turnto" },
    { "step", "actions", "<n|e|s|w> [sprint]", "xx", "n|e|s|w", PERM_ANY, c_step, "walk" },
    { "unstuck", "position", "", "", NULL, PERM_ANY, c_unstuck, NULL },
    { "eat", "actions", "", "", NULL, PERM_ANY, c_eat, NULL },
    { "treat", "actions", "", "", NULL, PERM_ANY, c_treat, "medicine" },
    { "equipbest", "equipment", "", "", NULL, PERM_ANY, c_equipbest, NULL },
    { "sort", "items", "", "", NULL, PERM_ANY, c_sort, NULL },
    { "stack", "items", "", "", NULL, PERM_ANY, c_stack, "merge" },
    { "swap", "items", "<slot> <slot>", "nn", NULL, PERM_ANY, c_swap, NULL },
    { "deposit", "items", "[item|all]", "i", NULL, PERM_ANY, c_deposit, "store" },
    { "withdraw", "items", "[item|all]", "i", NULL, PERM_ANY, c_withdraw, "take" },
    { "loadcargo", "ships", "[item|all]", "i", NULL, PERM_ANY, c_loadcargo, NULL },
    { "unload", "ships", "[item|all]", "i", NULL, PERM_ANY, c_unload, NULL },
    { "chestinfo", "items", "", "", NULL, PERM_ANY, c_chestinfo, NULL },
    { "emptychest", "items", "", "", NULL, PERM_CHEAT, c_emptychest, NULL },
    { "repairall", "items", "", "", NULL, PERM_CHEAT, c_repairall, NULL },
    { "maxskills", "player", "[level]", "n", NULL, PERM_CHEAT, c_maxskills, NULL },
    { "resetskills", "player", "", "", NULL, PERM_CHEAT, c_resetskills, NULL },
    { "stop", "ships", "", "", NULL, PERM_ANY, c_stop, NULL },
    { "turn", "ships", "<n|e|s|w>", "x", "n|e|s|w", PERM_ANY, c_turn, "steer" },
    { "ai", "creatures", "[on|off]", "o", NULL, PERM_CHEAT, c_ai, NULL },
    { "killhostile", "creatures", "", "", NULL, PERM_CHEAT, c_killhostile, NULL },
    { "mobcap", "creatures", "", "", NULL, PERM_ANY, c_mobcap, NULL },
    { "lightning", "weather", "", "", NULL, PERM_CHEAT, c_lightning, NULL },
    { "dawn", "time", "", "", NULL, PERM_CHEAT, c_dawn, "morning" },
    { "noon", "time", "", "", NULL, PERM_CHEAT, c_noon, NULL },
    { "dusk", "time", "", "", NULL, PERM_CHEAT, c_dusk, "evening" },
    { "midnight", "time", "", "", NULL, PERM_CHEAT, c_midnight, NULL },
    { "storm", "weather", "", "", NULL, PERM_CHEAT, c_storm, NULL },
    { "calm", "weather", "", "", NULL, PERM_CHEAT, c_calm, NULL },
    { "fluidinfo", "fluid", "", "", NULL, PERM_ANY, c_fluidinfo, NULL },
    { "sponge", "fluid", "[radius 1-8]", "n", NULL, PERM_CHEAT, c_sponge, NULL },
    { "constellations", "time", "", "", NULL, PERM_ANY, c_constellations, "stars" },
    { "deathpoint", "position", "", "", NULL, PERM_ANY, c_deathpoint, NULL },
    { "waypoint", "position", "<add|remove|list|clear> [name]", "x*", "add|remove|list|clear", PERM_ANY, c_waypoint, "wp" },
    { "navigate", "position", "<waypoint>", "*", NULL, PERM_ANY, c_navigate, "nav" },
    { "gowaypoint", "position", "<waypoint>", "*", NULL, PERM_CHEAT, c_gowaypoint, "warp" },
    { "sky", "interface", "", "", NULL, PERM_ANY, c_sky, NULL },
    { "openinv", "interface", "", "", NULL, PERM_ANY, c_openinv, NULL },
    { "opencraft", "interface", "", "", NULL, PERM_ANY, c_opencraft, NULL },
    { "openstats", "interface", "", "", NULL, PERM_ANY, c_openstats, NULL },
    { "panel", "interface", "<1-3>", "n", NULL, PERM_ANY, c_panel, NULL },
    { "log", "interface", "[lines]", "n", NULL, PERM_ANY, c_log, NULL },
    { "say", "interface", "<text>", "*", NULL, PERM_ANY, c_say, "note" },
    { "history", "interface", "", "", NULL, PERM_ANY, c_history, NULL },
    { "aliases", "system", "", "", NULL, PERM_ANY, c_aliases, NULL },
    { "keys", "interface", "", "", NULL, PERM_ANY, c_keys, "bindings" },
    { "topics", "help", "", "", NULL, PERM_ANY, c_topics, NULL },
    { "helpsearch", "help", "<text>", "*", NULL, PERM_ANY, c_helpsearch, NULL },
    { "hint", "help", "", "", NULL, PERM_ANY, c_hint, "tip" },
    { "tick", "system", "", "", NULL, PERM_ANY, c_tick, "tps" },
    { "osinfo", "system", "", "", NULL, PERM_ANY, c_platform, "sysinfo" },
    { "audio", "system", "", "", NULL, PERM_ANY, c_audio, NULL },
    { "sounds", "system", "", "", NULL, PERM_ANY, c_sounds, NULL },
    { "playsound", "system", "<effect>", "*", NULL, PERM_ANY, c_playsound, NULL },
    { "sound", "system", "[on|off]", "o", NULL, PERM_ANY, c_sound, "mute" },
    { "license", "system", "", "", NULL, PERM_ANY, c_license, NULL },
    { "creator", "system", "", "", NULL, PERM_ANY, c_creator, "author" },
    { "colors", "system", "", "", NULL, PERM_ANY, c_colors, "colours" },
    { "glyphs", "system", "", "", NULL, PERM_ANY, c_glyphs, "cp437" }
};

CommandTable cmd_table_act(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
