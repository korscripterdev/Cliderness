/* Cliderness - ui/scr_menu.c : title menu, new world, load world, credits, message box */
#include <stdio.h>
#include <string.h>
#include "ui/app.h"
#include "ui/widgets.h"
#include "gameplay/game.h"
#include "localization/locale.h"
#include "renderer/cp437.h"
#include "audio/audio.h"
#include "save/save.h"
#include "core/rng.h"
#include "core/str.h"
#include "core/version.h"

/* ------------------------------------------------------------ title */

static int g_menu_sel = 0;

static const char *const g_logo[5] = {
    "#### #    ### ###  #### ###  #  # #### #### ####",
    "#    #     #  #  # #    #  # ## # #    #    #   ",
    "#    #     #  #  # ###  ###  # ## ###  #### ####",
    "#    #     #  #  # #    # #  #  # #       #    #",
    "#### #### ### ###  #### #  # #  # #### #### ####"
};

static void draw_logo_bytes(Framebuffer *fb, int y)
{
    int row, i;
    for (row = 0; row < 5; ++row) {
        const char *s = g_logo[row];
        int len = (int)strlen(s);
        int x = (FB_W - len) / 2;
        for (i = 0; i < len; ++i) {
            if (s[i] == '#') {
                int col = row < 2 ? C_LIGHTGREEN : (row < 4 ? C_GREEN : C_BROWN);
                fb_put(fb, x + i, y + row, G_BLOCK, ATTR(col, C_BLACK));
            }
        }
    }
}

static void menu_draw_bg(Framebuffer *fb)
{
    int x;
    long t = (long)g_app.frame;
    /* drifting landscape strip (UI animation keeps running while paused) */
    for (x = 0; x < FB_W; ++x) {
        cl_u32 h = rng_hash3(7UL, (x + t / 8L) / 3L, 0, 0);
        int tree = (h % 5UL) == 0UL;
        fb_put(fb, x, 21, tree ? G_SPADE : ((h % 3UL) ? '.' : G_YEN), ATTR(tree ? C_GREEN : C_LIGHTGREEN, C_BLACK));
        fb_put(fb, x, 22, G_SHADE2, ATTR(C_BROWN, C_BLACK));
        fb_put(fb, x, 23, '~', ATTR(((x + t / 4L) % 5L) ? C_BLUE : C_LIGHTBLUE, C_BLACK));
    }
}

static void title_draw(Framebuffer *fb)
{
    const char *items[6];
    items[0] = loc_get("menu.new_world");
    items[1] = loc_get("menu.load_world");
    items[2] = loc_get("menu.settings");
    items[3] = loc_get("menu.help");
    items[4] = loc_get("menu.credits");
    items[5] = loc_get("menu.exit");
    fb_box(fb, 0, 0, FB_W, FB_H - 1, ATTR(C_DARKGRAY, C_BLACK), 0);
    draw_logo_bytes(fb, 2);
    fb_text_center(fb, 8, CLIDE_NAME, ATTR(C_WHITE, C_BLACK));
    fb_text_center(fb, 9, loc_get("menu.subtitle"), ATTR(C_LIGHTCYAN, C_BLACK));
    menu_draw(fb, 25, 11, 30, items, 6, g_menu_sel, UI_TEXT, UI_SEL);
    menu_draw_bg(fb);
    fb_text_center(fb, 18, loc_fmt("menu.created_by", CLIDE_CREATOR, NULL, NULL), ATTR(C_YELLOW, C_BLACK));
    fb_text(fb, 2, 19, "v" CLIDE_VERSION, UI_DIM, 0);
    ui_hint(fb, loc_get("hint.menu"));
}

static void title_enter(void)
{
    g_menu_sel = 0;
}

static void title_key(const ClideKeyEvent *ev)
{
    if (ev->key == CK_F1 || ev->key == '?') {
        app_open_help("start");
        return;
    }
    if (ev->key == CK_ESC) {
        g_menu_sel = 5;
        return;
    }
    if (!menu_key(&g_menu_sel, 6, ev)) {
        return;
    }
    aud_play(SFX_MENU);
    switch (g_menu_sel) {
    case 0: app_push(&scr_newworld); break;
    case 1: app_push(&scr_loadworld); break;
    case 2: app_push(&scr_settings); break;
    case 3: app_open_help("start"); break;
    case 4: app_push(&scr_credits); break;
    default: g_app.running = 0; break;
    }
}

const Screen scr_menu = { "menu", 1, 1, title_enter, title_key, title_draw };

/* ------------------------------------------------------------ credits */

