/* Cliderness - ui/scr_settings.c : settings, language selection, key bindings */
#include <stdio.h>
#include <string.h>
#include "ui/app.h"
#include "ui/widgets.h"
#include "gameplay/game.h"
#include "content/content.h"
#include "localization/locale.h"
#include "help/help.h"
#include "input/keybind.h"
#include "audio/audio.h"
#include "core/str.h"

/* ------------------------------------------------------------ settings */

static int g_set_sel = 0;
static const long g_day_lengths[] = { 600, 1200, 1800, 2400, 3600 };

static void settings_draw(Framebuffer *fb)
{
    char vals[5][64];
    const char *labels[5];
    int i;
    LangInfo *li = loc_lang(loc_find(loc_current()));
    labels[0] = loc_get("set.language");
    labels[1] = loc_get("set.keys");
    labels[2] = loc_get("set.audio");
    labels[3] = loc_get("set.day_length");
    labels[4] = loc_get("nw.back");
    str_copy(vals[0], li ? li->native : loc_current(), sizeof(vals[0]));
    vals[1][0] = '\0';
    sprintf(vals[2], "< %s >", loc_get(g_app.set.audio ? "con.on" : "con.off"));
    sprintf(vals[3], "< %ld min >", g_app.set.day_length / 60L);
    vals[4][0] = '\0';
    ui_frame(fb, 12, 4, 56, 16, loc_get("menu.settings"), UI_BOX);
    for (i = 0; i < 5; ++i) {
        cl_u8 a = i == g_set_sel ? UI_BOXSEL : UI_BOX;
        fb_fill(fb, 14, 6 + i * 2, 52, 1, ' ', a);
        fb_text(fb, 15, 6 + i * 2, labels[i], a, 24);
        fb_text(fb, 40, 6 + i * 2, vals[i], a, 25);
    }
    {
        char info[96];
        sprintf(info, "%s: %s", loc_get("set.audio_dev"), aud_backend());
        fb_text(fb, 14, 17, info, ATTR(C_LIGHTGRAY, C_BLUE), 52);
        fb_text(fb, 14, 18, loc_get("set.day_note"), ATTR(C_LIGHTGRAY, C_BLUE), 52);
    }
    ui_hint(fb, loc_get("hint.settings"));
}

static void settings_key(const ClideKeyEvent *ev)
{
    int i;
    if (ev->key == CK_ESC) {
        settings_save();
        app_pop();
        return;
    }
    if (ev->key == CK_LEFT || ev->key == CK_RIGHT) {
        if (g_set_sel == 2) {
            g_app.set.audio = !g_app.set.audio;
            aud_set_enabled(g_app.set.audio);
        } else if (g_set_sel == 3) {
            int n = (int)CL_COUNTOF(g_day_lengths), cur = 1;
            for (i = 0; i < n; ++i) {
                if (g_day_lengths[i] == g_app.set.day_length) {
                    cur = i;
                }
            }
            cur = (cur + (ev->key == CK_LEFT ? n - 1 : 1)) % n;
            g_app.set.day_length = g_day_lengths[cur];
            if (g_game.active) {
                g_game.time.cfg.real_seconds_per_day = g_app.set.day_length;
            }
        }
        settings_save();
        return;
    }
    if (!menu_key(&g_set_sel, 5, ev)) {
        return;
    }
    switch (g_set_sel) {
    case 0: app_push(&scr_language); break;
    case 1: app_push(&scr_keybinds); break;
    case 2:
        g_app.set.audio = !g_app.set.audio;
        aud_set_enabled(g_app.set.audio);
        settings_save();
        break;
    case 3: {
        ClideKeyEvent r;
        r.key = CK_RIGHT;
        r.mods = 0;
        settings_key(&r);
        break;
    }
    default:
        settings_save();
        app_pop();
        break;
    }
}

const Screen scr_settings = { "settings", 1, 1, NULL, settings_key, settings_draw };

/* ------------------------------------------------------------ language */

static int g_lang_sel = 0;
static int g_lang_confirm = -1;
static int g_lang_order[LOC_MAX_LANGS];
static int g_lang_n = 0;
static int g_lang_hidden = 0;

