/* Cliderness - tests/test_final.c : Phase 10 - commands, PCM audio, limits */
#include <stdio.h>
#include <string.h>
#include "tf.h"
#include "gameplay/game.h"
#include "content/content.h"
#include "gameplay/entities/creature.h"
#include "commands/console.h"
#include "audio/audio.h"
#include "save/save.h"
#include "platform/platform.h"
#include "core/log.h"
#include "core/str.h"

static int run(Game *g, const char *line)
{
    return con_exec(g, plat_native(), line);
}

static void t_count(void)
{
    int i, bad = 0;
    printf("    %d commands, %d aliases\n", con_count(), con_alias_count());
    CHECK(con_count() >= 240);
    for (i = 0; i < con_count(); ++i) {
        if (!con_desc(con_get(i))[0]) {
            printf("    no description: %s\n", con_get(i)->name);
            bad = 1;
        }
    }
    CHECK(!bad);
}

/* every command runs without arguments and with a typical argument,
 * returning a defined result and never crashing */
static void t_smoke(void)
{
    static const char *const skip[] = { "quit", "save", NULL };
    static const char *const args[] = { "", " 2", " stone 2", " n", " on", " list", " deer", " 1 1", NULL };
    Game *g = &g_game;
    int i, a, k, bad = 0;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 100, 0, 10);
    g->p.god = 1;
    for (i = 0; i < con_count(); ++i) {
        const Command *cmd = con_get(i);
        int skipped = 0;
        for (k = 0; skip[k]; ++k) if (str_ieq(cmd->name, skip[k])) skipped = 1;
        if (skipped) continue;
        for (a = 0; args[a]; ++a) {
            char line[80];
            int r;
            sprintf(line, "/%s%s", cmd->name, args[a]);
            r = run(g, line);
            if (r != CMD_OK && r != CMD_USAGE && r != CMD_ERR && r != CMD_DENIED) {
                printf("    %s -> %d\n", line, r);
                bad = 1;
            }
            g_con_request = CON_REQ_NONE;
            if (g->p.dim != 0) portal_travel(g, 0, 0);
            if (g->aboard >= 0) g->aboard = -1;
            if (g->p.dead) game_respawn(g);
        }
        game_teleport(g, 0, 100, 0);
    }
    CHECK(!bad);
    game_destroy(g);
}

static void t_shapes(void)
{
    Game *g = &g_game;
    int st;
    CHECK(tf_make_game(g, GM_CREATIVE, NULL));
    tf_flat_area(g, 0, 100, 0, 12);
    st = block_find("cobblestone");
    g->p.facing = DIR_E;
    CHECK(run(g, "/pillar cobblestone 4") == CMD_OK);
    CHECK_EQ_L(world_get(&g->world, 0, 1, 103, 0), st);
    CHECK(run(g, "/undo") == CMD_OK);
    CHECK(world_get(&g->world, 0, 1, 103, 0) == BLOCK_AIR);
    CHECK(run(g, "/walls cobblestone 5 3") == CMD_OK);
    CHECK_EQ_L(world_get(&g->world, 0, 5, 101, 2), st);
    CHECK(world_get(&g->world, 0, 2, 101, 2) == BLOCK_AIR);
    CHECK(run(g, "/sphere glass 3 hollow") == CMD_OK);
    CHECK_EQ_L(world_get(&g->world, 0, 0, 103, 0), block_find("glass"));
    CHECK(world_get(&g->world, 0, 0, 101, 0) == BLOCK_AIR);   /* hollow inside */
    CHECK(run(g, "/sphere nosuchblock 3") == CMD_ERR);
    CHECK(run(g, "/sphere glass 99") == CMD_USAGE);
    CHECK(run(g, "/tunnel 6 1 2") == CMD_OK);
    game_destroy(g);
}

static void t_inventory_cmds(void)
{
    Game *g = &g_game;
    int bread = item_find("bread"), c;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 100, 0, 4);
    g->p.facing = DIR_E;
    g->inv.s[3].item = (cl_s16)bread; g->inv.s[3].count = 5;
    g->inv.s[7].item = (cl_s16)bread; g->inv.s[7].count = 6;
    CHECK(run(g, "/stack") == CMD_OK);
    CHECK_EQ_L(g->inv.s[3].count, 11);
    CHECK(g->inv.s[7].item < 0);
    c = chest_create(g, 0, 1, 100, 0);
    world_set(&g->world, 0, 1, 100, 0, (cl_u8)block_find("chest"), META_PLACED);
    CHECK(run(g, "/deposit bread") == CMD_OK);
    CHECK_EQ_L(inv_count(&g->inv, bread), 0);
    CHECK_EQ_L(g->chests[c].s[0].count, 11);
    CHECK(run(g, "/chestinfo") == CMD_OK);
    CHECK(run(g, "/withdraw all") == CMD_OK);
    CHECK_EQ_L(inv_count(&g->inv, bread), 11);
    g->p.hunger = 300;
    CHECK(run(g, "/eat") == CMD_OK);
    CHECK(g->p.hunger > 300);
    CHECK_EQ_L(inv_count(&g->inv, bread), 10);
    CHECK(run(g, "/swap 1 2") == CMD_OK);
    CHECK(run(g, "/sort") == CMD_OK);
    game_destroy(g);
}

