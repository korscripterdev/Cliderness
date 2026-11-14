/* Cliderness - ui/app.c : start-up, main loop, screen stack, settings */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/app.h"
#include "ui/widgets.h"
#include "gameplay/game.h"
#include "content/content.h"
#include "world/biomes/biome.h"
#include "world/weather/weather.h"
#include "world/structures/structure.h"
#include "gameplay/entities/creature.h"
#include "gameplay/vehicles/ship.h"
#include "localization/locale.h"
#include "commands/console.h"
#include "help/help.h"
#include "input/keybind.h"
#include "audio/audio.h"
#include "audio/dsp.h"
#include "save/save.h"
#include "core/ini.h"
#include "core/log.h"
#include "core/mem.h"
#include "core/str.h"
#include "core/version.h"

App g_app;

#define SETTINGS_FILE "clide.ini"
#define KEYS_FILE     "keys.ini"

#ifdef CLIDE_TARGET_DOS
#define FRAME_MS 55
#else
#define FRAME_MS 33
#endif

/* ------------------------------------------------------------ stack */

void app_push(const Screen *s)
{
    if (g_app.depth < APP_STACK) {
        g_app.stack[g_app.depth++] = s;
        if (s->enter) {
            s->enter();
        }
    }
}

void app_pop(void)
{
    if (g_app.depth > 1) {
        --g_app.depth;
    }
}

void app_replace(const Screen *s)
{
    if (g_app.depth > 0) {
        --g_app.depth;
    }
    app_push(s);
}

void app_reset(const Screen *s)
{
    g_app.depth = 0;
    app_push(s);
}

const Screen *app_top(void)
{
    return g_app.depth > 0 ? g_app.stack[g_app.depth - 1] : NULL;
}

int app_on_stack(const Screen *s)
{
    int i;
    for (i = 0; i < g_app.depth; ++i) {
        if (g_app.stack[i] == s) {
            return 1;
        }
    }
    return 0;
}

int app_paused(void)
{
    int i;
    for (i = 0; i < g_app.depth; ++i) {
        if (g_app.stack[i]->pauses) {
            return 1;
        }
    }
    return 0;
}

void app_open_help(const char *context)
{
    str_copy(g_app.help_context, context ? context : "", sizeof(g_app.help_context));
    app_push(&scr_help);
}

/* ------------------------------------------------------------ settings */

static int settings_cb(void *user, const char *type, const char *id, const char *key,
                       const char *v, int line)
{
    long n;
    (void)user;
    (void)id;
    (void)line;
    if (!key) {
        return 0;
    }
    if (str_ieq(type, "settings")) {
        if (str_ieq(key, "language")) str_copy(g_app.set.lang, v, sizeof(g_app.set.lang));
        else if (str_ieq(key, "language_confirmed")) str_copy(g_app.set.lang_confirmed, v, sizeof(g_app.set.lang_confirmed));
        else if (str_ieq(key, "audio") && str_to_long(v, &n)) g_app.set.audio = (int)n;
        else if (str_ieq(key, "day_length") && str_to_long(v, &n) && n >= 60) g_app.set.day_length = n;
        else if (str_ieq(key, "panel_page") && str_to_long(v, &n)) g_app.set.panel_page = (int)n;
    } else if (str_ieq(type, "help")) {
        char buf[400];
        char *tok;
        str_copy(buf, v, sizeof(buf));
        if (str_ieq(key, "favorites")) {
            g_help_nfav = 0;
            for (tok = strtok(buf, ","); tok && g_help_nfav < HELP_MAX_FAV; tok = strtok(NULL, ",")) {
                str_copy(g_help_fav[g_help_nfav++], str_trim(tok), 24);
            }
        } else if (str_ieq(key, "recent")) {
            g_help_nrecent = 0;
            for (tok = strtok(buf, ","); tok && g_help_nrecent < HELP_MAX_RECENT; tok = strtok(NULL, ",")) {
                str_copy(g_help_recent[g_help_nrecent++], str_trim(tok), 24);
            }
        }
    }
    return 0;
}

void settings_load(void)
{
    g_app.set.lang[0] = '\0';
    g_app.set.lang_confirmed[0] = '\0';
    g_app.set.audio = 1;
    g_app.set.day_length = 1200;
    g_app.set.panel_page = 0;
    if (g_app.caps & TERMCAP_HEADLESS) {
        return;   /* tests never depend on the user's settings */
    }
    ini_parse_file(SETTINGS_FILE, settings_cb, NULL);
}