static void credits_draw(Framebuffer *fb)
{
    ui_frame(fb, 10, 3, 60, 18, loc_get("menu.credits"), UI_BOX);
    fb_text_center(fb, 5, CLIDE_NAME " " CLIDE_VERSION, ATTR(C_WHITE, C_BLUE));
    fb_text_center(fb, 6, loc_get("menu.subtitle"), ATTR(C_LIGHTCYAN, C_BLUE));
    fb_text_center(fb, 8, loc_get("credits.created_by"), ATTR(C_LIGHTGRAY, C_BLUE));
    fb_text_center(fb, 9, CLIDE_CREATOR, ATTR(C_YELLOW, C_BLUE));
    fb_text_center(fb, 11, loc_get("credits.developer"), ATTR(C_LIGHTGRAY, C_BLUE));
    fb_text_center(fb, 12, CLIDE_DEVELOPER, ATTR(C_YELLOW, C_BLUE));
    fb_text_center(fb, 14, CLIDE_ENGINE " (ISO C90)", ATTR(C_LIGHTGRAY, C_BLUE));
    fb_text_center(fb, 15, loc_get("credits.line1"), ATTR(C_LIGHTGRAY, C_BLUE));
    fb_text_center(fb, 16, loc_get("credits.line2"), ATTR(C_LIGHTGRAY, C_BLUE));
    fb_text_center(fb, 18, loc_fmt("credits.platform", g_app.p->name, NULL, NULL), ATTR(C_DARKGRAY, C_BLUE));
    ui_hint(fb, loc_get("hint.back"));
}

static void credits_key(const ClideKeyEvent *ev)
{
    if (ev->key == CK_ESC || ev->key == CK_ENTER || ev->key == CK_SPACE) {
        app_pop();
    }
}

const Screen scr_credits = { "credits", 1, 0, NULL, credits_key, credits_draw };

/* ------------------------------------------------------------ message box */

static char g_msg_title[64];
static char g_msg_text[256];

void app_message(const char *title, const char *text)
{
    str_copy(g_msg_title, title, sizeof(g_msg_title));
    str_copy(g_msg_text, text, sizeof(g_msg_text));
    app_push(&scr_message);
}

static void message_draw(Framebuffer *fb)
{
    ui_frame(fb, 12, 8, 56, 8, g_msg_title, ATTR(C_WHITE, C_RED));
    ui_draw_wrapped(fb, 14, 10, 52, 4, g_msg_text, 0, ATTR(C_WHITE, C_RED));
    fb_text(fb, 14, 14, loc_get("hint.ok"), ATTR(C_YELLOW, C_RED), 52);
}

static void message_key(const ClideKeyEvent *ev)
{
    if (ev->key == CK_ESC || ev->key == CK_ENTER || ev->key == CK_SPACE) {
        app_pop();
    }
}

const Screen scr_message = { "message", 1, 0, NULL, message_key, message_draw };

/* ------------------------------------------------------------ new world */

static TextInput g_nw_name, g_nw_seed;
static int g_nw_field = 0;
static int g_nw_mode = GM_SURVIVAL;
static int g_nw_cheats = 0;
static char g_nw_err[128];

static void newworld_enter(void)
{
    char seed[16];
    SaveList l;
    char name[12];
    int n = 1;
    save_list(&l);
    do {
        sprintf(name, "world%d", n++);
    } while (save_exists(name) && n < 1000);
    ti_set(&g_nw_name, name, 8, 1);
    sprintf(seed, "%lu", (unsigned long)(g_app.p->clock.wall_seed() % 1000000000UL));
    ti_set(&g_nw_seed, seed, 10, 1);
    g_nw_field = 0;
    g_nw_mode = GM_SURVIVAL;
    g_nw_cheats = 0;
    g_nw_err[0] = '\0';
}

static cl_u32 parse_seed(const char *s)
{
    unsigned long v = 0;
    int digits = 1;
    const char *p;
    if (!*s) {
        return g_app.p->clock.wall_seed();
    }
    for (p = s; *p; ++p) {
        if (*p < '0' || *p > '9') {
            digits = 0;
        }
    }
    if (!digits) {
        return rng_hash_str(s);   /* text seeds are hashed */
    }
    for (p = s; *p; ++p) {
        v = v * 10UL + (unsigned long)(*p - '0');
    }
    return CL_U32(v);
}

