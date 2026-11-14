/*
 * Cliderness - commands/console.h  (ClideConsole)
 * Command registry, parser, permission checks, history and completion.
 * Every registered command has a real handler; the count reported by
 * /commands and `--list-commands` is computed from the registry.
 */
#ifndef CLIDE_COMMANDS_CONSOLE_H
#define CLIDE_COMMANDS_CONSOLE_H

#include "gameplay/game.h"
#include "platform/platform.h"

#define CON_MAX_ARGS 16
#define CON_LINE     160
#define CON_HISTORY  32

#define PERM_ANY   0
#define PERM_CHEAT 1

#define CMD_OK     0
#define CMD_USAGE  1
#define CMD_ERR    2
#define CMD_DENIED 3
#define CMD_UNKNOWN 4

/* argspec letters (completion + validation hints):
 *   n number/coordinate (~ relative)   b block id    i item id
 *   w weather id   g game mode   o on/off   s attribute   k skill
 *   S season   t time word   l language   c command   @ selector
 *   B biome   x one of `choices` (| separated)   * free text        */
typedef struct ConCtx {
    Game *g;
    const ClidePlatform *plat;
    int   argc;
    char *argv[CON_MAX_ARGS];
} ConCtx;

typedef int (*CmdFn)(ConCtx *c);

typedef struct Command {
    const char *name;
    const char *category;
    const char *syntax;
    const char *argspec;
    const char *choices;
    int         perm;
    CmdFn       fn;
    const char *aliases;   /* comma separated, may be NULL */
} Command;

void con_init(void);
int  con_count(void);
int  con_alias_count(void);
const Command *con_get(int i);
const Command *con_find(const char *name);
int  con_exec(Game *g, const ClidePlatform *p, const char *line);
/* Tab completion: completes the last token of line in place.
 * Repeated calls with the same line cycle through candidates. */
int  con_complete(char *line, size_t n);
void con_history_push(const char *line);
int  con_history_count(void);
const char *con_history_get(int age);  /* 0 = newest */
/* description of a command (localised) */
const char *con_desc(const Command *c);

/* set by commands that need the UI to act (e.g. /quit) */
#define CON_REQ_NONE 0
#define CON_REQ_QUIT 1
#define CON_REQ_DEBUG 2
extern int g_con_request;
extern int g_con_debug;

/* output helpers for command handlers */
void con_out(const char *text);
void con_good(const char *text);
void con_err(const char *text);
int  con_usage(ConCtx *c);

/* argument helpers */
int  con_arg_long(ConCtx *c, int i, long *out);
int  con_arg_coord(ConCtx *c, int i, long base, long *out);
int  con_arg_onoff(ConCtx *c, int i, int current, int *out);

/* per-category command tables */
typedef struct CommandTable {
    const Command *cmds;
    int count;
} CommandTable;
CommandTable cmd_table_player(void);
CommandTable cmd_table_world(void);
CommandTable cmd_table_build(void);
CommandTable cmd_table_items(void);
CommandTable cmd_table_system(void);

#endif