void settings_save(void)
{
    FILE *f;
    int i;
    if (g_app.caps & TERMCAP_HEADLESS) {
        return;
    }
    f = fopen(SETTINGS_FILE, "w");
    if (!f) {
        return;
    }
    fprintf(f, "# Cliderness settings - created by korscripterdev\n[settings]\n");
    fprintf(f, "language=%s\n", loc_current());
    fprintf(f, "language_confirmed=%s\n", g_app.set.lang_confirmed);
    fprintf(f, "audio=%d\n", g_app.set.audio);
    fprintf(f, "day_length=%ld\n", g_app.set.day_length);
    fprintf(f, "panel_page=%d\n", g_app.set.panel_page);
    fprintf(f, "[help]\nfavorites=");
    for (i = 0; i < g_help_nfav; ++i) {
        fprintf(f, "%s%s", i ? "," : "", g_help_fav[i]);
    }
    fprintf(f, "\nrecent=");
    for (i = 0; i < g_help_nrecent; ++i) {
        fprintf(f, "%s%s", i ? "," : "", g_help_recent[i]);
    }
    fprintf(f, "\n");
    fclose(f);
}

static int time_cb(void *user, const char *type, const char *id, const char *key,
                   const char *v, int line)
{
    ClideTimeCfg *c = (ClideTimeCfg *)user;
    long n;
    (void)type;
    (void)id;
    (void)line;
    if (!key || !str_to_long(v, &n)) {
        if (key && str_ieq(key, "time_scale")) {
            /* "1.0" style */
            long whole = 0, frac = 0;
            const char *dot = strchr(v, '.');
            char tmp[8];
            if (dot && (size_t)(dot - v) < sizeof(tmp)) {
                memcpy(tmp, v, (size_t)(dot - v));
                tmp[dot - v] = '\0';
                str_to_long(tmp, &whole);
                str_copy(tmp, dot + 1, 3);
                str_to_long(tmp, &frac);
                if (strlen(tmp) == 1) {
                    frac *= 10;
                }
                c->scale_pct = whole * 100 + frac;
            }
        }
        return 0;
    }
    if (str_ieq(key, "real_seconds_per_game_day")) c->real_seconds_per_day = n;
    else if (str_ieq(key, "days_per_year")) c->days_per_year = n;
    else if (str_ieq(key, "days_per_season")) c->days_per_season = n;
    else if (str_ieq(key, "time_scale")) c->scale_pct = n * 100;
    return 0;
}

/* ------------------------------------------------------------ sessions */

int app_start_new(const char *name, cl_u32 seed, int mode, int cheats, char *err, size_t n)
{
    char dir[96];
    ClideTimeCfg tc = g_app.tcfg;
    if (!save_valid_name(name)) {
        str_copy(err, loc_get("err.bad_name"), n);
        return 0;
    }
    if (save_exists(name)) {
        str_copy(err, loc_get("err.world_exists"), n);
        return 0;
    }
    if (!save_prepare(g_app.p, name)) {
        str_copy(err, loc_get("err.mkdir"), n);
        return 0;
    }
    save_world_dir(name, dir, sizeof(dir));
    tc.real_seconds_per_day = g_app.set.day_length;
    if (!game_create(&g_game, name, dir, seed, mode, cheats, &tc)) {
        str_copy(err, loc_get("err.out_of_memory"), n);
        return 0;
    }
    g_game.world.replace = g_app.p->fs.replace;
    log_clear();
    log_add(UI_TITLE, loc_fmt("msg.welcome", CLIDE_NAME, CLIDE_CREATOR, NULL));
    log_add(UI_TEXT, loc_get("msg.help_hint"));
    g_app.accum = 0;
    app_save_world();
    app_reset(&scr_game);
    return 1;
}

int app_load_world(const char *name, char *err, size_t n)
{
    if (!save_load(&g_game, g_app.p, name, &g_app.tcfg, err, n)) {
        return 0;
    }
    log_clear();
    log_add(UI_TITLE, loc_fmt("msg.welcome_back", name, NULL, NULL));
    g_app.accum = 0;
    app_reset(&scr_game);
    return 1;
}

