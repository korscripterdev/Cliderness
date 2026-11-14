/*
 * Cliderness - tests/test_app.c
 * Drives the real application loop through the headless platform:
 * title screen, creator credit, new world, pausing with F1 / Esc,
 * silent (null audio) operation.
 */
#include <string.h>
#include "tf.h"
#include "ui/app.h"
#include "gameplay/game.h"
#include "platform/platform.h"
#include "audio/audio.h"
#include "core/str.h"

static int screen_contains(const char *needle)
{
    char row[FB_W * 4 + 1];
    int y;
    for (y = 0; y < FB_H; ++y) {
        fb_dump_row(headless_screen(), y, row, sizeof(row));
        if (strstr(row, needle)) {
            return 1;
        }
    }
    return 0;
}

static int run_app(const char *script)
{
    char *argv[7];
    argv[0] = (char *)"cliderness";
    argv[1] = (char *)"--data";
    argv[2] = (char *)tf_data;
    argv[3] = (char *)"--headless";
    argv[4] = (char *)script;
    argv[5] = (char *)"--quiet";
    argv[6] = NULL;
    return app_main(6, argv);
}

static void t_title(void)
{
    CHECK(run_app("{WAIT:3}") == 0);
    CHECK(screen_contains("Cliderness"));
    CHECK(screen_contains("Created by korscripterdev"));
    CHECK(screen_contains("New World"));
    CHECK(run_app("{DOWN}{DOWN}{DOWN}{DOWN}{ENTER}{WAIT:2}") == 0);   /* credits */
    CHECK(screen_contains("korscripterdev"));
    CHECK(!aud_available());          /* null audio device: game still runs */
}

/* Time only advances while no pausing screen is open. */
static void t_pause(void)
{
    long base, running, paused_help, paused_esc;
    /* new world, run 40 frames */
    CHECK(run_app("{ENTER}{WAIT:2}{ENTER}{WAIT:40}") == 0);
    base = g_game.time.ms;
    running = base - 8L * 3600000L;
    printf("    40 frames running: +%ld game ms\n", running);
    CHECK(running > 30L * 3600L);
    /* same, then F1 help open for 200 frames */
    CHECK(run_app("{ENTER}{WAIT:2}{ENTER}{WAIT:40}{F1}{WAIT:200}") == 0);
    paused_help = g_game.time.ms - 8L * 3600000L;
    CHECK(screen_contains("Cliderness Help"));
    /* same, then Esc pause menu for 200 frames */
    CHECK(run_app("{ENTER}{WAIT:2}{ENTER}{WAIT:40}{ESC}{WAIT:200}") == 0);
    paused_esc = g_game.time.ms - 8L * 3600000L;
    CHECK(screen_contains("Game time is frozen"));
    printf("    +200 frames in help: +%ld, in pause menu: +%ld game ms\n", paused_help, paused_esc);
    CHECK(paused_help <= running + 2L * 3600L);
    CHECK(paused_esc <= running + 2L * 3600L);
    /* resuming does not replay the paused time */
    CHECK(run_app("{ENTER}{WAIT:2}{ENTER}{WAIT:40}{ESC}{WAIT:200}{ESC}{WAIT:10}") == 0);
    CHECK(g_game.time.ms - 8L * 3600000L < running + 14L * 3600L);
    CHECK(g_game.time.ms - 8L * 3600000L > running);
}

static void t_play(void)
{
    /* walk, open inventory, console command, sky view */
    CHECK(run_app("{ENTER}{WAIT:2}{ENTER}{WAIT:5}d{WAIT:6}d{WAIT:6}{TAB}{WAIT:2}") == 0);
    CHECK(screen_contains("Inventory"));
    CHECK(run_app("{ENTER}{WAIT:2}{ENTER}{WAIT:5}/seed{ENTER}{WAIT:2}") == 0);
    CHECK(screen_contains("Seed: "));
    CHECK(run_app("{ENTER}{WAIT:2}{ENTER}{WAIT:5}{RSHIFT}{WAIT:5}") == 0);
    CHECK(screen_contains("constellation"));
    CHECK(run_app("{ENTER}{WAIT:2}{ENTER}{WAIT:5}c{WAIT:2}") == 0);
    CHECK(screen_contains("Attributes"));
}

void suite_app(void)
{
    tf_run("title screen shows the creator", t_title);
    tf_run("F1 / Esc freeze game time", t_pause);
    tf_run("play screens", t_play);
}