static int confirmed(const char *code)
{
    char buf[64];
    char *tok;
    str_copy(buf, g_app.set.lang_confirmed, sizeof(buf));
    for (tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
        if (str_ieq(tok, code)) {
            return 1;
        }
    }
    return 0;
}

static void language_enter(void)
{
    int i, pass;
    g_lang_n = 0;
    g_lang_hidden = 0;
    g_lang_confirm = -1;
    /* verified languages first, then unverified ones; unavailable are hidden */
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < loc_lang_count(); ++i) {
            LangInfo *li = loc_lang(i);
            int ok = li->status == LANG_OK || confirmed(li->code);
            if (li->status == LANG_UNAVAILABLE) {
                if (pass == 0) {
                    ++g_lang_hidden;
                }
                continue;
            }
            if ((pass == 0) == ok) {
                g_lang_order[g_lang_n++] = i;
            }
        }
    }
    g_lang_sel = 0;
    for (i = 0; i < g_lang_n; ++i) {
        if (str_ieq(loc_lang(g_lang_order[i])->code, loc_current())) {
            g_lang_sel = i;
        }
    }
}

static void apply_lang(int idx)
{
    LangInfo *li = loc_lang(idx);
    if (loc_set(li->code)) {
        help_load(g_data_root, loc_current());
        settings_save();
    }
}

static void language_draw(Framebuffer *fb)
{
    int i;
    char buf[96];
    ui_frame(fb, 10, 2, 60, 21, loc_get("set.language"), UI_BOX);
    for (i = 0; i < g_lang_n; ++i) {
        LangInfo *li = loc_lang(g_lang_order[i]);
        cl_u8 a = i == g_lang_sel ? UI_BOXSEL : UI_BOX;
        int ok = li->status == LANG_OK || confirmed(li->code);
        fb_fill(fb, 12, 4 + i, 56, 1, ' ', a);
        fb_text(fb, 13, 4 + i, li->code, a, 6);
        fb_text(fb, 20, 4 + i, li->name, a, 18);
        /* native names are only drawn when they can be displayed */
        if (ok || (g_app.caps & TERMCAP_UNICODE)) {
            fb_text(fb, 39, 4 + i, li->native, a, 18);
        }
        if (!ok) {
            fb_text(fb, 58, 4 + i, loc_get("lang.unverified"), (cl_u8)((a & 0xF0) | C_YELLOW), 10);
        }
        if (str_ieq(li->code, loc_current())) {
            fb_text(fb, 12, 4 + i, "*", a, 1);
        }
    }
    if (g_lang_hidden) {
        sprintf(buf, "%d", g_lang_hidden);
        fb_text(fb, 12, 20, loc_fmt("lang.hidden", buf, NULL, NULL), ATTR(C_LIGHTGRAY, C_BLUE), 56);
    }
    if (g_lang_confirm >= 0) {
        LangInfo *li = loc_lang(g_lang_confirm);
        ui_frame(fb, 14, 8, 52, 8, loc_get("lang.confirm_title"), ATTR(C_WHITE, C_RED));
        fb_text(fb, 16, 10, li->sample[0] ? li->sample : li->native, ATTR(C_YELLOW, C_RED), 48);
        fb_text(fb, 16, 12, loc_get("lang.confirm_q"), ATTR(C_WHITE, C_RED), 48);
        fb_text(fb, 16, 13, "Y / N", ATTR(C_WHITE, C_RED), 48);
    }
    ui_hint(fb, loc_get("hint.select"));
}

static void language_key(const ClideKeyEvent *ev)
{
    if (g_lang_confirm >= 0) {
        if (ev->key == 'y' || ev->key == 'Y') {
            LangInfo *li = loc_lang(g_lang_confirm);
            if (!confirmed(li->code)) {
                if (g_app.set.lang_confirmed[0]) {
                    str_cat(g_app.set.lang_confirmed, ",", sizeof(g_app.set.lang_confirmed));
                }
                str_cat(g_app.set.lang_confirmed, li->code, sizeof(g_app.set.lang_confirmed));
            }
            apply_lang(g_lang_confirm);
            g_lang_confirm = -1;
        } else if (ev->key == 'n' || ev->key == 'N' || ev->key == CK_ESC) {
            g_lang_confirm = -1;
        }
        return;
    }
    if (ev->key == CK_ESC) {
        app_pop();
        return;
    }
    if (menu_key(&g_lang_sel, g_lang_n, ev) && g_lang_n > 0) {
        int idx = g_lang_order[g_lang_sel];
        LangInfo *li = loc_lang(idx);
        if (li->status == LANG_OK || confirmed(li->code)) {
            apply_lang(idx);
        } else {
            g_lang_confirm = idx;
        }
    }
}

