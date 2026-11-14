/* Cliderness - input/keybind.c */
#include <stdio.h>
#include <string.h>
#include "input/keybind.h"
#include "core/ini.h"
#include "core/str.h"

KeySpec g_binds[ACT_COUNT][2];

static const char *const g_ids[ACT_COUNT] = {
    "none", "move_north", "move_west", "move_south", "move_east",
    "crouch", "look_sky", "inventory", "drop", "drop_stack", "character", "interact",
    "hotbar1", "hotbar2", "hotbar3", "hotbar4", "hotbar5", "hotbar6", "hotbar7", "hotbar8", "hotbar9",
    "help", "pause", "console", "mine", "place", "dig_down", "build_up", "use", "panel_page", "craft"
};

static const char *const g_defaults[ACT_COUNT][2] = {
    { "", "" },
    { "W", "Up" }, { "A", "Left" }, { "S", "Down" }, { "D", "Right" },
    { "Ctrl", "X" }, { "RShift", "V" }, { "Tab", "I" }, { "Q", "" }, { "Ctrl+Q", "" },
    { "C", "" }, { "E", "" },
    { "1", "" }, { "2", "" }, { "3", "" }, { "4", "" }, { "5", "" }, { "6", "" }, { "7", "" },
    { "8", "" }, { "9", "" },
    { "F1", "?" }, { "Esc", "" }, { "/", "T" }, { "F", "" }, { "R", "" }, { ">", "" }, { "<", "" },
    { "U", "" }, { "P", "" }, { "K", "" }
};

typedef struct NamedKey {
    const char *name;
    long key;
} NamedKey;

static const NamedKey g_named[] = {
    { "Up", CK_UP }, { "Down", CK_DOWN }, { "Left", CK_LEFT }, { "Right", CK_RIGHT },
    { "Home", CK_HOME }, { "End", CK_END }, { "PgUp", CK_PGUP }, { "PgDn", CK_PGDN },
    { "Ins", CK_INSERT }, { "Del", CK_DELETE }, { "Tab", CK_TAB }, { "Enter", CK_ENTER },
    { "Esc", CK_ESC }, { "Space", CK_SPACE }, { "Backspace", CK_BACKSPACE },
    { "RShift", CK_RSHIFT }, { "LShift", CK_LSHIFT }, { "Ctrl", CK_CTRL }
};

const char *kb_action_id(int a)
{
    return (a >= 0 && a < ACT_COUNT) ? g_ids[a] : "none";
}

const char *kb_action_key(int a)
{
    static char buf[40];
    str_copy(buf, "action.", sizeof(buf));
    str_cat(buf, kb_action_id(a), sizeof(buf));
    return buf;
}

static long norm(long k)
{
    if (k >= 'A' && k <= 'Z') {
        return k - 'A' + 'a';
    }
    return k;
}

int kb_parse(const char *s, KeySpec *out)
{
    char buf[32];
    char *p = buf;
    int i;
    long n;
    out->key = CK_NONE;
    out->mods = 0;
    str_copy(buf, s, sizeof(buf));
    p = str_trim(buf);
    if (!*p) {
        return 0;
    }
    for (;;) {
        if (str_iprefix(p, "Ctrl+") && p[5]) {
            out->mods |= KM_CTRL;
            p += 5;
        } else if (str_iprefix(p, "Alt+") && p[4]) {
            out->mods |= KM_ALT;
            p += 4;
        } else {
            break;
        }
    }
    for (i = 0; i < (int)CL_COUNTOF(g_named); ++i) {
        if (str_ieq(p, g_named[i].name)) {
            out->key = g_named[i].key;
            return 1;
        }
    }
    if ((p[0] == 'F' || p[0] == 'f') && p[1] && str_to_long(p + 1, &n) && n >= 1 && n <= 12) {
        out->key = CK_F1 + n - 1;
        return 1;
    }
    if (p[0] && !p[1]) {
        out->key = norm((long)(unsigned char)p[0]);
        return 1;
    }
    return 0;
}

