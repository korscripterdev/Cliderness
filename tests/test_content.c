/* Cliderness - tests/test_content.c : data files, localisation, help, key bindings */
#include <string.h>
#include "tf.h"
#include "content/content.h"
#include "world/biomes/biome.h"
#include "world/weather/weather.h"
#include "localization/locale.h"
#include "platform/platform.h"
#include "help/help.h"
#include "input/keybind.h"
#include "core/str.h"
#include "core/version.h"

static void t_registry(void)
{
    int i, ok = 1;
    printf("    %d blocks, %d items, %d biomes, %d weather types\n", g_block_count, g_item_count,
           g_biome_count, g_weather_count);
    CHECK(g_block_count >= 60);
    CHECK(g_item_count >= 80);
    CHECK(block_find("Cliderness:stone") == block_find("stone"));
    CHECK(item_find("cliderness:dirt") >= 0);
    CHECK(g_blocks[0].glyph == ' ' && str_ieq(g_blocks[0].id, "air"));
    for (i = 1; i < g_block_count; ++i) {
        const BlockDef *b = &g_blocks[i];
        if (b->drop_item >= g_item_count) ok = 0;
        if (b->fg > 15 || b->bg > 15 || b->floor_fg > 15 || b->floor_bg > 15) ok = 0;
    }
    CHECK(ok);
    CHECK(g_blocks[g_cb.stone].drop_item == item_find("cobblestone"));
    CHECK(g_items[item_find("oak_log")].density < 1000);
    CHECK(g_items[item_find("stone")].density > 1000);
    CHECK(str_ieq(CLIDE_CREATOR, "korscripterdev") && strcmp(CLIDE_CREATOR, "korscripterdev") == 0);
}

static void t_locale(void)
{
    static char missing[2048];
    int i;
    CHECK(loc_key_count() > 300);
    CHECK(loc_lang_count() >= 1);
    CHECK(strcmp(loc_get("menu.new_world"), "New World") == 0);
    CHECK(strcmp(loc_get("no.such.key"), "no.such.key") == 0);
    CHECK(strcmp(loc_fmt("menu.created_by", CLIDE_CREATOR, NULL, NULL), "Created by korscripterdev") == 0);
    for (i = 0; i < loc_lang_count(); ++i) {
        LangInfo *li = loc_lang(i);
        /* every UI string in every language; command descriptions in en/ko
         * (other languages fall back to English for newer commands) */
        int full = str_ieq(li->code, "en") || str_ieq(li->code, "ko");
        int ui = loc_missing(li->code, full ? NULL : "!cmd.", missing, sizeof(missing));
        int cmd = full ? 0 : loc_missing(li->code, "cmd.", NULL, 0);
        printf("    %-6s %-24s missing %d (command descriptions in English: %d)\n", li->code, li->native, ui, cmd);
        if (ui > 0) {
            printf("%s", missing);
        }
        CHECK(ui == 0);
    }
    /* switching languages and back */
    if (loc_find("ko") >= 0) {
        CHECK(loc_set("ko"));
        CHECK(strcmp(loc_get("menu.new_world"), "New World") != 0);
        CHECK(strcmp(loc_fmt("menu.created_by", CLIDE_CREATOR, NULL, NULL), "Created by korscripterdev") != 0);
        CHECK(strstr(loc_fmt("menu.created_by", CLIDE_CREATOR, NULL, NULL), "korscripterdev") != NULL);
        CHECK(loc_set("en"));
    }
}

static void t_locale_cp437(void)
{
    /* A CP437-only display (DOS) must hide languages it cannot draw. */
    int ko, es, en;
    CHECK(loc_init(tf_data, 0));
    {
        int i;
        printf("    CP437 display:");
        for (i = 0; i < loc_lang_count(); ++i) {
            printf(" %s=%s", loc_lang(i)->code, loc_lang(i)->status == LANG_OK ? "ok" : "hidden");
        }
        printf("\n");
    }
    en = loc_find("en");
    ko = loc_find("ko");
    es = loc_find("es");
    CHECK(en >= 0 && loc_lang(en)->status == LANG_OK);
    if (ko >= 0) CHECK(loc_lang(ko)->status == LANG_UNAVAILABLE);
    if (es >= 0) CHECK(loc_lang(es)->status == LANG_OK);
    CHECK(strcmp(loc_choose("ko"), "en") == 0);
    CHECK(loc_init(tf_data, TERMCAP_UNICODE));
    if (ko >= 0) CHECK(loc_lang(ko)->status == LANG_UNVERIFIED);
    if (ko >= 0) CHECK(strcmp(loc_choose("ko"), "ko") == 0);
    CHECK(strcmp(loc_choose("xx"), "en") == 0);
}

