/* Cliderness - tests/test_more.c : 1.1 - attribute growth, fishing, stealth,
 * A* pathfinding, the Ancient sky, streamed PCM */
#include <stdio.h>
#include <string.h>
#include "tf.h"
#include "gameplay/game.h"
#include "gameplay/entities/creature.h"
#include "content/content.h"
#include "world/sky/sky.h"
#include "audio/audio.h"
#include "save/save.h"
#include "platform/platform.h"
#include "core/str.h"

static void ticks(Game *g, long n)
{
    while (n-- > 0) game_tick(g);
}

static void t_attribute_growth(void)
{
    Game *g = &g_game;
    const ClidePlatform *p = plat_native();
    ClideTimeCfg tc;
    char err[128];
    int k;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    CHECK_EQ_L(g->p.stats[ST_STR], 10);
    /* 600 training points take STR from 10 to 11 */
    for (k = 0; k < 600; ++k) stat_train(g, ST_STR, 1);
    CHECK_EQ_L(g->p.stats[ST_STR], 11);
    CHECK_EQ_L(g->p.stat_xp[ST_STR], 0);
    /* skills train their attribute */
    g->p.xp_minute = 0;
    skill_gain(g, SK_RANGED, 50);
    CHECK(g->p.stat_xp[ST_DEX] > 0);
    /* CON from surviving damage raises max HP */
    {
        int hp0 = g->p.hp_max;
        for (k = 0; k < 700; ++k) stat_train(g, ST_CON, 1);
        CHECK_EQ_L(g->p.stats[ST_CON], 11);
        CHECK_EQ_L(g->p.hp_max, hp0 + 2);
    }
    surv_damage(g, 10, "cause.command");
    CHECK(g->p.stat_xp[ST_CON] > 0);
    /* play stops at 30 */
    g->p.stats[ST_LCK] = STAT_PLAY_MAX;
    stat_train(g, ST_LCK, 100000);
    CHECK_EQ_L(g->p.stats[ST_LCK], STAT_PLAY_MAX);
    game_destroy(g);
    /* training is saved */
    ctime_default_cfg(&tc);
    CHECK(save_prepare(p, "tsave11"));
    CHECK(game_create(g, "tsave11", "saves/tsave11", 11UL, GM_SURVIVAL, 1, &tc));
    g->world.replace = p->fs.replace;
    stat_train(g, ST_INT, 123);
    CHECK(save_game(g, p, err, sizeof(err)));
    game_destroy(g);
    CHECK(save_load(g, p, "tsave11", &tc, err, sizeof(err)));
    CHECK_EQ_L(g->p.stat_xp[ST_INT], 123);
    game_destroy(g);
}

static void t_fishing(void)
{
    Game *g = &g_game;
    int rod = item_find("fishing_rod"), fish = item_find("raw_fish"), casts = 0;
    long x, z;
    CHECK(rod >= 0);
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 100, 0, 4);
    g->p.god = 1;
    g->p.facing = DIR_E;
    inv_add(&g->inv, rod, 1, g_items[rod].durability);
    game_select(g, 0);
    game_use(g);                                   /* no water */
    CHECK_EQ_L(g->p.fishing, 0);
    for (z = -1; z <= 1; ++z)
        for (x = 1; x <= 3; ++x)
            world_set(&g->world, 0, x, 99, z, (cl_u8)g_cb.water, 0);
    while (inv_count(&g->inv, fish) == 0 && casts < 60) {
        game_use(g);
        CHECK(g->p.fishing > 0);
        ticks(g, 200);
        ++casts;
    }
    printf("    caught a fish after %d casts\n", casts);
    CHECK(inv_count(&g->inv, fish) >= 1);
    CHECK(g->p.skill_xp[SK_FISHING] > 0);
    CHECK(g->inv.s[0].dur < g_items[rod].durability);
    /* moving reels the line in */
    game_use(g);
    game_request_move(g, DIR_W, 0);
    ticks(g, 10);
    CHECK_EQ_L(g->p.fishing, 0);
    game_destroy(g);
}

static void t_stealth(void)
{
    Game *g = &g_game;
    int k;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 100, 0, 10);
    g->p.god = 1;
    g->ai_off = 1;
    mob_spawn(g, creature_find("deer"), 0, 6, 100, 0);
    game_toggle_crouch(g);
    for (k = 0; k < 8; ++k) {
        game_request_move(g, (k & 1) ? DIR_N : DIR_S, 0);
        ticks(g, 12);
    }
    CHECK(g->p.skill_xp[SK_STEALTH] > 0);
    CHECK(g->p.noise <= 10);
    game_destroy(g);
}

