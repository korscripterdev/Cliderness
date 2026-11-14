/*
 * Cliderness - tests/test_main.c
 * Test runner: loads the real data files and runs every suite.
 *   clide_tests [--data <dir>] [suite]
 */
#include <stdio.h>
#include <string.h>
#include "tf.h"
#include "platform/platform.h"
#include "content/content.h"
#include "world/biomes/biome.h"
#include "world/weather/weather.h"
#include "world/structures/structure.h"
#include "gameplay/entities/creature.h"
#include "gameplay/vehicles/ship.h"
#include "localization/locale.h"
#include "commands/console.h"
#include "help/help.h"
#include "gameplay/game.h"
#include "core/log.h"
#include "core/str.h"

int tf_pass = 0;
int tf_fail = 0;
int tf_case_fail = 0;
const char *tf_data = "data";

void tf_run(const char *name, TfFn fn)
{
    tf_case_fail = 0;
    fn();
    printf("  %-40s %s\n", name, tf_case_fail ? "FAILED" : "ok");
}

int tf_make_game(Game *g, int mode, const char *dir)
{
    ClideTimeCfg tc;
    ctime_default_cfg(&tc);
    if (!game_create(g, "test", dir, 12345UL, mode, 1, &tc)) {
        return 0;
    }
    /* most tests build ad-hoc pools and pits; the fluid tests enable flow */
    g->world.fluid.enabled = 0;
    /* creatures are spawned explicitly by the creature tests */
    g->spawning = 0;
    return 1;
}

/* a flat stone platform with open air above, radius r around (cx, cz) */
void tf_flat_area(Game *g, long cx, long y, long cz, int r)
{
    long x, z, yy;
    for (z = cz - r; z <= cz + r; ++z) {
        for (x = cx - r; x <= cx + r; ++x) {
            world_set(&g->world, 0, x, y - 1, z, (cl_u8)g_cb.stone, 0);
            world_set(&g->world, 0, x, y - 2, z, (cl_u8)g_cb.stone, 0);
            for (yy = y; yy < y + 6; ++yy) {
                world_set(&g->world, 0, x, yy, z, BLOCK_AIR, 0);
            }
        }
    }
    game_teleport(g, cx, y, cz);
}

static int setup(void)
{
    char err[200];
    str_copy(g_data_root, tf_data, sizeof(g_data_root));
    if (!loc_init(tf_data, TERMCAP_UNICODE)) {
        printf("cannot load locale from %s\n", tf_data);
        return 0;
    }
    if (!content_load(tf_data, err, sizeof(err)) || !biome_load(tf_data, err, sizeof(err)) ||
        !weather_load(tf_data, err, sizeof(err)) || !structures_load(tf_data, err, sizeof(err)) ||
        !creatures_load(tf_data, err, sizeof(err)) ||
        !ships_load(tf_data, err, sizeof(err))) {
        printf("data error: %s\n", err);
        return 0;
    }
    con_init();
    if (!help_load(tf_data, "en")) {
        printf("cannot load help\n");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    const char *only = NULL;
    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
            tf_data = argv[++i];
        } else {
            only = argv[i];
        }
    }
    printf("Cliderness test suite (data: %s)\n", tf_data);
    if (!setup()) {
        return 1;
    }
    if (!only || str_ieq(only, "core")) { printf("[core]\n"); suite_core(); }
    if (!only || str_ieq(only, "content")) { printf("[content]\n"); suite_content(); }
    if (!only || str_ieq(only, "world")) { printf("[world]\n"); suite_world(); }
    if (!only || str_ieq(only, "gameplay")) { printf("[gameplay]\n"); suite_gameplay(); }
    if (!only || str_ieq(only, "commands")) { printf("[commands]\n"); suite_commands(); }
    if (!only || str_ieq(only, "save")) { printf("[save]\n"); suite_save(); }
    if (!only || str_ieq(only, "crafting")) { printf("[crafting]\n"); suite_crafting(); }
    if (!only || str_ieq(only, "fluid")) { printf("[fluid]\n"); suite_fluid(); }
    if (!only || str_ieq(only, "mob")) { printf("[mob]\n"); suite_mob(); }
    if (!only || str_ieq(only, "ship")) { printf("[ship]\n"); suite_ship(); }
    if (!only || str_ieq(only, "ancient")) { printf("[ancient]\n"); suite_ancient(); }
    if (!only || str_ieq(only, "final")) { printf("[final]\n"); suite_final(); }
    if (!only || str_ieq(only, "app")) { printf("[app]\n"); suite_app(); }
    printf("\n%d checks passed, %d failed\n", tf_pass, tf_fail);
    return tf_fail ? 1 : 0;
}