static void newworld_draw(Framebuffer *fb)
{
    int i;
    const char *labels[6];
    char val[64];
    labels[0] = loc_get("nw.name");
    labels[1] = loc_get("nw.seed");
    labels[2] = loc_get("nw.mode");
    labels[3] = loc_get("nw.cheats");
    labels[4] = loc_get("nw.create");
    labels[5] = loc_get("nw.back");
    ui_frame(fb, 12, 4, 56, 16, loc_get("menu.new_world"), UI_BOX);
    for (i = 0; i < 6; ++i) {
        cl_u8 a = (i == g_nw_field) ? UI_BOXSEL : UI_BOX;
        int y = 6 + i * 2;
        fb_fill(fb, 14, y, 52, 1, ' ', a);
        fb_text(fb, 15, y, labels[i], a, 18);
        val[0] = '\0';
        if (i == 0) {
            str_copy(val, g_nw_name.buf, sizeof(val));
            if (i == g_nw_field && (g_app.frame / 8) % 2) str_cat(val, "_", sizeof(val));
        } else if (i == 1) {
            str_copy(val, g_nw_seed.buf, sizeof(val));
            if (i == g_nw_field && (g_app.frame / 8) % 2) str_cat(val, "_", sizeof(val));
        } else if (i == 2) {
            str_copy(val, "< ", sizeof(val));
            str_cat(val, loc_get(game_mode_key(g_nw_mode)), sizeof(val));
            str_cat(val, " >", sizeof(val));
        } else if (i == 3) {
            str_copy(val, "< ", sizeof(val));
            str_cat(val, loc_get(g_nw_cheats ? "con.on" : "con.off"), sizeof(val));
            str_cat(val, " >", sizeof(val));
        }
        fb_text(fb, 34, y, val, a, 31);
    }
    if (g_nw_err[0]) {
        fb_text(fb, 14, 18, g_nw_err, ATTR(C_LIGHTRED, C_BLUE), 52);
    } else {
        fb_text(fb, 14, 18, loc_get("nw.name_rule"), ATTR(C_LIGHTGRAY, C_BLUE), 52);
    }
    ui_hint(fb, loc_get("hint.newworld"));
}

static void newworld_key(const ClideKeyEvent *ev)
{
    if (ev->key == CK_ESC) {
        app_pop();
        return;
    }
    if (ev->key == CK_UP) {
        g_nw_field = (g_nw_field + 5) % 6;
        return;
    }
    if (ev->key == CK_DOWN || ev->key == CK_TAB) {
        g_nw_field = (g_nw_field + 1) % 6;
        return;
    }
    if (g_nw_field == 0 && ti_key(&g_nw_name, ev)) {
        return;
    }
    if (g_nw_field == 1 && ti_key(&g_nw_seed, ev)) {
        return;
    }
    if (g_nw_field == 2 && (ev->key == CK_LEFT || ev->key == CK_RIGHT || ev->key == CK_ENTER || ev->key == CK_SPACE)) {
        g_nw_mode = (g_nw_mode + (ev->key == CK_LEFT ? GM_COUNT - 1 : 1)) % GM_COUNT;
        if (g_nw_mode != GM_SURVIVAL && g_nw_mode != GM_ADVENTURE) {
            g_nw_cheats = 1;
        }
        return;
    }
    if (g_nw_field == 3 && (ev->key == CK_LEFT || ev->key == CK_RIGHT || ev->key == CK_ENTER || ev->key == CK_SPACE)) {
        g_nw_cheats = !g_nw_cheats;
        return;
    }
    if (ev->key == CK_ENTER) {
        if (g_nw_field == 5) {
            app_pop();
            return;
        }
        if (g_nw_field < 2 || g_nw_field == 4) {
            if (!app_start_new(g_nw_name.buf, parse_seed(g_nw_seed.buf), g_nw_mode, g_nw_cheats,
                               g_nw_err, sizeof(g_nw_err))) {
                aud_play(SFX_HURT);
            }
        }
    }
}

const Screen scr_newworld = { "newworld", 1, 1, newworld_enter, newworld_key, newworld_draw };

/* ------------------------------------------------------------ load world */

static SaveList g_saves;
static int g_ls_sel = 0;
static char g_ls_err[128];

static void loadworld_enter(void)
{
    save_list(&g_saves);
    g_ls_sel = 0;
    g_ls_err[0] = '\0';
}

static void loadworld_draw(Framebuffer *fb)
{
    int i;
    ui_frame(fb, 16, 3, 48, 19, loc_get("menu.load_world"), UI_BOX);
    if (g_saves.count == 0) {
        fb_text_center(fb, 10, loc_get("lw.none"), ATTR(C_LIGHTGRAY, C_BLUE));
    }
    for (i = 0; i < g_saves.count && i < 14; ++i) {
        cl_u8 a = (i == g_ls_sel) ? UI_BOXSEL : UI_BOX;
        fb_fill(fb, 18, 5 + i, 44, 1, ' ', a);
        fb_text(fb, 20, 5 + i, g_saves.names[i], a, 40);
    }
    if (g_ls_err[0]) {
        fb_text(fb, 18, 20, g_ls_err, ATTR(C_LIGHTRED, C_BLUE), 44);
    }
    ui_hint(fb, loc_get("hint.load"));
}

static void loadworld_key(const ClideKeyEvent *ev)
{
    if (ev->key == CK_ESC) {
        app_pop();
        return;
    }
    if (menu_key(&g_ls_sel, g_saves.count, ev) && g_saves.count > 0) {
        if (!app_load_world(g_saves.names[g_ls_sel], g_ls_err, sizeof(g_ls_err))) {
            aud_play(SFX_HURT);
        }
    }
}

const Screen scr_loadworld = { "loadworld", 1, 1, loadworld_enter, loadworld_key, loadworld_draw };
