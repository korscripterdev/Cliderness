/* Cliderness - commands/cmd_mob.c : creature, combat and difficulty commands */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "content/effects.h"
#include "gameplay/entities/creature.h"
#include "localization/locale.h"
#include "core/mathx.h"
#include "core/str.h"

static int c_summon(ConCtx *c)
{
    int def, k, n = 0;
    long count = 1, dx, dz;
    Player *p = &c->g->p;
    char buf[64];
    if (c->argc < 2) {
        return con_usage(c);
    }
    def = creature_find(c->argv[1]);
    if (def < 0) {
        con_err(loc_fmt("con.unknown_creature", c->argv[1], NULL, NULL));
        return CMD_ERR;
    }
    if (c->argc >= 3 && (!con_arg_long(c, 2, &count) || count < 1 || count > 16)) {
        return con_usage(c);
    }
    dir_delta(p->facing, &dx, &dz);
    for (k = 0; k < count; ++k) {
        long x = p->x + dx * (1 + k / 3) + (k % 3 == 1 ? dz : k % 3 == 2 ? -dz : 0);
        long z = p->z + dz * (1 + k / 3) + (k % 3 == 1 ? dx : k % 3 == 2 ? -dx : 0);
        long y = p->y;
        if (g_creatures[def].aquatic) {
            y = p->y;
        } else {
            while (y > 1 && !blk_solid(world_get(&c->g->world, p->dim, x, y - 1, z))) --y;
            while (y < WORLD_H - 2 && blk_solid(world_get(&c->g->world, p->dim, x, y, z))) ++y;
        }
        if (mob_spawn(c->g, def, p->dim, x, y, z) >= 0) {
            ++n;
        }
    }
    if (n == 0) {
        con_err(loc_get("con.mob_limit"));
        return CMD_ERR;
    }
    c->g->cheated = 1;
    sprintf(buf, "%d", n);
    con_good(loc_fmt("con.summoned", creature_name(def), buf, NULL));
    return CMD_OK;
}

static int c_creatures(ConCtx *c)
{
    static const char *const kinds[3] = { "mk.animal", "mk.monster", "mk.npc" };
    int i;
    char buf[160];
    for (i = 0; i < g_creature_count; ++i) {
        const CreatureDef *d = &g_creatures[i];
        if (c->argc >= 2 && !str_ieq(c->argv[1], loc_get(kinds[d->kind])) &&
            !str_ieq(c->argv[1], kinds[d->kind] + 3)) {
            continue;
        }
        sprintf(buf, "%-14.14s %-10.10s hp%3d dmg%2d  %.40s", d->id, loc_get(kinds[d->kind]), d->hp, d->damage,
                creature_name(i));
        con_out(buf);
    }
    return CMD_OK;
}

static int c_mobs(ConCtx *c)
{
    int i, n = 0;
    char buf[160];
    Player *p = &c->g->p;
    for (i = 0; i < MAX_MOBS; ++i) {
        const Mob *m = &c->g->mobs[i];
        long dx, dz;
        if (!m->active) continue;
        dx = m->x - p->x;
        dz = m->z - p->z;
        sprintf(buf, "#%-2d %-14.14s %3d/%-3d %-10.10s %ld,%ld,%ld (%ldm)", i, g_creatures[m->def].id, m->hp,
                g_creatures[m->def].hp, loc_get(mob_state_key(m->state)), m->x, m->y, m->z,
                mx_isqrt(dx * dx + dz * dz));
        con_out(buf);
        ++n;
    }
    sprintf(buf, "%d/%d", n, MAX_MOBS);
    con_good(buf);
    return CMD_OK;
}

static int c_difficulty(ConCtx *c)
{
    if (c->argc >= 2) {
        int d = difficulty_find(c->argv[1]);
        if (d < 0) {
            return con_usage(c);
        }
        c->g->difficulty = d;
        if (d == DIFF_PEACEFUL) {
            int i;
            for (i = 0; i < MAX_MOBS; ++i) {
                if (c->g->mobs[i].active && g_creatures[c->g->mobs[i].def].kind == MK_MONSTER) {
                    c->g->mobs[i].active = 0;
                }
            }
        }
    }
    con_good(loc_fmt("con.difficulty", loc_get(difficulty_key(c->g->difficulty)), NULL, NULL));
    return CMD_OK;
}

static int c_spawning(ConCtx *c)
{
    int v;
    if (c->argc >= 2) {
        if (!con_arg_onoff(c, 1, c->g->spawning, &v)) {
            return con_usage(c);
        }
        c->g->spawning = v;
    }
    con_out(loc_fmt("con.spawning", loc_get(c->g->spawning ? "con.on" : "con.off"), NULL, NULL));
    return CMD_OK;
}

