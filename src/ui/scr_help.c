/*
 * Cliderness - ui/scr_help.c  (ClideHelp viewer)
 * Opening help pauses the game completely (Screen.pauses = 1).
 * Left: topic list (All / Favourites / Recent tabs, level filter, search).
 * Right: topic text, word-wrapped, scrollable. The "commands" topic is
 * generated from the live command registry so it is always complete.
 */
#include <stdio.h>
#include <string.h>
#include "ui/app.h"
#include "ui/widgets.h"
#include "help/help.h"
#include "commands/console.h"
#include "localization/locale.h"
#include "input/keybind.h"
#include "renderer/cp437.h"
#include "core/str.h"

static int g_h_list[HELP_MAX_TOPICS];
static int g_h_n = 0;
static int g_h_sel = 0;
static int g_h_top = 0;
static int g_h_scroll = 0;
static int g_h_tab = 0;        /* 0 all, 1 favourites, 2 recent */
static int g_h_level = 5;      /* show topics with level <= this */
static int g_h_searching = 0;
static int g_h_related = 0;
static TextInput g_h_query;
static char g_h_text[HELP_TEXT_MAX];
static int g_h_loaded = -1;

static void rebuild(void)
{
    int i;
    g_h_n = 0;
    if (g_h_tab == 1) {
        for (i = 0; i < g_help_nfav; ++i) {
            int t = help_find(g_help_fav[i]);
            if (t >= 0) g_h_list[g_h_n++] = t;
        }
    } else if (g_h_tab == 2) {
        for (i = 0; i < g_help_nrecent; ++i) {
            int t = help_find(g_help_recent[i]);
            if (t >= 0) g_h_list[g_h_n++] = t;
        }
    } else {
        for (i = 0; i < g_help.count; ++i) {
            if (g_help.t[i].level > g_h_level) {
                continue;
            }
            if (g_h_query.buf[0] && !help_matches(i, g_h_query.buf)) {
                continue;
            }
            g_h_list[g_h_n++] = i;
        }
    }
    if (g_h_sel >= g_h_n) {
        g_h_sel = g_h_n ? g_h_n - 1 : 0;
    }
    g_h_loaded = -1;
}

static void gen_commands(char *out, size_t n)
{
    int i;
    char line[200];
    out[0] = '\0';
    sprintf(line, "# %s\n", loc_fmt("con.command_count", "", NULL, NULL));
    {
        char num[12];
        sprintf(num, "%d", con_count());
        str_copy(line, "# ", sizeof(line));
        str_cat(line, loc_fmt("con.command_count", num, NULL, NULL), sizeof(line));
        str_cat(line, "\n", sizeof(line));
    }
    str_cat(out, line, n);
    str_cat(out, loc_get("help.cmd_intro"), n);
    str_cat(out, "\n\n", n);
    for (i = 0; i < con_count(); ++i) {
        const Command *c = con_get(i);
        str_copy(line, "> /", sizeof(line));
        str_cat(line, c->name, sizeof(line));
        str_cat(line, " ", sizeof(line));
        str_cat(line, c->syntax ? c->syntax : "", sizeof(line));
        if (c->perm == PERM_CHEAT) {
            str_cat(line, "  [cheat]", sizeof(line));
        }
        str_cat(line, "\n  ", sizeof(line));
        str_cat(line, con_desc(c), sizeof(line));
        str_cat(line, "\n", sizeof(line));
        if (strlen(out) + strlen(line) + 1 >= n) {
            break;
        }
        str_cat(out, line, n);
    }
}

static void load_current(void)
{
    int t;
    if (g_h_n == 0) {
        g_h_text[0] = '\0';
        return;
    }
    t = g_h_list[g_h_sel];
    if (t == g_h_loaded) {
        return;
    }
    g_h_loaded = t;
    g_h_scroll = 0;
    g_h_related = 0;
    if (str_ieq(g_help.t[t].id, "commands")) {
        help_read(t, g_h_text, sizeof(g_h_text) / 4);
        str_cat(g_h_text, "\n\n", sizeof(g_h_text));
        gen_commands(g_h_text + strlen(g_h_text), sizeof(g_h_text) - strlen(g_h_text));
    } else {
        help_read(t, g_h_text, sizeof(g_h_text));
    }
    help_touch_recent(g_help.t[t].id);
}

static void help_enter(void)
{
    int ctx;
    ti_set(&g_h_query, "", 24, 0);
    g_h_searching = 0;
    g_h_tab = 0;
    rebuild();
    ctx = help_find(g_app.help_context);
    g_h_sel = 0;
    if (ctx >= 0) {
        int i;
        for (i = 0; i < g_h_n; ++i) {
            if (g_h_list[i] == ctx) {
                g_h_sel = i;
            }
        }
    }
    g_h_top = g_h_sel > 10 ? g_h_sel - 10 : 0;
    g_h_loaded = -1;
}

