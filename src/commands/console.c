/* Cliderness - commands/console.c */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "world/weather/weather.h"
#include "world/biomes/biome.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/log.h"
#include "core/str.h"

#define CON_MAX_CMDS 300

static const Command *g_reg[CON_MAX_CMDS];
static int g_nreg = 0;
static char g_hist[CON_HISTORY][CON_LINE];
static int g_nhist = 0;
int g_con_request = CON_REQ_NONE;
int g_con_arg = 0;
int g_con_debug = 0;

/* completion state */
static char g_comp_base[CON_LINE];
static char g_comp_last[CON_LINE];
static int g_comp_index = 0;

static void add_table(CommandTable t)
{
    int i;
    for (i = 0; i < t.count && g_nreg < CON_MAX_CMDS; ++i) {
        g_reg[g_nreg++] = &t.cmds[i];
    }
}

void con_init(void)
{
    g_nreg = 0;
    add_table(cmd_table_player());
    add_table(cmd_table_world());
    add_table(cmd_table_build());
    add_table(cmd_table_items());
    add_table(cmd_table_system());
    add_table(cmd_table_craft());
    add_table(cmd_table_fluid());
    add_table(cmd_table_mob());
    add_table(cmd_table_ship());
    add_table(cmd_table_ancient());
    add_table(cmd_table_shape());
    add_table(cmd_table_info());
    add_table(cmd_table_act());
}

int con_count(void)
{
    return g_nreg;
}

int con_alias_count(void)
{
    int i, n = 0;
    for (i = 0; i < g_nreg; ++i) {
        const char *a = g_reg[i]->aliases;
        if (a && *a) {
            ++n;
            for (; *a; ++a) {
                if (*a == ',') {
                    ++n;
                }
            }
        }
    }
    return n;
}

const Command *con_get(int i)
{
    return (i >= 0 && i < g_nreg) ? g_reg[i] : NULL;
}

static int alias_match(const char *aliases, const char *name)
{
    char buf[64];
    char *tok;
    if (!aliases) {
        return 0;
    }
    str_copy(buf, aliases, sizeof(buf));
    for (tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
        if (str_ieq(str_trim(tok), name)) {
            return 1;
        }
    }
    return 0;
}

const Command *con_find(const char *name)
{
    int i;
    if (*name == '/') {
        ++name;
    }
    for (i = 0; i < g_nreg; ++i) {
        if (str_ieq(g_reg[i]->name, name)) {
            return g_reg[i];
        }
    }
    for (i = 0; i < g_nreg; ++i) {
        if (alias_match(g_reg[i]->aliases, name)) {
            return g_reg[i];
        }
    }
    return NULL;
}

const char *con_desc(const Command *c)
{
    char key[48];
    str_copy(key, "cmd.", sizeof(key));
    str_cat(key, c->name, sizeof(key));
    return loc_get_or(key, "");
}

void con_out(const char *text)
{
    log_add(ATTR(C_LIGHTGRAY, C_BLACK), text);
}

void con_good(const char *text)
{
    log_add(ATTR(C_LIGHTGREEN, C_BLACK), text);
}

void con_err(const char *text)
{
    log_add(ATTR(C_LIGHTRED, C_BLACK), text);
}

static const Command *g_running = NULL;

int con_usage(ConCtx *c)
{
    char buf[160];
    (void)c;
    if (g_running) {
        str_copy(buf, loc_get("con.usage"), sizeof(buf));
        str_cat(buf, " /", sizeof(buf));
        str_cat(buf, g_running->name, sizeof(buf));
        if (g_running->syntax && *g_running->syntax) {
            str_cat(buf, " ", sizeof(buf));
            str_cat(buf, g_running->syntax, sizeof(buf));
        }
        con_err(buf);
    }
    return CMD_USAGE;
}

int con_arg_long(ConCtx *c, int i, long *out)
{
    if (i >= c->argc) {
        return 0;
    }
    return str_to_long(c->argv[i], out);
}

int con_arg_coord(ConCtx *c, int i, long base, long *out)
{
    const char *s;
    long n = 0;
    if (i >= c->argc) {
        return 0;
    }
    s = c->argv[i];
    if (s[0] == '~') {
        if (s[1] && !str_to_long(s + 1, &n)) {
            return 0;
        }
        *out = base + n;
        return 1;
    }
    return str_to_long(s, out);
}