static int c_despawn(ConCtx *c)
{
    int def = -1;
    char buf[16];
    if (c->argc >= 2 && !str_ieq(c->argv[1], "all")) {
        def = creature_find(c->argv[1]);
        if (def < 0) {
            con_err(loc_fmt("con.unknown_creature", c->argv[1], NULL, NULL));
            return CMD_ERR;
        }
    }
    sprintf(buf, "%d", mob_kill_type(c->g, def));
    con_good(loc_fmt("con.killed", buf, NULL, NULL));
    return CMD_OK;
}

static int c_bestiary(ConCtx *c)
{
    int def;
    char buf[160];
    const CreatureDef *d;
    static const char *const agg[4] = { "agg.never", "agg.defensive", "agg.night", "agg.always" };
    if (c->argc < 2) {
        return con_usage(c);
    }
    def = creature_find(c->argv[1]);
    if (def < 0) {
        con_err(loc_fmt("con.unknown_creature", c->argv[1], NULL, NULL));
        return CMD_ERR;
    }
    d = &g_creatures[def];
    con_good(creature_name(def));
    sprintf(buf, "HP %d  DMG %d  DEF %d  speed %d  sight %d  hearing %d  xp %d", d->hp, d->damage,
            d->defense, d->speed, d->sight, d->hearing, d->xp);
    con_out(buf);
    con_out(loc_get(agg[d->aggression & 3]));
    if (d->burns) {
        con_out(loc_get("con.burns_daylight"));
    }
    if (d->effect >= 0) {
        sprintf(buf, "%s %d%%", effect_name(d->effect), d->effect_chance);
        con_out(buf);
    }
    if (d->ntrades) {
        int k;
        for (k = 0; k < d->ntrades; ++k) {
            char n1[40], n2[40];
            str_copy(n1, item_name(d->trade_in[k]), sizeof(n1));
            str_copy(n2, item_name(d->trade_out[k]), sizeof(n2));
            sprintf(buf, "  %s x%d -> %s x%d", n1, d->trade_in_n[k], n2, d->trade_out_n[k]);
            con_out(buf);
        }
    }
    return CMD_OK;
}

static int c_attack(ConCtx *c)
{
    if (!game_attack(c->g)) {
        con_err(loc_get("con.no_target"));
        return CMD_ERR;
    }
    return CMD_OK;
}

static int c_trade(ConCtx *c)
{
    long dx, dz;
    Player *p = &c->g->p;
    dir_delta(p->facing, &dx, &dz);
    if (!ent_interact(c->g, p->x + dx, p->y, p->z + dz)) {
        con_err(loc_get("con.no_target"));
        return CMD_ERR;
    }
    return CMD_OK;
}

static int c_threat(ConCtx *c)
{
    con_out(loc_get(ent_hostile_near(c->g, 16) ? "con.threat_yes" : "con.threat_no"));
    return CMD_OK;
}

static int c_kills(ConCtx *c)
{
    char buf[24];
    sprintf(buf, "%ld", c->g->kills);
    con_out(loc_fmt("con.kills", buf, NULL, NULL));
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "summon", "creatures", "<creature> [count]", "*n", NULL, PERM_CHEAT, c_summon, NULL },
    { "creatures", "creatures", "[animal|monster|npc]", "x", "animal|monster|npc", PERM_ANY, c_creatures, NULL },
    { "mobs", "creatures", "", "", NULL, PERM_ANY, c_mobs, "nearby" },
    { "difficulty", "world", "[peaceful|easy|normal|hard]", "x", "peaceful|easy|normal|hard", PERM_ANY,
      c_difficulty, NULL },
    { "spawning", "creatures", "[on|off]", "o", NULL, PERM_CHEAT, c_spawning, "mobspawning" },
    { "despawn", "creatures", "[creature|all]", "*", NULL, PERM_CHEAT, c_despawn, "butcher" },
    { "bestiary", "creatures", "<creature>", "*", NULL, PERM_ANY, c_bestiary, "mobinfo" },
    { "attack", "combat", "", "", NULL, PERM_ANY, c_attack, NULL },
    { "trade", "creatures", "", "", NULL, PERM_ANY, c_trade, NULL },
    { "threat", "combat", "", "", NULL, PERM_ANY, c_threat, NULL },
    { "kills", "combat", "", "", NULL, PERM_ANY, c_kills, NULL }
};

CommandTable cmd_table_mob(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
