/* Cliderness - commands/cmd_system.c : help, system, settings and debug */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "help/help.h"
#include "localization/locale.h"
#include "core/log.h"
#include "core/mem.h"
#include "core/str.h"
#include "core/version.h"

static int c_help(ConCtx *c)
{
    char buf[160];
    long page = 1;
    int i, per = 8, n = con_count();
    if (c->argc >= 2 && !str_to_long(c->argv[1], &page)) {
        const Command *cmd = con_find(c->argv[1]);
        if (!cmd) {
            con_err(loc_fmt("con.unknown", c->argv[1], NULL, NULL));
            return CMD_ERR;
        }
        sprintf(buf, "/%s %s", cmd->name, cmd->syntax ? cmd->syntax : "");
        con_good(buf);
        con_out(con_desc(cmd));
        if (cmd->aliases) {
            str_copy(buf, loc_get("con.aliases"), sizeof(buf));
            str_cat(buf, " ", sizeof(buf));
            str_cat(buf, cmd->aliases, sizeof(buf));
            con_out(buf);
        }
        sprintf(buf, "%s: %s  |  %s", loc_get("con.category"), cmd->category,
                loc_get(cmd->perm == PERM_CHEAT ? "con.perm_cheat" : "con.perm_any"));
        con_out(buf);
        return CMD_OK;
    }
    if (page < 1 || (page - 1) * per >= n) {
        page = 1;
    }
    sprintf(buf, "%s (%ld/%d)", loc_get("con.help_header"), page, (n + per - 1) / per);
    con_good(buf);
    for (i = (int)(page - 1) * per; i < n && i < (int)page * per; ++i) {
        const Command *cmd = con_get(i);
        char d[80];
        str_copy(d, con_desc(cmd), sizeof(d));
        sprintf(buf, "/%-11s %s", cmd->name, d);
        con_out(buf);
    }
    return CMD_OK;
}

static int c_commands(ConCtx *c)
{
    static const char *const cats[] = {
        "player", "movement", "position", "gamemode", "item", "equipment", "block", "building",
        "world", "terrain", "weather", "time", "monster", "npc", "fluid", "ancient", "vehicle",
        "durability", "debug", "system", "settings"
    };
    char buf[200];
    int i, k;
    (void)c;
    sprintf(buf, "%d", con_count());
    con_good(loc_fmt("con.command_count", buf, NULL, NULL));
    sprintf(buf, "%d", con_alias_count());
    con_out(loc_fmt("con.alias_count", buf, NULL, NULL));
    buf[0] = '\0';
    for (k = 0; k < (int)CL_COUNTOF(cats); ++k) {
        int n = 0;
        char part[32];
        for (i = 0; i < con_count(); ++i) {
            if (str_ieq(con_get(i)->category, cats[k])) {
                ++n;
            }
        }
        sprintf(part, "%s:%d ", cats[k], n);
        if (strlen(buf) + strlen(part) > 76) {
            con_out(buf);
            buf[0] = '\0';
        }
        str_cat(buf, part, sizeof(buf));
    }
    con_out(buf);
    return CMD_OK;
}

static int c_version(ConCtx *c)
{
    char buf[128];
    (void)c;
    sprintf(buf, "%s %s - %s (%s)", CLIDE_NAME, CLIDE_VERSION, CLIDE_ENGINE, c->plat->name);
    con_good(buf);
    con_out("Created by " CLIDE_CREATOR);
    return CMD_OK;
}

static int c_credits(ConCtx *c)
{
    (void)c;
    con_good(CLIDE_NAME " - " CLIDE_GENRE);
    con_out("Created by " CLIDE_CREATOR);
    con_out(loc_get("credits.line1"));
    return CMD_OK;
}

static int c_cheats(ConCtx *c)
{
    int v;
    if (!con_arg_onoff(c, 1, c->g->cheats, &v)) {
        return con_usage(c);
    }
    c->g->cheats = v;
    if (v) {
        c->g->cheated = 1;
    }
    con_good(loc_get(v ? "con.cheats_on" : "con.cheats_off"));
    return CMD_OK;
}