int app_save_world(void)
{
    char err[96];
    if (!g_game.active) {
        return 0;
    }
    if (save_game(&g_game, g_app.p, err, sizeof(err))) {
        log_add(UI_GOOD, loc_get("msg.saved"));
        return 1;
    }
    log_add(UI_BAD, loc_fmt("msg.save_failed", err, NULL, NULL));
    return 0;
}

void app_close_world(int save)
{
    if (!g_game.active) {
        return;
    }
    if (save) {
        app_save_world();
    }
    game_destroy(&g_game);
}

/* ------------------------------------------------------------ main loop */

static void draw_all(void)
{
    int i, start = 0;
    int tw, th;
    for (i = g_app.depth - 1; i >= 0; --i) {
        if (g_app.stack[i]->opaque) {
            start = i;
            break;
        }
    }
    fb_clear(&g_app.back, ATTR(C_LIGHTGRAY, C_BLACK));
    for (i = start; i < g_app.depth; ++i) {
        g_app.stack[i]->draw(&g_app.back);
    }
    g_app.p->term.get_size(&tw, &th);
    if (tw < FB_W || th < FB_H) {
        char buf[80];
        sprintf(buf, " %dx%d < 80x25 ", tw, th);
        fb_text(&g_app.back, 0, 0, buf, ATTR(C_WHITE, C_RED), 0);
    }
}

static void handle_key(const ClideKeyEvent *ev)
{
    const Screen *top = app_top();
    if (ev->key == CK_RESIZE) {
        g_app.force = 1;
        return;
    }
    if (ev->key == CK_QUIT) {
        if (g_game.active) {
            app_close_world(1);
        }
        g_app.running = 0;
        return;
    }
    if (top && top->key) {
        top->key(ev);
    }
}

static void loop(void)
{
    const ClidePlatform *p = g_app.p;
    cl_u32 last = p->clock.ms();
    int headless = (g_app.caps & TERMCAP_HEADLESS) != 0;
    g_app.running = 1;
    g_app.force = 1;
    while (g_app.running) {
        ClideKeyEvent ev;
        cl_u32 now = p->clock.ms();
        long dt = (long)CL_U32(now - last);
        last = now;
        if (dt > 250) {
            dt = 250;   /* never replay long stalls */
        }
        while (p->input.poll(&ev)) {
            handle_key(&ev);
            if (!g_app.running) {
                break;
            }
        }
        if (!g_app.running) {
            break;
        }
        if (g_game.active && !app_paused()) {
            int guard = 0;
            g_app.accum += dt;
            while (g_app.accum >= TICK_MS && guard < 10) {
                game_tick(&g_game);
                g_app.accum -= TICK_MS;
                ++guard;
            }
            if (guard >= 10) {
                g_app.accum = 0;
            }
        } else {
            g_app.accum = 0;   /* paused real time is discarded */
        }
        aud_update(dt);
        ++g_app.frame;
        draw_all();
        p->term.present(&g_app.back, &g_app.front, g_app.force);
        g_app.force = 0;
        if (headless) {
            p->clock.sleep_ms(TICK_MS);
            if (headless_done() && --g_app.headless_frames <= 0) {
                break;
            }
        } else {
            cl_u32 used = CL_U32(p->clock.ms() - now);
            if (used < FRAME_MS) {
                p->clock.sleep_ms(FRAME_MS - used);
            }
        }
    }
}

/* ------------------------------------------------------------ start-up */