static void t_pathfinding(void)
{
    Game *g = &g_game;
    int m;
    long z, n;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 100, 0, 12);
    /* a wall between the creature and the target with a gap at z = 6 */
    for (z = -12; z <= 12; ++z) {
        if (z == 6) continue;
        world_set(&g->world, 0, 0, 100, z, (cl_u8)g_cb.stone, 0);
        world_set(&g->world, 0, 0, 101, z, (cl_u8)g_cb.stone, 0);
    }
    game_teleport(g, 5, 100, 0);
    m = mob_spawn(g, creature_find("zombie"), 0, -5, 100, 0);
    n = mob_find_path(g, m, 4, 100, 0);
    printf("    path around the wall: %ld steps\n", n);
    CHECK_EQ_L(n, 20);                              /* down to the gap and back: optimal */
    CHECK(g->mobs[m].npath > 0);
    /* with the AI running, the zombie reaches the player at night */
    ctime_set_time_of_day(&g->time, 0);
    g->last_minute = ctime_total_minutes(&g->time);
    g->p.god = 0;                                   /* creatures ignore god mode */
    g->p.hp_max = g->p.hp = 30000;
    g->p.noise = 80;
    ticks(g, 20 * 40);
    printf("    zombie at %ld,%ld (player 5,0)\n", g->mobs[m].x, g->mobs[m].z);
    CHECK(g->mobs[m].x > 0);                        /* it went through the gap */
    /* no route: walled in */
    for (z = -12; z <= 12; ++z) {
        world_set(&g->world, 0, 0, 100, z, (cl_u8)g_cb.stone, 0);
        world_set(&g->world, 0, 0, 101, z, (cl_u8)g_cb.stone, 0);
    }
    g->mobs[m].x = -5;
    g->mobs[m].z = 0;
    CHECK_EQ_L(mob_find_path(g, m, 4, 100, 0), 0);
    game_destroy(g);
}

static void t_ancient_sky(void)
{
    Game *g = &g_game;
    static Framebuffer fb;
    SkyInfo info;
    int seen_moon = 0, h;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    fb_clear(&fb, 0);
    weather_force(&g->weather, weather_find("clear"), 9999);
    ctime_set_time_of_day(&g->time, 23L * 3600L);
    sky_render(&fb, g, 0, 0, 80, 20, 0, &info);
    CHECK(!info.second_moon && !info.ringed_world && !info.aether_veil);
    portal_travel(g, DIM_ANCIENT, 0);
    weather_force(&g->weather, weather_find("starfall"), 9999);
    for (h = 0; h < 72; h += 3) {
        ctime_set_time_of_day(&g->time, (long)(h % 24) * 3600L);
        g->time.day = h / 24;
        sky_render(&fb, g, 0, 0, 80, 20, 0, &info);
        if (info.second_moon) seen_moon = 1;
        if (h % 24 == 0) {
            CHECK(info.aether_veil);
            CHECK(!info.birds && !info.aurora);
        }
        CHECK(info.ringed_world);
    }
    CHECK(seen_moon);
    {
        int a, b;
        ctime_set_time_of_day(&g->time, 0);
        g->time.day = 0;
        a = sky_veyra_phase(g);
        g->time.day = 3;
        b = sky_veyra_phase(g);
        CHECK_EQ_L(a, b);                           /* a 3-day cycle */
    }
    game_destroy(g);
}

static void t_streamed_pcm(void)
{
    const ClidePlatform *h = plat_headless();
    long n0;
    headless_pcm_mode(PCM_STREAM);
    aud_init(h, 1);
    CHECK(aud_pcm());
    n0 = headless_pcm_samples();
    aud_play(SFX_RAIN);
    CHECK(headless_pcm_samples() - n0 < 2000L);       /* only a short lead-in */
    aud_update(500);
    CHECK(headless_pcm_samples() - n0 > 10000L);      /* blocks follow in real time */
    n0 = headless_pcm_samples();
    aud_play(SFX_STEP);                               /* mixed, no second stream */
    aud_update(100);
    CHECK(headless_pcm_samples() - n0 < 3000L);
    aud_update(5000);
    CHECK_EQ_L(aud_played(), 2);
    aud_shutdown();
    headless_pcm_mode(PCM_FLUSH);
}

void suite_more(void)
{
    tf_run("attributes grow through play", t_attribute_growth);
    tf_run("fishing", t_fishing);
    tf_run("stealth skill", t_stealth);
    tf_run("A* pathfinding", t_pathfinding);
    tf_run("the Ancient sky", t_ancient_sky);
    tf_run("streamed PCM mixing", t_streamed_pcm);
}