static int c_language(ConCtx *c)
{
    char buf[160];
    int i;
    if (c->argc < 2 || str_ieq(c->argv[1], "list")) {
        for (i = 0; i < loc_lang_count(); ++i) {
            LangInfo *li = loc_lang(i);
            sprintf(buf, "%-6s %-20s %s", li->code, li->name,
                    li->status == LANG_OK ? "ok" : li->status == LANG_UNVERIFIED ? "unverified" : "unavailable");
            con_out(buf);
        }
        return CMD_OK;
    }
    i = loc_find(c->argv[1]);
    if (i < 0) {
        con_err(loc_fmt("con.unknown_lang", c->argv[1], NULL, NULL));
        return CMD_ERR;
    }
    if (loc_lang(i)->status == LANG_UNAVAILABLE) {
        con_err(loc_get("con.lang_unavailable"));
        return CMD_ERR;
    }
    if (!loc_set(loc_lang(i)->code)) {
        con_err("cannot load language");
        return CMD_ERR;
    }
    help_load(g_data_root, loc_current());
    con_good(loc_get("con.lang_set"));
    return CMD_OK;
}

static int c_mem(ConCtx *c)
{
    char buf[160];
    sprintf(buf, "heap in use %lu B, peak %lu B, blocks %lu, budget %lu B",
            (unsigned long)mem_in_use(), (unsigned long)mem_peak(), (unsigned long)mem_alloc_count(),
            (unsigned long)mem_budget());
    con_out(buf);
    sprintf(buf, "world cache %lu B, game state %lu B, platform free %lu B",
            (unsigned long)world_memory(&c->g->world), (unsigned long)sizeof(Game),
            (unsigned long)c->plat->memory.free_bytes());
    con_out(buf);
    return CMD_OK;
}

static int c_chunks(ConCtx *c)
{
    char buf[160];
    World *w = &c->g->world;
    sprintf(buf, "cache %d/%d, columns %d, generated %lu, loaded %lu, evicted %lu, written %lu, stored %d",
            world_cached_chunks(w), w->nchunks, w->ncols, (unsigned long)w->stats.generated,
            (unsigned long)w->stats.loaded, (unsigned long)w->stats.evicted, (unsigned long)w->stats.written,
            w->store_open ? w->store.count : 0);
    con_out(buf);
    return CMD_OK;
}

static int c_entities(ConCtx *c)
{
    char buf[96];
    int i, shown = 0;
    sprintf(buf, "%d", ent_count(c->g));
    con_out(loc_fmt("con.count", buf, NULL, NULL));
    for (i = 0; i < MAX_ITEM_ENTS && shown < 8; ++i) {
        const ItemEntity *e = &c->g->ents[i];
        char nm[40];
        if (!e->active) {
            continue;
        }
        str_copy(nm, item_name(e->item), sizeof(nm));
        sprintf(buf, "%s x%d @ %ld %ld %ld%s", nm, e->count, e->x, e->y, e->z, e->floating ? " (floating)" : "");
        con_out(buf);
        ++shown;
    }
    return CMD_OK;
}

static int c_clearlog(ConCtx *c)
{
    (void)c;
    log_clear();
    return CMD_OK;
}

static int c_quit(ConCtx *c)
{
    (void)c;
    g_con_request = CON_REQ_QUIT;
    return CMD_OK;
}

static int c_debug(ConCtx *c)
{
    int v;
    if (!con_arg_onoff(c, 1, g_con_debug, &v)) {
        return con_usage(c);
    }
    g_con_debug = v;
    con_good(v ? "debug overlay on" : "debug overlay off");
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "help", "system", "[command|page]", "c", NULL, PERM_ANY, c_help, "?" },
    { "commands", "system", "", "", NULL, PERM_ANY, c_commands, NULL },
    { "version", "system", "", "", NULL, PERM_ANY, c_version, "ver" },
    { "credits", "system", "", "", NULL, PERM_ANY, c_credits, NULL },
    { "cheats", "settings", "<on|off>", "o", NULL, PERM_ANY, c_cheats, NULL },
    { "language", "settings", "<code|list>", "l", NULL, PERM_ANY, c_language, "lang" },
    { "mem", "debug", "", "", NULL, PERM_ANY, c_mem, "memory" },
    { "chunks", "debug", "", "", NULL, PERM_ANY, c_chunks, NULL },
    { "entities", "debug", "", "", NULL, PERM_ANY, c_entities, NULL },
    { "clearlog", "system", "", "", NULL, PERM_ANY, c_clearlog, "cls" },
    { "quit", "system", "", "", NULL, PERM_ANY, c_quit, "exit" },
    { "debug", "debug", "[on|off]", "o", NULL, PERM_ANY, c_debug, NULL }
};

CommandTable cmd_table_system(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