static int file_exists(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

static void find_data(const char *argv0, const char *override)
{
    char path[160];
    if (override) {
        str_copy(g_data_root, override, sizeof(g_data_root));
        return;
    }
    if (file_exists("data/game.ini")) {
        str_copy(g_data_root, "data", sizeof(g_data_root));
        return;
    }
    if (argv0) {
        char *slash;
        str_copy(path, argv0, sizeof(path));
        slash = strrchr(path, '/');
        if (!slash) {
            slash = strrchr(path, '\\');
        }
        if (slash) {
            slash[1] = '\0';
            str_cat(path, "data", sizeof(path));
            {
                char test[180];
                str_copy(test, path, sizeof(test));
                str_cat(test, "/game.ini", sizeof(test));
                if (file_exists(test)) {
                    str_copy(g_data_root, path, sizeof(g_data_root));
                    return;
                }
            }
        }
    }
    if (file_exists("../data/game.ini")) {
        str_copy(g_data_root, "../data", sizeof(g_data_root));
        return;
    }
    str_copy(g_data_root, "data", sizeof(g_data_root));
}

static void usage(void)
{
    printf("%s %s - %s\nCreated by %s\n\n", CLIDE_NAME, CLIDE_VERSION, CLIDE_GENRE, CLIDE_CREATOR);
    printf("usage: cliderness [options]\n");
    printf("  --version            print version and creator\n");
    printf("  --help               this text\n");
    printf("  --data <dir>         data directory (default: ./data)\n");
    printf("  --lang <code>        force a language (en, ko, ja, ...)\n");
    printf("  --headless <script>  run with scripted input and print the final screen\n");
    printf("  --quiet              with --headless: do not print the final screen\n");
    printf("  --list-commands      print the console command registry\n");
    printf("  --check-lang         report missing translations\n");
    printf("  --render-sfx <name> <file.wav>  render a procedural sound to WAV\n");
    printf("  --list-sfx           list procedural sounds\n");
    printf("  --play-sfx <effect>  play a game sound effect on the audio device\n");
}

static int load_content(char *err, size_t n)
{
    char path[160];
    if (!content_load(g_data_root, err, n)) {
        return 0;
    }
    if (!biome_load(g_data_root, err, n)) {
        return 0;
    }
    if (!weather_load(g_data_root, err, n)) {
        return 0;
    }
    if (!structures_load(g_data_root, err, n)) {
        return 0;
    }
    if (!creatures_load(g_data_root, err, n)) {
        return 0;
    }
    if (!ships_load(g_data_root, err, n)) {
        return 0;
    }
    ctime_default_cfg(&g_app.tcfg);
    str_copy(path, g_data_root, sizeof(path));
    str_cat(path, "/config/time.ini", sizeof(path));
    ini_parse_file(path, time_cb, &g_app.tcfg);
    return 1;
}

/* play one game sound effect on the live audio back end (audio check) */
static int play_sfx(const char *name)
{
    const ClidePlatform *p = plat_native();
    int sfx = aud_find(name);
    long t;
    if (sfx < 0) {
        int k;
        fprintf(stderr, "unknown sound effect: %s\neffects:", name);
        for (k = 1; k < SFX_COUNT; ++k) {
            fprintf(stderr, " %s", aud_sfx_name(k));
        }
        fprintf(stderr, "\n");
        return 1;
    }
    aud_init(p, 1);
    printf("audio back end: %s\n", aud_backend());
    aud_play(sfx);
    for (t = 0; t < 2000; t += 20) {
        p->clock.sleep_ms(20);
        aud_update(20);
    }
    printf("%s: %s\n", name, aud_played() > 0 ? "played" : "not played (no audio device)");
    aud_shutdown();
    return aud_played() > 0 ? 0 : 3;
}

static int render_sfx(const char *name, const char *file)
{
    long rate = 22050, n = rate * 3;
    short *buf = (short *)mem_alloc(sizeof(short) * (size_t)n);
    int ok;
    if (!buf) {
        return 1;
    }
    if (dsp_render(name, buf, n, rate, 12345UL) != n) {
        fprintf(stderr, "unknown sound: %s\n", name);
        mem_free(buf);
        return 1;
    }
    ok = dsp_write_wav(file, buf, n, rate);
    mem_free(buf);
    printf("%s -> %s (%ld samples @ %ld Hz)\n", name, file, n, rate);
    return ok ? 0 : 1;
}

int app_main(int argc, char **argv)
{
    char err[200];
    const char *data_override = NULL, *lang_override = NULL, *script = NULL;
    int i, list_cmds = 0, check_lang = 0, quiet = 0;
    memset(&g_app, 0, sizeof(g_app));
    for (i = 1; i < argc; ++i) {
        if (str_ieq(argv[i], "--version") || str_ieq(argv[i], "-v")) {
            printf("%s %s\n%s\nCreated by %s\nLicense: %s (see LICENSE)\n", CLIDE_NAME, CLIDE_VERSION,
                   CLIDE_ENGINE, CLIDE_CREATOR, CLIDE_LICENSE);
            return 0;
        } else if (str_ieq(argv[i], "--help") || str_ieq(argv[i], "-h") || str_ieq(argv[i], "/?")) {
            usage();
            return 0;
        } else if (str_ieq(argv[i], "--data") && i + 1 < argc) {
            data_override = argv[++i];
        } else if (str_ieq(argv[i], "--lang") && i + 1 < argc) {
            lang_override = argv[++i];
        } else if (str_ieq(argv[i], "--headless") && i + 1 < argc) {
            script = argv[++i];
        } else if (str_ieq(argv[i], "--quiet")) {
            quiet = 1;
        } else if (str_ieq(argv[i], "--list-commands")) {
            list_cmds = 1;
        } else if (str_ieq(argv[i], "--check-lang")) {
            check_lang = 1;
        } else if (str_ieq(argv[i], "--list-sfx")) {
            int k;
            for (k = 0; dsp_sound_name(k); ++k) {
                printf("%s\n", dsp_sound_name(k));
            }
            return 0;
        } else if (str_ieq(argv[i], "--render-sfx") && i + 2 < argc) {
            return render_sfx(argv[i + 1], argv[i + 2]);
        } else if (str_ieq(argv[i], "--play-sfx") && i + 1 < argc) {
            return play_sfx(argv[i + 1]);
        } else {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            usage();
            return 2;
        }
    }
#ifdef CLIDE_TARGET_DOS16
    mem_set_budget(420000UL);
#endif
    find_data(argc > 0 ? argv[0] : NULL, data_override);
    g_app.p = script ? plat_headless() : plat_native();
    g_app.caps = g_app.p->term.caps();
    if (!loc_init(g_data_root, (list_cmds || check_lang) ? TERMCAP_UNICODE : g_app.caps)) {
        fprintf(stderr, "Cliderness: cannot load %s/locale/en.lng\n", g_data_root);
        return 1;
    }
    if (!load_content(err, sizeof(err))) {
        fprintf(stderr, "Cliderness: data error: %s\n", err);
        return 1;
    }
    con_init();
    if (list_cmds) {
        int k;
        for (k = 0; k < con_count(); ++k) {
            const Command *c = con_get(k);
            printf("/%-12s %-10s %-5s %s\n", c->name, c->category, c->perm ? "cheat" : "any", c->syntax);
        }
        printf("total top-level commands: %d (aliases: %d)\n", con_count(), con_alias_count());
        return 0;
    }
    if (check_lang) {
        static char missing[4096];
        int k, bad = 0;
        printf("English keys: %d\n", loc_key_count());
        for (k = 0; k < loc_lang_count(); ++k) {
            LangInfo *li = loc_lang(k);
            int full = str_ieq(li->code, "en") || str_ieq(li->code, "ko");
            int n = loc_missing(li->code, full ? NULL : "!cmd.", missing, sizeof(missing));
            int cmd = full ? 0 : loc_missing(li->code, "cmd.", NULL, 0);
            printf("%-6s %-22s missing %d  (command descriptions shown in English: %d)\n",
                   li->code, li->name, n, cmd);
            if (n > 0) {
                printf("%s", missing);
                bad = 1;
            }
        }
        return bad;
    }
    settings_load();
    if ((g_app.caps & TERMCAP_HEADLESS) || !kb_load(KEYS_FILE)) {
        kb_defaults();
    }
    {
        char det[16];
        const char *choice;
        if (lang_override) {
            choice = lang_override;
        } else if (g_app.set.lang[0]) {
            choice = g_app.set.lang;
        } else {
            g_app.p->locale.detect(det, sizeof(det));
            choice = loc_choose(det);
        }
        if (loc_find(choice) < 0 || loc_lang(loc_find(choice))->status == LANG_UNAVAILABLE || !loc_set(choice)) {
            loc_set("en");
        }
    }
    help_load(g_data_root, loc_current());
    if (script) {
        headless_reset();
        if (!headless_script(script)) {
            fprintf(stderr, "bad headless script\n");
            return 2;
        }
        g_app.headless_frames = 2;
    }
    if (!g_app.p->term.init()) {
        fprintf(stderr, "Cliderness: this program needs an interactive terminal.\n");
        return 1;
    }
    aud_init(g_app.p, g_app.set.audio);
    app_reset(&scr_menu);
    loop();
    if (g_game.active) {
        app_close_world(1);
    }
    aud_shutdown();
    g_app.p->term.shutdown();
    settings_save();
    if (script && !quiet) {
        headless_dump(stdout);
    }
    loc_shutdown();
    return 0;
}