int con_arg_onoff(ConCtx *c, int i, int current, int *out)
{
    if (i >= c->argc) {
        *out = !current;
        return 1;
    }
    if (str_ieq(c->argv[i], "on") || str_ieq(c->argv[i], "1") || str_ieq(c->argv[i], "true")) {
        *out = 1;
        return 1;
    }
    if (str_ieq(c->argv[i], "off") || str_ieq(c->argv[i], "0") || str_ieq(c->argv[i], "false")) {
        *out = 0;
        return 1;
    }
    if (str_ieq(c->argv[i], "toggle")) {
        *out = !current;
        return 1;
    }
    return 0;
}

static int tokenize(char *s, char **argv, int max)
{
    int n = 0;
    while (*s && n < max) {
        while (*s == ' ' || *s == '\t') {
            ++s;
        }
        if (!*s) {
            break;
        }
        if (*s == '"') {
            argv[n++] = ++s;
            while (*s && *s != '"') {
                ++s;
            }
        } else {
            argv[n++] = s;
            while (*s && *s != ' ' && *s != '\t') {
                ++s;
            }
        }
        if (*s) {
            *s++ = '\0';
        }
    }
    return n;
}

int con_exec(Game *g, const ClidePlatform *p, const char *line)
{
    static char buf[CON_LINE];
    ConCtx c;
    const Command *cmd;
    int r;
    str_copy(buf, line, sizeof(buf));
    c.g = g;
    c.plat = p;
    c.argc = tokenize(buf, c.argv, CON_MAX_ARGS);
    if (c.argc == 0) {
        return CMD_OK;
    }
    cmd = con_find(c.argv[0]);
    if (!cmd) {
        con_err(loc_fmt("con.unknown", c.argv[0], NULL, NULL));
        return CMD_UNKNOWN;
    }
    if (cmd->perm == PERM_CHEAT && !g->cheats) {
        con_err(loc_get("con.no_cheats"));
        return CMD_DENIED;
    }
    if (cmd->perm == PERM_CHEAT) {
        g->cheated = 1;
    }
    g_running = cmd;
    r = cmd->fn(&c);
    g_running = NULL;
    return r;
}

void con_history_push(const char *line)
{
    int i;
    if (!*line) {
        return;
    }
    if (g_nhist > 0 && strcmp(g_hist[0], line) == 0) {
        return;
    }
    if (g_nhist < CON_HISTORY) {
        ++g_nhist;
    }
    for (i = g_nhist - 1; i > 0; --i) {
        str_copy(g_hist[i], g_hist[i - 1], CON_LINE);
    }
    str_copy(g_hist[0], line, CON_LINE);
}

int con_history_count(void)
{
    return g_nhist;
}

const char *con_history_get(int age)
{
    return (age >= 0 && age < g_nhist) ? g_hist[age] : "";
}

/* ---------------------------------------------------------- completion */

static int cand_count;
static const char *cand_pick;
static int cand_want;

static void cand(const char *s, const char *prefix)
{
    if (!str_iprefix(s, prefix)) {
        return;
    }
    if (cand_count == cand_want) {
        cand_pick = s;
    }
    ++cand_count;
}

static void cand_list(const char *list, const char *prefix)
{
    static char buf[256];
    static char toks[16][32];
    char *tok;
    int n = 0;
    str_copy(buf, list, sizeof(buf));
    for (tok = strtok(buf, "|"); tok && n < 16; tok = strtok(NULL, "|")) {
        str_copy(toks[n], tok, sizeof(toks[n]));
        cand(toks[n], prefix);
        ++n;
    }
}