const Screen scr_language = { "language", 1, 1, language_enter, language_key, language_draw };

/* ------------------------------------------------------------ key bindings */

static int g_kb_sel = 1;
static int g_kb_top = 1;
static int g_kb_wait = 0;
static int g_kb_slot = 0;

static void keybinds_enter(void)
{
    g_kb_sel = 1;
    g_kb_top = 1;
    g_kb_wait = 0;
}

static void keybinds_draw(Framebuffer *fb)
{
    int i, rows = 17;
    char buf[48];
    ui_frame(fb, 6, 1, 68, 22, loc_get("set.keys"), UI_BOX);
    for (i = 0; i < rows && g_kb_top + i < ACT_COUNT; ++i) {
        int a = g_kb_top + i;
        cl_u8 at = a == g_kb_sel ? UI_BOXSEL : UI_BOX;
        char b1[24], b2[24];
        fb_fill(fb, 8, 3 + i, 64, 1, ' ', at);
        fb_text(fb, 9, 3 + i, loc_get(kb_action_key(a)), at, 30);
        kb_format(&g_binds[a][0], b1, sizeof(b1));
        kb_format(&g_binds[a][1], b2, sizeof(b2));
        fb_text(fb, 42, 3 + i, b1[0] ? b1 : "-", at, 14);
        fb_text(fb, 58, 3 + i, b2[0] ? b2 : "-", at, 14);
    }
    if (g_kb_wait) {
        sprintf(buf, "%s (%d)", loc_get("kb.press"), g_kb_slot + 1);
        fb_text(fb, 8, 21, buf, ATTR(C_YELLOW, C_BLUE), 60);
    } else if (!(g_app.caps & TERMCAP_RSHIFT)) {
        fb_text(fb, 8, 21, loc_get("kb.no_bare_mods"), ATTR(C_LIGHTGRAY, C_BLUE), 60);
    }
    ui_hint(fb, loc_get("hint.keys"));
}

static void keybinds_key(const ClideKeyEvent *ev)
{
    if (g_kb_wait) {
        KeySpec k;
        k.key = ev->key >= 'A' && ev->key <= 'Z' ? ev->key - 'A' + 'a' : ev->key;
        k.mods = ev->mods & (KM_CTRL | KM_ALT);
        if (ev->key == CK_ESC && g_kb_sel != ACT_PAUSE) {
            g_kb_wait = 0;
            return;
        }
        g_binds[g_kb_sel][g_kb_slot] = k;
        g_kb_wait = 0;
        kb_save("keys.ini");
        return;
    }
    if (ev->key == CK_ESC) {
        kb_save("keys.ini");
        app_pop();
        return;
    }
    if (ev->key == CK_UP && g_kb_sel > 1) {
        --g_kb_sel;
    } else if (ev->key == CK_DOWN && g_kb_sel < ACT_COUNT - 1) {
        ++g_kb_sel;
    } else if (ev->key == CK_ENTER) {
        g_kb_wait = 1;
        g_kb_slot = 0;
    } else if (ev->key == 'a' || ev->key == 'A' || ev->key == CK_SPACE) {
        g_kb_wait = 1;
        g_kb_slot = 1;
    } else if (ev->key == CK_DELETE || ev->key == CK_BACKSPACE) {
        g_binds[g_kb_sel][1].key = CK_NONE;
    } else if (ev->key == 'r' || ev->key == 'R') {
        kb_defaults();
    }
    if (g_kb_sel < g_kb_top) {
        g_kb_top = g_kb_sel;
    }
    if (g_kb_sel >= g_kb_top + 17) {
        g_kb_top = g_kb_sel - 16;
    }
}

const Screen scr_keybinds = { "keybinds", 1, 1, keybinds_enter, keybinds_key, keybinds_draw };