void kb_format(const KeySpec *k, char *out, int n)
{
    int i;
    out[0] = '\0';
    if (k->key == CK_NONE) {
        return;
    }
    if (k->mods & KM_CTRL) {
        str_cat(out, "Ctrl+", (size_t)n);
    }
    if (k->mods & KM_ALT) {
        str_cat(out, "Alt+", (size_t)n);
    }
    for (i = 0; i < (int)CL_COUNTOF(g_named); ++i) {
        if (g_named[i].key == k->key) {
            str_cat(out, g_named[i].name, (size_t)n);
            return;
        }
    }
    if (k->key >= CK_F1 && k->key <= CK_F12) {
        char f[8];
        sprintf(f, "F%d", (int)(k->key - CK_F1 + 1));
        str_cat(out, f, (size_t)n);
        return;
    }
    if (k->key > 32 && k->key < 127) {
        char c[2];
        c[0] = (char)(k->key >= 'a' && k->key <= 'z' ? k->key - 'a' + 'A' : k->key);
        c[1] = '\0';
        str_cat(out, c, (size_t)n);
    }
}

void kb_describe(int a, char *out, int n)
{
    char b1[24], b2[24];
    kb_format(&g_binds[a][0], b1, sizeof(b1));
    kb_format(&g_binds[a][1], b2, sizeof(b2));
    str_copy(out, b1, (size_t)n);
    if (b2[0]) {
        if (b1[0]) {
            str_cat(out, " / ", (size_t)n);
        }
        str_cat(out, b2, (size_t)n);
    }
}

void kb_defaults(void)
{
    int a, k;
    for (a = 0; a < ACT_COUNT; ++a) {
        for (k = 0; k < 2; ++k) {
            if (!kb_parse(g_defaults[a][k], &g_binds[a][k])) {
                g_binds[a][k].key = CK_NONE;
                g_binds[a][k].mods = 0;
            }
        }
    }
}

int kb_spec_matches(const KeySpec *k, const ClideKeyEvent *ev)
{
    long key = norm(ev->key);
    if (k->key == CK_NONE) {
        return 0;
    }
    if (norm(k->key) != key) {
        return 0;
    }
    return (k->mods & (KM_CTRL | KM_ALT)) == (ev->mods & (KM_CTRL | KM_ALT));
}

int kb_action(const ClideKeyEvent *ev, int *sprint)
{
    int a, k;
    if (sprint) {
        *sprint = ((ev->mods & KM_SHIFT) != 0) || (ev->key >= 'A' && ev->key <= 'Z');
    }
    for (a = 1; a < ACT_COUNT; ++a) {
        for (k = 0; k < 2; ++k) {
            if (kb_spec_matches(&g_binds[a][k], ev)) {
                return a;
            }
        }
    }
    return ACT_NONE;
}

static int load_cb(void *user, const char *type, const char *id, const char *key,
                   const char *value, int line)
{
    int a;
    (void)user;
    (void)type;
    (void)id;
    (void)line;
    if (!key) {
        return 0;
    }
    for (a = 1; a < ACT_COUNT; ++a) {
        if (str_ieq(key, g_ids[a])) {
            char buf[64];
            char *comma;
            str_copy(buf, value, sizeof(buf));
            comma = strchr(buf, ',');
            if (comma) {
                *comma = '\0';
                if (!kb_parse(comma + 1, &g_binds[a][1])) {
                    g_binds[a][1].key = CK_NONE;
                }
            } else {
                g_binds[a][1].key = CK_NONE;
            }
            if (!kb_parse(buf, &g_binds[a][0])) {
                g_binds[a][0].key = CK_NONE;
            }
            return 0;
        }
    }
    return 0;
}

int kb_load(const char *path)
{
    kb_defaults();
    return ini_parse_file(path, load_cb, NULL) == 0;
}

int kb_save(const char *path)
{
    FILE *f = fopen(path, "w");
    int a;
    if (!f) {
        return 0;
    }
    fprintf(f, "# Cliderness key bindings (two keys per action, comma separated)\n[keys]\n");
    for (a = 1; a < ACT_COUNT; ++a) {
        char b1[24], b2[24];
        kb_format(&g_binds[a][0], b1, sizeof(b1));
        kb_format(&g_binds[a][1], b2, sizeof(b2));
        fprintf(f, "%s=%s%s%s\n", g_ids[a], b1, b2[0] ? ", " : "", b2);
    }
    return fclose(f) == 0;
}
