/* Cliderness - commands/cmd_fluid.c : ClideFluid and structure commands */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "world/structures/structure.h"
#include "localization/locale.h"
#include "core/str.h"

static int c_fluid(ConCtx *c)
{
    FluidSim *f = &c->g->world.fluid;
    char buf[128];
    if (c->argc >= 2) {
        int v;
        if (!con_arg_onoff(c, 1, f->enabled, &v)) {
            return con_usage(c);
        }
        f->enabled = v;
    }
    sprintf(buf, "ClideFluid %s: active %ld, budget %ld/step, processed %ld, dropped %ld",
            loc_get(f->enabled ? "con.on" : "con.off"), f->count, f->budget, f->processed, f->dropped);
    con_out(buf);
    return CMD_OK;
}

static int put_fluid(ConCtx *c, int block)
{
    long lvl = 15, dx, dz;
    Player *p = &c->g->p;
    if (c->argc >= 2 && (!con_arg_long(c, 1, &lvl) || lvl < 1 || lvl > 15)) {
        return con_usage(c);
    }
    dir_delta(p->facing, &dx, &dz);
    if (blk_solid(world_get(&c->g->world, p->dim, p->x + dx, p->y, p->z + dz))) {
        con_err(loc_get("msg.no_room"));
        return CMD_ERR;
    }
    fluid_put(&c->g->world, p->dim, p->x + dx, p->y, p->z + dz, block, (int)lvl);
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_water(ConCtx *c) { return put_fluid(c, g_cb.water); }
static int c_lava(ConCtx *c) { return put_fluid(c, g_cb.lava); }

static int c_settle(ConCtx *c)
{
    long steps = 200, i, n = 0;
    char buf[64];
    if (c->argc >= 2 && (!con_arg_long(c, 1, &steps) || steps < 1 || steps > 20000)) {
        return con_usage(c);
    }
    for (i = 0; i < steps && c->g->world.fluid.count > 0; ++i) {
        n += fluid_step(&c->g->world);
    }
    sprintf(buf, "%ld steps, %ld cells, %ld still active", i, n, c->g->world.fluid.count);
    con_good(buf);
    return CMD_OK;
}

static int c_volume(ConCtx *c)
{
    long r = 8;
    char buf[96];
    Player *p = &c->g->p;
    long wv, lv;
    if (c->argc >= 2 && (!con_arg_long(c, 1, &r) || r < 1 || r > 32)) {
        return con_usage(c);
    }
    wv = fluid_volume(&c->g->world, p->dim, g_cb.water, p->x - r, p->y - r, p->z - r, p->x + r, p->y + r, p->z + r);
    lv = fluid_volume(&c->g->world, p->dim, g_cb.lava, p->x - r, p->y - r, p->z - r, p->x + r, p->y + r, p->z + r);
    sprintf(buf, "%s %ld, %s %ld (1 block = 15)", block_name(g_cb.water), wv, block_name(g_cb.lava), lv);
    con_out(buf);
    return CMD_OK;
}

static int c_structures(ConCtx *c)
{
    int i;
    char buf[160];
    (void)c;
    for (i = 0; i < g_struct_count; ++i) {
        sprintf(buf, "%-16s %3d%%", g_structs[i].id, g_structs[i].rarity);
        con_out(buf);
    }
    return CMD_OK;
}

static int c_findstructure(ConCtx *c)
{
    int s;
    long x, y, z;
    char buf[128];
    if (c->argc < 2) {
        return con_usage(c);
    }
    s = struct_find(c->argv[1]);
    if (s < 0) {
        con_err(loc_fmt("con.unknown_structure", c->argv[1], NULL, NULL));
        return CMD_ERR;
    }
    if (g_structs[s].dim != c->g->p.dim ||
        !structure_locate(c->g->seed, s, c->g->p.x, c->g->p.z, &x, &y, &z)) {
        con_err(loc_get("con.not_found"));
        return CMD_ERR;
    }
    sprintf(buf, "%s: x=%ld y=%ld z=%ld", struct_name(s), x, y, z);
    con_good(buf);
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "fluid", "fluid", "[on|off]", "o", NULL, PERM_ANY, c_fluid, NULL },
    { "water", "fluid", "[level 1-15]", "n", NULL, PERM_CHEAT, c_water, NULL },
    { "lava", "fluid", "[level 1-15]", "n", NULL, PERM_CHEAT, c_lava, NULL },
    { "settle", "fluid", "[steps]", "n", NULL, PERM_CHEAT, c_settle, NULL },
    { "volume", "fluid", "[radius]", "n", NULL, PERM_ANY, c_volume, NULL },
    { "structures", "world", "", "", NULL, PERM_ANY, c_structures, NULL },
    { "findstructure", "world", "<structure>", "", NULL, PERM_CHEAT, c_findstructure, "locatestructure" }
};

CommandTable cmd_table_fluid(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
