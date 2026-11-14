/* Cliderness - commands/cmd_player.c : player, movement and position commands */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "localization/locale.h"
#include "core/str.h"

static int set_flag(ConCtx *c, int *flag, const char *name)
{
    int v;
    char buf[96];
    if (!con_arg_onoff(c, 1, *flag, &v)) {
        return con_usage(c);
    }
    *flag = v;
    sprintf(buf, "%s: %s", name, loc_get(v ? "con.on" : "con.off"));
    con_good(buf);
    return CMD_OK;
}

static int c_gamemode(ConCtx *c)
{
    int m;
    if (c->argc < 2) {
        con_out(loc_fmt("con.gamemode_is", loc_get(game_mode_key(c->g->mode)), NULL, NULL));
        return CMD_OK;
    }
    m = game_mode_find(c->argv[1]);
    if (m < 0) {
        return con_usage(c);
    }
    game_set_mode(c->g, m);
    con_good(loc_fmt("con.gamemode_set", loc_get(game_mode_key(m)), NULL, NULL));
    return CMD_OK;
}

static int c_god(ConCtx *c) { return set_flag(c, &c->g->p.god, "god"); }
static int c_noclip(ConCtx *c)
{
    int r = set_flag(c, &c->g->p.noclip, "noclip");
    if (c->g->p.noclip) {
        c->g->p.fly = 1;
    }
    return r;
}
static int c_fly(ConCtx *c) { return set_flag(c, &c->g->p.fly, "fly"); }

static int c_heal(ConCtx *c)
{
    long n = c->g->p.hp_max;
    Player *p = &c->g->p;
    if (c->argc >= 2 && (!con_arg_long(c, 1, &n) || n < 0)) {
        return con_usage(c);
    }
    p->hp = (int)CL_MIN((long)p->hp_max, (long)p->hp + n);
    if (p->dead && p->hp > 0) {
        p->dead = 0;
    }
    con_good(loc_get("con.healed"));
    return CMD_OK;
}

static int c_feed(ConCtx *c)
{
    c->g->p.hunger = 1000;
    con_good(loc_get("con.fed"));
    return CMD_OK;
}

static int c_hydrate(ConCtx *c)
{
    c->g->p.thirst = 1000;
    con_good(loc_get("con.hydrated"));
    return CMD_OK;
}

static int c_rest(ConCtx *c)
{
    c->g->p.fatigue = 0;
    c->g->p.stamina = c->g->p.stamina_max;
    con_good(loc_get("con.rested"));
    return CMD_OK;
}

/* "<set|get> [value]" with percent values on a 0..1000 meter */
static int meter(ConCtx *c, int *v, int scale, int lo, int hi, const char *name)
{
    char buf[96];
    long n;
    if (c->argc < 2 || str_ieq(c->argv[1], "get")) {
        sprintf(buf, "%s = %d", name, *v / scale);
        con_out(buf);
        return CMD_OK;
    }
    if (!str_ieq(c->argv[1], "set") || !con_arg_long(c, 2, &n) || n < lo || n > hi) {
        return con_usage(c);
    }
    *v = (int)n * scale;
    sprintf(buf, "%s = %ld", name, n);
    con_good(buf);
    return CMD_OK;
}

static int c_stamina(ConCtx *c) { return meter(c, &c->g->p.stamina, 10, 0, 100, "stamina"); }
static int c_hunger(ConCtx *c) { return meter(c, &c->g->p.hunger, 10, 0, 100, "hunger"); }
static int c_thirst(ConCtx *c) { return meter(c, &c->g->p.thirst, 10, 0, 100, "thirst"); }
static int c_fatigue(ConCtx *c) { return meter(c, &c->g->p.fatigue, 10, 0, 100, "fatigue"); }

static int c_health(ConCtx *c)
{
    int r = meter(c, &c->g->p.hp, 1, 0, 1000, "health");
    if (c->g->p.hp > c->g->p.hp_max) {
        c->g->p.hp_max = c->g->p.hp;
    }
    if (c->g->p.hp > 0) {
        c->g->p.dead = 0;
    }
    return r;
}

static int c_temperature(ConCtx *c)
{
    char buf[64];
    long n;
    Player *p = &c->g->p;
    if (c->argc < 2 || str_ieq(c->argv[1], "get")) {
        sprintf(buf, "body %d.%d C, environment %d C", p->body_temp / 10, p->body_temp % 10, p->env_temp);
        con_out(buf);
        return CMD_OK;
    }
    if (!str_ieq(c->argv[1], "set") || !con_arg_long(c, 2, &n) || n < 30 || n > 42) {
        return con_usage(c);
    }
    p->body_temp = (int)n * 10;
    con_good("ok");
    return CMD_OK;
}