static void candidates(char type, const Command *cmd, const char *prefix)
{
    int i;
    static const char *const stats[] = { "strength", "dexterity", "intelligence", "perception",
                                         "constitution", "endurance", "willpower", "luck" };
    static const char *const skills[] = { "mining", "woodcutting", "building", "crafting", "melee",
                                          "ranged", "swimming", "sailing", "fishing", "cooking",
                                          "stealth", "exploration" };
    switch (type) {
    case 'c':
        for (i = 0; i < g_nreg; ++i) cand(g_reg[i]->name, prefix);
        break;
    case 'b':
        for (i = 1; i < g_block_count; ++i) cand(g_blocks[i].id, prefix);
        cand("air", prefix);
        break;
    case 'i':
        for (i = 0; i < g_item_count; ++i) cand(g_items[i].id, prefix);
        break;
    case 'w':
        for (i = 0; i < g_weather_count; ++i) cand(g_weather[i].id, prefix);
        break;
    case 'B':
        for (i = 0; i < g_biome_count; ++i) cand(g_biomes[i].id, prefix);
        break;
    case 'g':
        cand_list("survival|creative|adventure|spectator", prefix);
        break;
    case 'o':
        cand_list("on|off", prefix);
        break;
    case 's':
        for (i = 0; i < 8; ++i) cand(stats[i], prefix);
        break;
    case 'k':
        for (i = 0; i < 12; ++i) cand(skills[i], prefix);
        break;
    case 'S':
        cand_list("spring|summer|autumn|winter", prefix);
        break;
    case 't':
        cand_list("day|noon|sunset|night|midnight|sunrise", prefix);
        break;
    case 'l':
        for (i = 0; i < loc_lang_count(); ++i) cand(loc_lang(i)->code, prefix);
        break;
    case '@':
        cand_list("@s|@p|@e|@e[type=item]", prefix);
        break;
    case 'x':
        if (cmd && cmd->choices) cand_list(cmd->choices, prefix);
        break;
    default:
        break;
    }
}

int con_complete(char *line, size_t n)
{
    char work[CON_LINE];
    char *argv[CON_MAX_ARGS];
    int argc, ends_space;
    const char *prefix;
    char type;
    const Command *cmd = NULL;
    size_t len;
    if (strcmp(line, g_comp_last) != 0 || !g_comp_last[0]) {
        str_copy(g_comp_base, line, sizeof(g_comp_base));
        g_comp_index = 0;
    } else {
        ++g_comp_index;
    }
    str_copy(work, g_comp_base, sizeof(work));
    len = strlen(work);
    ends_space = len > 0 && work[len - 1] == ' ';
    argc = tokenize(work, argv, CON_MAX_ARGS);
    if (argc == 0 || (argc == 1 && !ends_space)) {
        type = 'c';
        prefix = argc ? argv[0] : "";
        if (*prefix == '/') {
            ++prefix;
        }
    } else {
        int argi = ends_space ? argc - 1 : argc - 2;
        cmd = con_find(argv[0]);
        if (!cmd || !cmd->argspec) {
            return 0;
        }
        type = argi < (int)strlen(cmd->argspec) ? cmd->argspec[argi] : 0;
        prefix = ends_space ? "" : argv[argc - 1];
    }
    cand_count = 0;
    cand_pick = NULL;
    cand_want = -1;
    candidates(type, cmd, prefix);
    if (cand_count == 0) {
        return 0;
    }
    cand_want = g_comp_index % cand_count;
    cand_count = 0;
    candidates(type, cmd, prefix);
    if (!cand_pick) {
        return 0;
    }
    {
        /* rebuild: base without the partial token + candidate */
        char out[CON_LINE];
        size_t keep = strlen(g_comp_base);
        if (!ends_space) {
            while (keep > 0 && g_comp_base[keep - 1] != ' ') {
                --keep;
            }
        }
        if (keep >= sizeof(out)) {
            keep = sizeof(out) - 1;
        }
        memcpy(out, g_comp_base, keep);
        out[keep] = '\0';
        if (type == 'c' && keep == 0 && g_comp_base[0] == '/') {
            str_cat(out, "/", sizeof(out));
        }
        str_cat(out, cand_pick, sizeof(out));
        str_cat(out, " ", sizeof(out));
        str_copy(line, out, n);
        /* with a single candidate the next Tab starts a fresh completion
         * (e.g. of the next argument); otherwise it cycles */
        if (cand_count == 1) {
            g_comp_last[0] = '\0';
        } else {
            str_copy(g_comp_last, line, sizeof(g_comp_last));
        }
    }
    return 1;
}
