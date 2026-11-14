/* Cliderness - help/help.c */
#include <stdio.h>
#include <string.h>
#include "help/help.h"
#include "core/str.h"

HelpIndex g_help;
char g_help_fav[HELP_MAX_FAV][24];
int  g_help_nfav = 0;
char g_help_recent[HELP_MAX_RECENT][24];
int  g_help_nrecent = 0;

static int load_file(const char *path)
{
    static char line[512];
    FILE *f = fopen(path, "rb");
    HelpTopic *cur = NULL;
    if (!f) {
        return 0;
    }
    g_help.count = 0;
    str_copy(g_help.path, path, sizeof(g_help.path));
    for (;;) {
        long off = ftell(f);
        char *s;
        if (!fgets(line, sizeof(line), f)) {
            break;
        }
        s = line;
        if ((unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) {
            s += 3;
        }
        if (s[0] != '@') {
            continue;
        }
        s = str_trim(s);
        if (str_iprefix(s, "@topic ")) {
            if (g_help.count >= HELP_MAX_TOPICS) {
                break;
            }
            cur = &g_help.t[g_help.count++];
            memset(cur, 0, sizeof(*cur));
            str_copy(cur->id, str_trim(s + 7), sizeof(cur->id));
            str_copy(cur->title, cur->id, sizeof(cur->title));
            cur->level = 1;
            cur->offset = off;
        } else if (cur && str_iprefix(s, "@title ")) {
            str_copy(cur->title, str_trim(s + 7), sizeof(cur->title));
        } else if (cur && str_iprefix(s, "@level ")) {
            long n;
            if (str_to_long(s + 7, &n) && n >= 1 && n <= 5) {
                cur->level = (cl_u8)n;
            }
        } else if (cur && str_iprefix(s, "@related ")) {
            str_copy(cur->related, str_trim(s + 9), sizeof(cur->related));
        }
    }
    fclose(f);
    return g_help.count > 0;
}

int help_load(const char *root, const char *lang)
{
    char path[160];
    str_copy(path, root, sizeof(path));
    str_cat(path, "/help/", sizeof(path));
    str_cat(path, lang, sizeof(path));
    str_cat(path, ".hlp", sizeof(path));
    if (load_file(path)) {
        str_copy(g_help.lang, lang, sizeof(g_help.lang));
        return 1;
    }
    str_copy(path, root, sizeof(path));
    str_cat(path, "/help/en.hlp", sizeof(path));
    str_copy(g_help.lang, "en", sizeof(g_help.lang));
    return load_file(path);
}

int help_find(const char *id)
{
    int i;
    for (i = 0; i < g_help.count; ++i) {
        if (str_ieq(g_help.t[i].id, id)) {
            return i;
        }
    }
    return -1;
}

int help_read(int i, char *out, size_t n)
{
    static char line[512];
    FILE *f;
    size_t o = 0;
    int first = 1;
    out[0] = '\0';
    if (i < 0 || i >= g_help.count) {
        return 0;
    }
    f = fopen(g_help.path, "rb");
    if (!f) {
        return 0;
    }
    fseek(f, g_help.t[i].offset, SEEK_SET);
    while (fgets(line, sizeof(line), f)) {
        size_t len;
        if (line[0] == '@') {
            if (!first && str_iprefix(line, "@topic ")) {
                break;
            }
            first = 0;
            continue;
        }
        first = 0;
        len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (o + len + 2 >= n) {
            break;
        }
        memcpy(out + o, line, len);
        o += len;
        out[o++] = '\n';
        out[o] = '\0';
    }
    fclose(f);
    while (o > 0 && out[o - 1] == '\n') {
        out[--o] = '\0';
    }
    return 1;
}

int help_matches(int i, const char *query)
{
    static char buf[HELP_TEXT_MAX];
    if (i < 0 || i >= g_help.count) {
        return 0;
    }
    if (!*query || str_icontains(g_help.t[i].title, query) || str_icontains(g_help.t[i].id, query)) {
        return 1;
    }
    help_read(i, buf, sizeof(buf));
    return str_icontains(buf, query);
}

const char *help_level_key(int level)
{
    static const char *const k[6] = {
        "help.level0", "help.level1", "help.level2", "help.level3", "help.level4", "help.level5"
    };
    return k[(level >= 0 && level <= 5) ? level : 0];
}

void help_touch_recent(const char *id)
{
    int i, j;
    for (i = 0; i < g_help_nrecent; ++i) {
        if (str_ieq(g_help_recent[i], id)) {
            break;
        }
    }
    if (i == g_help_nrecent) {
        if (g_help_nrecent < HELP_MAX_RECENT) {
            ++g_help_nrecent;
        }
        i = g_help_nrecent - 1;
    }
    for (j = i; j > 0; --j) {
        str_copy(g_help_recent[j], g_help_recent[j - 1], 24);
    }
    str_copy(g_help_recent[0], id, 24);
}

int help_is_fav(const char *id)
{
    int i;
    for (i = 0; i < g_help_nfav; ++i) {
        if (str_ieq(g_help_fav[i], id)) {
            return 1;
        }
    }
    return 0;
}

void help_toggle_fav(const char *id)
{
    int i;
    for (i = 0; i < g_help_nfav; ++i) {
        if (str_ieq(g_help_fav[i], id)) {
            for (; i + 1 < g_help_nfav; ++i) {
                str_copy(g_help_fav[i], g_help_fav[i + 1], 24);
            }
            --g_help_nfav;
            return;
        }
    }
    if (g_help_nfav < HELP_MAX_FAV) {
        str_copy(g_help_fav[g_help_nfav++], id, 24);
    }
}