static int c_attribute(ConCtx *c)
{
    int st;
    long n;
    char buf[96];
    if (c->argc < 3) {
        return con_usage(c);
    }
    st = player_find_stat(c->argv[2]);
    if (st < 0) {
        con_err(loc_fmt("con.unknown_stat", c->argv[2], NULL, NULL));
        return CMD_ERR;
    }
    if (str_ieq(c->argv[1], "get")) {
        sprintf(buf, "%s = %d", c->argv[2], c->g->p.stats[st]);
        con_out(buf);
        return CMD_OK;
    }
    if (!str_ieq(c->argv[1], "set") || !con_arg_long(c, 3, &n) || n < 1 || n > 100) {
        return con_usage(c);
    }
    c->g->p.stats[st] = (int)n;
    if (st == ST_CON) {
        c->g->p.hp_max = 80 + (int)n * 2;
        if (c->g->p.hp > c->g->p.hp_max) {
            c->g->p.hp = c->g->p.hp_max;
        }
    }
    sprintf(buf, "%s = %ld", c->argv[2], n);
    con_good(buf);
    return CMD_OK;
}

static int c_skill(ConCtx *c)
{
    int sk;
    long n;
    char buf[96];
    if (c->argc < 3) {
        return con_usage(c);
    }
    sk = player_find_skill(c->argv[2]);
    if (sk < 0) {
        con_err(loc_fmt("con.unknown_skill", c->argv[2], NULL, NULL));
        return CMD_ERR;
    }
    if (str_ieq(c->argv[1], "get")) {
        sprintf(buf, "%s = %d (%ld xp)", c->argv[2], player_skill_level(&c->g->p, sk), c->g->p.skill_xp[sk]);
        con_out(buf);
        return CMD_OK;
    }
    if (!str_ieq(c->argv[1], "set") || !con_arg_long(c, 3, &n) || n < 0 || n > SKILL_MAX_LEVEL) {
        return con_usage(c);
    }
    c->g->p.skill_xp[sk] = player_level_xp((int)n);
    sprintf(buf, "%s = %ld", c->argv[2], n);
    con_good(buf);
    return CMD_OK;
}

static int c_kill(ConCtx *c)
{
    const char *sel = c->argc >= 2 ? c->argv[1] : "@s";
    char buf[64];
    if (str_ieq(sel, "@s") || str_ieq(sel, "@p") || str_ieq(sel, "@a")) {
        int god = c->g->p.god;
        int mode = c->g->mode;
        c->g->p.god = 0;
        c->g->mode = GM_SURVIVAL;
        surv_damage(c->g, c->g->p.hp + 1000, "cause.command");
        c->g->p.god = god;
        c->g->mode = mode;
        return CMD_OK;
    }
    if (str_ieq(sel, "@e") || str_ieq(sel, "@e[type=item]") || str_ieq(sel, "@e[type=cliderness:item]")) {
        sprintf(buf, "%d", ent_kill_all(c->g));
        con_good(loc_fmt("con.killed", buf, NULL, NULL));
        return CMD_OK;
    }
    if (str_iprefix(sel, "@e[type=")) {
        con_err(loc_get("con.no_such_entity"));
        return CMD_ERR;
    }
    return con_usage(c);
}

static int c_respawn(ConCtx *c)
{
    game_respawn(c->g);
    return CMD_OK;
}

static int c_spawnpoint(ConCtx *c)
{
    Player *p = &c->g->p;
    p->spawn_x = p->x;
    p->spawn_y = p->y;
    p->spawn_z = p->z;
    con_good(loc_get("con.spawn_set"));
    return CMD_OK;
}

static int c_crouch(ConCtx *c)
{
    game_toggle_crouch(c->g);
    return CMD_OK;
}

static int c_tp(ConCtx *c)
{
    long x, y, z;
    Player *p = &c->g->p;
    char buf[80];
    if (c->argc == 2 && str_ieq(c->argv[1], "spawn")) {
        x = p->spawn_x;
        z = p->spawn_z;
        y = world_top(&c->g->world, p->dim, x, z, WORLD_H - 1) + 1;
    } else if (c->argc < 4 || !con_arg_coord(c, 1, p->x, &x) || !con_arg_coord(c, 2, p->y, &y) ||
               !con_arg_coord(c, 3, p->z, &z)) {
        return con_usage(c);
    }
    if (y < 1 || y >= WORLD_H - 1) {
        con_err(loc_get("con.bad_height"));
        return CMD_ERR;
    }
    game_teleport(c->g, x, y, z);
    sprintf(buf, "-> %ld %ld %ld", x, y, z);
    con_good(buf);
    return CMD_OK;
}

static int c_pos(ConCtx *c)
{
    char buf[96];
    Player *p = &c->g->p;
    sprintf(buf, "x=%ld y=%ld z=%ld  chunk %ld,%ld", p->x, p->y, p->z,
            p->x >= 0 ? p->x / CHUNK_SIZE : -((-p->x + CHUNK_SIZE - 1) / CHUNK_SIZE),
            p->z >= 0 ? p->z / CHUNK_SIZE : -((-p->z + CHUNK_SIZE - 1) / CHUNK_SIZE));
    con_out(buf);
    return CMD_OK;
}