static void help_draw(Framebuffer *fb)
{
    int i;
    int rows = 20;
    char buf[128];
    static const char *const tabs[3] = { "help.tab_all", "help.tab_fav", "help.tab_recent" };
    load_current();
    fb_clear(fb, UI_TEXT);
    ui_frame(fb, 0, 0, FB_W, FB_H - 1, loc_get("help.title"), ATTR(C_LIGHTGRAY, C_BLACK));
    /* tabs */
    {
        int x = 2;
        for (i = 0; i < 3; ++i) {
            cl_u8 a = i == g_h_tab ? UI_SEL : UI_DIM;
            x += fb_text(fb, x, 1, loc_get(tabs[i]), a, 12) + 1;
        }
    }
    sprintf(buf, "%s: %s", loc_get("help.level"), loc_get(help_level_key(g_h_level)));
    fb_text(fb, 2, 2, buf, UI_KEY, 24);
    if (g_h_searching || g_h_query.buf[0]) {
        sprintf(buf, "/%s%s", g_h_query.buf, g_h_searching ? "_" : "");
        fb_text(fb, 2, 22, buf, UI_KEY, 24);
    }
    if (g_h_sel < g_h_top) g_h_top = g_h_sel;
    if (g_h_sel >= g_h_top + rows - 2) g_h_top = g_h_sel - rows + 3;
    for (i = 0; i < rows - 2 && g_h_top + i < g_h_n; ++i) {
        int t = g_h_list[g_h_top + i];
        cl_u8 a = (g_h_top + i == g_h_sel) ? UI_SEL : UI_TEXT;
        fb_fill(fb, 1, 3 + i, 26, 1, ' ', a);
        fb_text(fb, 1, 3 + i, help_is_fav(g_help.t[t].id) ? "*" : " ", ATTR(C_YELLOW, ATTR_BG(a)), 1);
        fb_text(fb, 2, 3 + i, g_help.t[t].title, a, 24);
    }
    if (g_h_n == 0) {
        fb_text(fb, 2, 4, loc_get("help.no_results"), UI_DIM, 24);
    }
    fb_vline(fb, 27, 1, FB_H - 3, G_VLINE, UI_DIM);
    if (g_h_n > 0) {
        int t = g_h_list[g_h_sel];
        HelpTopic *ht = &g_help.t[t];
        fb_text(fb, 29, 1, ht->title, UI_TITLE, 50);
        sprintf(buf, "[%s]", loc_get(help_level_key(ht->level)));
        fb_text(fb, 29, 2, buf, UI_DIM, 30);
        ui_draw_wrapped(fb, 29, 3, 50, 18, g_h_text, g_h_scroll, UI_TEXT);
        if (ht->related[0]) {
            str_copy(buf, loc_get("help.related"), sizeof(buf));
            str_cat(buf, " ", sizeof(buf));
            str_cat(buf, ht->related, sizeof(buf));
            fb_text(fb, 29, 22, buf, UI_KEY, 50);
        }
    }
    ui_hint(fb, loc_get("hint.help"));
}

static void open_related(void)
{
    char buf[128];
    char *tok;
    int k = 0, t;
    HelpTopic *ht;
    if (g_h_n == 0) {
        return;
    }
    ht = &g_help.t[g_h_list[g_h_sel]];
    str_copy(buf, ht->related, sizeof(buf));
    for (tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
        if (k++ == g_h_related) {
            t = help_find(str_trim(tok));
            if (t >= 0) {
                int i;
                g_h_tab = 0;
                g_h_level = 5;
                ti_set(&g_h_query, "", 24, 0);
                rebuild();
                for (i = 0; i < g_h_n; ++i) {
                    if (g_h_list[i] == t) {
                        g_h_sel = i;
                    }
                }
            }
            return;
        }
    }
    g_h_related = 0;
}

static void help_key(const ClideKeyEvent *ev)
{
    if (g_h_searching) {
        if (ev->key == CK_ENTER || ev->key == CK_ESC) {
            g_h_searching = 0;
            if (ev->key == CK_ESC) {
                ti_set(&g_h_query, "", 24, 0);
            }
            rebuild();
            return;
        }
        if (ti_key(&g_h_query, ev)) {
            rebuild();
        }
        return;
    }
    if (ev->key == CK_ESC || ev->key == CK_F1 || ev->key == 'q') {
        settings_save();
        app_pop();
        return;
    }
    if (ev->key == CK_UP && g_h_sel > 0) --g_h_sel;
    else if (ev->key == CK_DOWN && g_h_sel + 1 < g_h_n) ++g_h_sel;
    else if (ev->key == CK_PGDN || ev->key == CK_SPACE) g_h_scroll += 8;
    else if (ev->key == CK_PGUP) g_h_scroll = g_h_scroll > 8 ? g_h_scroll - 8 : 0;
    else if (ev->key == CK_RIGHT) ++g_h_scroll;
    else if (ev->key == CK_LEFT && g_h_scroll > 0) --g_h_scroll;
    else if (ev->key == CK_HOME) g_h_sel = 0;
    else if (ev->key == CK_END && g_h_n) g_h_sel = g_h_n - 1;
    else if (ev->key == '/') g_h_searching = 1;
    else if (ev->key == CK_TAB) { g_h_tab = (g_h_tab + 1) % 3; g_h_sel = 0; rebuild(); }
    else if (ev->key == 'l' || ev->key == 'L') { g_h_level = g_h_level % 5 + 1; rebuild(); }
    else if ((ev->key == 'f' || ev->key == 'F') && g_h_n) {
        help_toggle_fav(g_help.t[g_h_list[g_h_sel]].id);
        if (g_h_tab == 1) rebuild();
    } else if (ev->key == 'r' || ev->key == 'R' || ev->key == CK_ENTER) {
        open_related();
        ++g_h_related;
    }
    if (g_h_scroll > 390) {
        g_h_scroll = 390;
    }
}

const Screen scr_help = { "help", 1, 1, help_enter, help_key, help_draw };
