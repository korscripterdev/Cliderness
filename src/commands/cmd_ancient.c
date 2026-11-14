/* Cliderness - commands/cmd_ancient.c : dimension and portal commands */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "world/biomes/biome.h"
#include "world/structures/structure.h"
#include "gameplay/entities/creature.h"
#include "localization/locale.h"
#include "core/str.h"

static int c_dimension(ConCtx *c)
{
    char buf[128];
    Player *p = &c->g->p;
    sprintf(buf, "%s (%d) %ld,%ld,%ld", loc_get(dim_key(p->dim)), p->dim, p->x, p->y, p->z);
    con_good(buf);
    sprintf(buf, "%s: %s", loc_get("con.respawn_in"), loc_get(dim_key(p->spawn_dim)));
    con_out(buf);
    return CMD_OK;
}

static int c_dimensions(ConCtx *c)
{
    int d, i;
    char buf[160];
    (void)c;
    for (d = DIM_OVERWORLD; d <= DIM_ANCIENT; ++d) {
        int nb = 0, nc = 0, ns = 0;
        for (i = 0; i < g_biome_count; ++i) {
            if ((g_biomes[i].kind == BK_ANCIENT) == (d == DIM_ANCIENT)) ++nb;
        }
        for (i = 0; i < g_creature_count; ++i) {
            if (g_creatures[i].dim == d) ++nc;
        }
        for (i = 0; i < g_struct_count; ++i) {
            if (g_structs[i].dim == d) ++ns;
        }
        sprintf(buf, "%d %-12.12s %s %2d  %s %2d  %s %2d", d, loc_get(dim_key(d)), loc_get("con.biomes"), nb,
                loc_get("con.creatures"), nc, loc_get("con.structures"), ns);
        con_out(buf);
    }
    return CMD_OK;
}

static int c_travel(ConCtx *c)
{
    int d;
    if (c->argc < 2) {
        return con_usage(c);
    }
    d = dim_find(c->argv[1]);
    if (d < 0) {
        return con_usage(c);
    }
    if (d == c->g->p.dim) {
        con_err(loc_get("con.same_dim"));
        return CMD_ERR;
    }
    portal_travel(c->g, d, c->argc >= 3 && str_ieq(c->argv[2], "portal"));
    return CMD_OK;
}

static int c_makeportal(ConCtx *c)
{
    long dx, dz;
    Player *p = &c->g->p;
    int pb = block_find("ancient_portal");
    dir_delta(p->facing, &dx, &dz);
    if (pb < 0 || blk_solid(world_get(&c->g->world, p->dim, p->x + dx, p->y, p->z + dz))) {
        con_err(loc_get("msg.no_room"));
        return CMD_ERR;
    }
    world_set(&c->g->world, p->dim, p->x + dx, p->y, p->z + dz, (cl_u8)pb, META_PLACED);
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_ancientbiomes(ConCtx *c)
{
    int i;
    char buf[128];
    (void)c;
    for (i = 0; i < g_biome_count; ++i) {
        if (g_biomes[i].kind == BK_ANCIENT) {
            sprintf(buf, "%-18.18s %.40s", g_biomes[i].id, biome_name(i));
            con_out(buf);
        }
    }
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "dimension", "world", "", "", NULL, PERM_ANY, c_dimension, "dim" },
    { "dimensions", "world", "", "", NULL, PERM_ANY, c_dimensions, NULL },
    { "travel", "world", "<overworld|ancient> [portal]", "xx", "overworld|ancient", PERM_CHEAT, c_travel, "gotodim" },
    { "makeportal", "world", "", "", NULL, PERM_CHEAT, c_makeportal, NULL },
    { "ancientbiomes", "world", "", "", NULL, PERM_ANY, c_ancientbiomes, NULL }
};

CommandTable cmd_table_ancient(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