static void t_help(void)
{
    static const char *const required[] = {
        "start", "controls", "movement", "sprint", "crouch", "stamina", "survival", "gathering",
        "mining", "building", "crafting", "combat", "monsters", "items", "blocks", "inventory",
        "stats", "skills", "weather", "time", "sky", "caves", "ocean", "ancient", "ships",
        "durability", "cheats", "commands", "settings", "troubleshooting", "advanced", "pause"
    };
    static char text[HELP_TEXT_MAX];
    int i, ok = 1, levels[6];
    memset(levels, 0, sizeof(levels));
    CHECK(help_load(tf_data, "en"));
    printf("    %d help topics\n", g_help.count);
    for (i = 0; i < (int)(sizeof(required) / sizeof(required[0])); ++i) {
        if (help_find(required[i]) < 0) {
            printf("    missing help topic: %s\n", required[i]);
            ok = 0;
        }
    }
    CHECK(ok);
    ok = 1;
    for (i = 0; i < g_help.count; ++i) {
        char rel[80];
        char *tok;
        levels[g_help.t[i].level] = 1;
        if (!help_read(i, text, sizeof(text)) || strlen(text) < 40) {
            printf("    topic too short: %s\n", g_help.t[i].id);
            ok = 0;
        }
        str_copy(rel, g_help.t[i].related, sizeof(rel));
        for (tok = strtok(rel, ","); tok; tok = strtok(NULL, ",")) {
            if (help_find(str_trim(tok)) < 0) {
                printf("    %s: broken related link %s\n", g_help.t[i].id, tok);
                ok = 0;
            }
        }
    }
    CHECK(ok);
    CHECK(levels[1] && levels[2] && levels[3] && levels[4] && levels[5]);
    CHECK(help_matches(help_find("stamina"), "sprint"));
    help_toggle_fav("mining");
    CHECK(help_is_fav("mining"));
    help_toggle_fav("mining");
    CHECK(!help_is_fav("mining"));
    /* every translated help file has the same topics as English */
    for (i = 0; i < loc_lang_count(); ++i) {
        const char *code = loc_lang(i)->code;
        int k, ids_ok = 1, n_en;
        static char ids[HELP_MAX_TOPICS][24];
        help_load(tf_data, "en");
        n_en = g_help.count;
        for (k = 0; k < n_en; ++k) str_copy(ids[k], g_help.t[k].id, 24);
        help_load(tf_data, code);
        if (!str_ieq(g_help.lang, code)) {
            continue;   /* no translated help: English fallback */
        }
        for (k = 0; k < n_en; ++k) {
            if (help_find(ids[k]) < 0) {
                printf("    %s.hlp missing topic %s\n", code, ids[k]);
                ids_ok = 0;
            }
        }
        CHECK(ids_ok);
    }
    help_load(tf_data, "en");
}

static void t_keybinds(void)
{
    ClideKeyEvent ev;
    int sprint;
    KeySpec k;
    char buf[32];
    kb_defaults();
    ev.key = 'w'; ev.mods = 0;
    CHECK(kb_action(&ev, &sprint) == ACT_MOVE_N && !sprint);
    ev.key = 'W'; ev.mods = KM_SHIFT;
    CHECK(kb_action(&ev, &sprint) == ACT_MOVE_N && sprint);       /* Shift+W sprints */
    ev.key = 'q'; ev.mods = 0;
    CHECK(kb_action(&ev, NULL) == ACT_DROP);
    ev.key = 'q'; ev.mods = KM_CTRL;
    CHECK(kb_action(&ev, NULL) == ACT_DROP_STACK);                /* Ctrl+Q */
    ev.key = CK_RSHIFT; ev.mods = 0;
    CHECK(kb_action(&ev, NULL) == ACT_SKY);
    ev.key = 'v';
    CHECK(kb_action(&ev, NULL) == ACT_SKY);                       /* terminal fallback */
    ev.key = CK_CTRL;
    CHECK(kb_action(&ev, NULL) == ACT_CROUCH);
    ev.key = CK_F1;
    CHECK(kb_action(&ev, NULL) == ACT_HELP);
    ev.key = '?';
    CHECK(kb_action(&ev, NULL) == ACT_HELP);
    ev.key = CK_ESC;
    CHECK(kb_action(&ev, NULL) == ACT_PAUSE);
    ev.key = CK_TAB;
    CHECK(kb_action(&ev, NULL) == ACT_INVENTORY);
    ev.key = '/';
    CHECK(kb_action(&ev, NULL) == ACT_CONSOLE);
    ev.key = '5';
    CHECK(kb_action(&ev, NULL) == ACT_HOT5);
    CHECK(kb_parse("Ctrl+Q", &k) && k.key == 'q' && k.mods == KM_CTRL);
    kb_format(&k, buf, sizeof(buf));
    CHECK(strcmp(buf, "Ctrl+Q") == 0);
    CHECK(kb_parse("F12", &k) && k.key == CK_F12);
    /* rebinding persists through keys.ini */
    CHECK(kb_parse("K", &g_binds[ACT_MINE][0]));
    CHECK(kb_save("test_keys.ini"));
    kb_defaults();
    CHECK(kb_load("test_keys.ini"));
    ev.key = 'k'; ev.mods = 0;
    CHECK(kb_action(&ev, NULL) == ACT_MINE);
    kb_defaults();
    remove("test_keys.ini");
}

void suite_content(void)
{
    tf_run("block and item registry", t_registry);
    tf_run("translations complete", t_locale);
    tf_run("language availability on CP437 / Unicode", t_locale_cp437);
    tf_run("help index, links and levels", t_help);
    tf_run("key bindings", t_keybinds);
}
