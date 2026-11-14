/*
 * Cliderness - help/help.h  (ClideHelp)
 * Offline help documents in data/help/<lang>.hlp:
 *
 *   @topic <id>
 *   @title <title>
 *   @level <1..5>          1 Beginner .. 5 Master
 *   @related <id>,<id>
 *   text ...
 *
 * Only a small index (id, title, level, related, file offset) is kept in
 * memory; topic text is streamed from disk when opened (DOS friendly).
 */
#ifndef CLIDE_HELP_H
#define CLIDE_HELP_H

#include "core/types.h"

#define HELP_MAX_TOPICS 112
#define HELP_TEXT_MAX   12000
#define HELP_MAX_RECENT 8
#define HELP_MAX_FAV    16

typedef struct HelpTopic {
    char  id[24];
    char  title[56];
    char  related[80];
    cl_u8 level;
    long  offset;
} HelpTopic;

typedef struct HelpIndex {
    HelpTopic t[HELP_MAX_TOPICS];
    int  count;
    char path[160];
    char lang[8];
} HelpIndex;

extern HelpIndex g_help;

/* Load <root>/help/<lang>.hlp, falling back to en.hlp. */
int  help_load(const char *root, const char *lang);
int  help_find(const char *id);
/* Read the body of topic i into out (UTF-8, '\n' separated). */
int  help_read(int i, char *out, size_t n);
/* 1 if the title or body of topic i contains the query (case-insensitive). */
int  help_matches(int i, const char *query);
const char *help_level_key(int level);

/* favourites / recent (persisted by the settings module) */
extern char g_help_fav[HELP_MAX_FAV][24];
extern int  g_help_nfav;
extern char g_help_recent[HELP_MAX_RECENT][24];
extern int  g_help_nrecent;
void help_touch_recent(const char *id);
int  help_is_fav(const char *id);
void help_toggle_fav(const char *id);

#endif