static int c_top(ConCtx *c)
{
    Player *p = &c->g->p;
    long y = world_top(&c->g->world, p->dim, p->x, p->z, WORLD_H - 2) + 1;
    game_teleport(c->g, p->x, y, p->z);
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_up(ConCtx *c)
{
    long n;
    Player *p = &c->g->p;
    int glass = block_find("glass");
    if (!con_arg_long(c, 1, &n) || n < 1 || n > 100) {
        return con_usage(c);
    }
    if (p->y + n >= WORLD_H - 1) {
        con_err(loc_get("con.bad_height"));
        return CMD_ERR;
    }
    game_teleport(c->g, p->x, p->y + n, p->z);
    if (glass > 0 && !blk_solid(world_get(&c->g->world, p->dim, p->x, p->y - 1, p->z))) {
        world_set(&c->g->world, p->dim, p->x, p->y - 1, p->z, (cl_u8)glass, META_PLACED);
    }
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_spawn(ConCtx *c)
{
    Player *p = &c->g->p;
    long y = world_top(&c->g->world, p->dim, p->spawn_x, p->spawn_z, WORLD_H - 2) + 1;
    game_teleport(c->g, p->spawn_x, y, p->spawn_z);
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static int c_stats(ConCtx *c)
{
    char buf[128];
    Player *p = &c->g->p;
    sprintf(buf, "STR %d DEX %d INT %d PER %d CON %d END %d WIL %d LCK %d",
            p->stats[0], p->stats[1], p->stats[2], p->stats[3], p->stats[4], p->stats[5],
            p->stats[6], p->stats[7]);
    con_out(buf);
    sprintf(buf, "HP %d/%d  stamina %d%%  hunger %d%%  thirst %d%%  fatigue %d%%", p->hp, p->hp_max,
            p->stamina / 10, p->hunger / 10, p->thirst / 10, p->fatigue / 10);
    con_out(buf);
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "gamemode", "gamemode", "<survival|creative|adventure|spectator>", "g", NULL, PERM_CHEAT, c_gamemode, "gm" },
    { "god", "player", "[on|off]", "o", NULL, PERM_CHEAT, c_god, NULL },
    { "noclip", "player", "[on|off]", "o", NULL, PERM_CHEAT, c_noclip, NULL },
    { "fly", "player", "[on|off]", "o", NULL, PERM_CHEAT, c_fly, NULL },
    { "heal", "player", "[amount]", "n", NULL, PERM_CHEAT, c_heal, NULL },
    { "feed", "player", "", "", NULL, PERM_CHEAT, c_feed, NULL },
    { "hydrate", "player", "", "", NULL, PERM_CHEAT, c_hydrate, "drink" },
    { "rest", "player", "", "", NULL, PERM_CHEAT, c_rest, NULL },
    { "stamina", "player", "<set|get> [0-100]", "xn", "set|get", PERM_CHEAT, c_stamina, NULL },
    { "health", "player", "<set|get> [hp]", "xn", "set|get", PERM_CHEAT, c_health, "hp" },
    { "hunger", "player", "<set|get> [0-100]", "xn", "set|get", PERM_CHEAT, c_hunger, NULL },
    { "thirst", "player", "<set|get> [0-100]", "xn", "set|get", PERM_CHEAT, c_thirst, NULL },
    { "fatigue", "player", "<set|get> [0-100]", "xn", "set|get", PERM_CHEAT, c_fatigue, NULL },
    { "temperature", "player", "<set|get> [30-42]", "xn", "set|get", PERM_CHEAT, c_temperature, "temp" },
    { "attribute", "player", "<set|get> <stat> [1-100]", "xsn", "set|get", PERM_CHEAT, c_attribute, "attr" },
    { "skill", "player", "<set|get> <skill> [0-100]", "xkn", "set|get", PERM_CHEAT, c_skill, NULL },
    { "kill", "player", "[@s|@e|@e[type=item]]", "@", NULL, PERM_CHEAT, c_kill, NULL },
    { "respawn", "player", "", "", NULL, PERM_ANY, c_respawn, NULL },
    { "spawnpoint", "position", "", "", NULL, PERM_CHEAT, c_spawnpoint, "setspawn" },
    { "crouch", "movement", "", "", NULL, PERM_ANY, c_crouch, "sneak" },
    { "tp", "position", "<x> <y> <z> | spawn", "nnn", NULL, PERM_CHEAT, c_tp, "teleport" },
    { "pos", "position", "", "", NULL, PERM_ANY, c_pos, "where,coords" },
    { "top", "position", "", "", NULL, PERM_CHEAT, c_top, NULL },
    { "up", "position", "<blocks>", "n", NULL, PERM_CHEAT, c_up, NULL },
    { "spawn", "position", "", "", NULL, PERM_CHEAT, c_spawn, "home" },
    { "stats", "player", "", "", NULL, PERM_ANY, c_stats, NULL }
};

CommandTable cmd_table_player(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