static void t_waypoints_and_death(void)
{
    Game *g = &g_game;
    const ClidePlatform *p = plat_native();
    ClideTimeCfg tc;
    char err[128];
    ctime_default_cfg(&tc);
    CHECK(save_prepare(p, "tsave10"));
    CHECK(game_create(g, "tsave10", "saves/tsave10", 10UL, GM_SURVIVAL, 1, &tc));
    g->world.replace = p->fs.replace;
    CHECK(run(g, "/waypoint add camp") == CMD_OK);
    game_teleport(g, g->p.x + 40, g->p.y, g->p.z);
    CHECK(run(g, "/navigate camp") == CMD_OK);
    CHECK(run(g, "/navigate nowhere") == CMD_ERR);
    surv_damage(g, 10000, "cause.command");
    CHECK_EQ_L(g->deaths, 1);
    CHECK(run(g, "/deathpoint") == CMD_OK);
    game_respawn(g);
    CHECK(save_game(g, p, err, sizeof(err)));
    game_destroy(g);
    CHECK(save_load(g, p, "tsave10", &tc, err, sizeof(err)));
    CHECK(g->wp[0].active && str_ieq(g->wp[0].name, "camp"));
    CHECK_EQ_L(g->deaths, 1);
    CHECK(run(g, "/gowaypoint camp") == CMD_OK);
    CHECK(g->p.x == g->wp[0].x && g->p.z == g->wp[0].z);
    CHECK(run(g, "/waypoint remove camp") == CMD_OK && !g->wp[0].active);
    game_destroy(g);
}

static void t_ai_and_requests(void)
{
    Game *g = &g_game;
    int m;
    long x0;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 100, 0, 8);
    m = mob_spawn(g, creature_find("deer"), 0, 2, 100, 0);
    g->p.noise = 80;
    CHECK(run(g, "/ai off") == CMD_OK && g->ai_off);
    x0 = g->mobs[m].x;
    {
        int k;
        for (k = 0; k < 100; ++k) game_tick(g);
    }
    CHECK_EQ_L(g->mobs[m].x, x0);
    CHECK(run(g, "/ai on") == CMD_OK && !g->ai_off);
    CHECK(run(g, "/sky") == CMD_OK && g_con_request == CON_REQ_SKY);
    CHECK(run(g, "/panel 2") == CMD_OK && g_con_request == CON_REQ_PANEL && g_con_arg == 1);
    g_con_request = CON_REQ_NONE;
    CHECK(run(g, "/unstuck") == CMD_OK);
    world_set(&g->world, 0, g->p.x, g->p.y, g->p.z, (cl_u8)g_cb.stone, 0);
    CHECK(run(g, "/unstuck") == CMD_OK);
    CHECK(!blk_solid(world_get(&g->world, 0, g->p.x, g->p.y, g->p.z)));
    game_destroy(g);
}

static void t_pcm_audio(void)
{
    const ClidePlatform *h = plat_headless();
    long n0;
    aud_init(h, 1);
    CHECK(aud_pcm());
    n0 = headless_pcm_samples();
    aud_play(SFX_RAIN);
    CHECK(aud_played() == 1);
    CHECK(headless_pcm_samples() - n0 > 22050L);       /* 1.8 s of rain */
    n0 = headless_pcm_samples();
    aud_update(500);
    aud_play(SFX_STEP);                                /* mixed into the rest of the rain */
    CHECK(aud_played() == 2);
    CHECK(headless_pcm_samples() - n0 > 22050L);       /* about 1.3 s of rain remain */
    CHECK(headless_pcm_samples() - n0 < 39690L);
    aud_play(SFX_THUNDER);
    CHECK(aud_played() == 3);
    aud_update(5000);
    aud_play(SFX_PICKUP);                              /* tone-only effect rendered as PCM */
    CHECK(aud_played() == 4);
    aud_set_enabled(0);
    aud_play(SFX_MENU);
    CHECK(aud_played() == 4);
    CHECK(aud_find("crack") == SFX_CRACK && aud_find("nope") < 0);
    aud_shutdown();
}

/* DOS keeps the language tables in 36000-byte pools with 960 entries */
static void t_dos_locale_limits(void)
{
    static const char *const langs[] = { "en", "es", "fr", "de", "pt", "it", NULL };
    int i;
    for (i = 0; langs[i]; ++i) {
        char path[200], line[600];
        long n = 0, bytes = 0;
        FILE *f;
        sprintf(path, "%s/locale/%s.lng", tf_data, langs[i]);
        f = fopen(path, "r");
        CHECK(f != NULL);
        if (!f) continue;
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq && line[0] != '#' && line[0] != '@') {
                ++n;
                bytes += (long)strlen(line) + 1;
            }
        }
        fclose(f);
        if (n >= 960 || bytes >= 36000L) printf("    %s: %ld keys, %ld bytes\n", langs[i], n, bytes);
        CHECK(n < 960);
        CHECK(bytes < 36000L);
    }
}

void suite_final(void)
{
    tf_run("240 commands with descriptions", t_count);
    tf_run("every command runs (smoke)", t_smoke);
    tf_run("building shapes and undo", t_shapes);
    tf_run("inventory and chest commands", t_inventory_cmds);
    tf_run("waypoints and death point are saved", t_waypoints_and_death);
    tf_run("AI freeze, UI requests, unstuck", t_ai_and_requests);
    tf_run("PCM synthesis and priorities", t_pcm_audio);
    tf_run("language tables fit DOS limits", t_dos_locale_limits);
}
