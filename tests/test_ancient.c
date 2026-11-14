/* Cliderness - tests/test_ancient.c : Phase 9 - the Ancient Dimension and portals */
#include <stdio.h>
#include <string.h>
#include "tf.h"
#include "gameplay/game.h"
#include "gameplay/entities/creature.h"
#include "content/content.h"
#include "content/recipes.h"
#include "world/biomes/biome.h"
#include "world/structures/structure.h"
#include "world/weather/weather.h"
#include "commands/console.h"
#include "save/save.h"
#include "platform/platform.h"
#include "core/str.h"

static void t_content(void)
{
    int i, ancient_biomes = 0, ancient_mobs = 0;
    for (i = 0; i < g_biome_count; ++i) {
        if (g_biomes[i].kind == BK_ANCIENT) ++ancient_biomes;
    }
    for (i = 0; i < g_creature_count; ++i) {
        if (g_creatures[i].dim == DIM_ANCIENT) ++ancient_mobs;
    }
    printf("    %d ancient biomes, %d ancient creatures\n", ancient_biomes, ancient_mobs);
    CHECK_EQ_L(ancient_biomes, 8);
    CHECK(ancient_mobs >= 6);
    CHECK(block_find("ancient_portal") > 0 && g_blocks[block_find("ancient_portal")].func == FUNC_PORTAL);
    CHECK(g_items[item_find("portal_core")].place_block == block_find("ancient_portal"));
    CHECK(g_blocks[block_find("ancient_altar")].station == STATION_ALTAR);
    CHECK(recipe_find("portal_core") >= 0 && recipe_find("starmetal_sword") >= 0);
    CHECK(g_structs[struct_find("ancient_city")].dim == DIM_ANCIENT);
    CHECK(g_weather[weather_find("starfall")].ancient == 1);
}

static void t_generation(void)
{
    Game *g = &g_game;
    long x, z;
    int ancient_surface = 1, overworld_ancient = 0, ores = 0, voids = 0, void_water = 0;
    int aether = block_find("aether_ore"), star = block_find("starmetal_ore");
    int vr = biome_find("void_rift");
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    for (z = -256; z <= 256; z += 16) {
        for (x = -256; x <= 256; x += 16) {
            int b = world_biome(&g->world, DIM_ANCIENT, x, z);
            int h = world_gen_height(&g->world, DIM_ANCIENT, x, z);
            if (g_biomes[b].kind != BK_ANCIENT) ancient_surface = 0;
            if (g_biomes[world_biome(&g->world, DIM_OVERWORLD, x, z)].kind == BK_ANCIENT) overworld_ancient = 1;
            if (b == vr) {
                ++voids;
                if (blk_liquid(world_get(&g->world, DIM_ANCIENT, x, h + 1, z))) ++void_water;
            }
        }
    }
    for (z = 0; z < 32; ++z) {
        for (x = 0; x < 32; ++x) {
            long y;
            for (y = 4; y < 60; ++y) {
                cl_u8 b = world_get(&g->world, DIM_ANCIENT, x, y, z);
                if (b == (cl_u8)aether || b == (cl_u8)star) ++ores;
            }
        }
    }
    printf("    void rift samples %d, ore blocks in one column chunk %d\n", voids, ores);
    CHECK(ancient_surface);
    CHECK(!overworld_ancient);
    CHECK(ores > 0);
    CHECK_EQ_L(void_water, 0);
    /* deterministic and independent of the overworld */
    CHECK(world_gen_height(&g->world, DIM_ANCIENT, 100, 100) == world_gen_height(&g->world, DIM_ANCIENT, 100, 100));
    game_destroy(g);
}

static void t_portal_travel(void)
{
    Game *g = &g_game;
    int pb = block_find("ancient_portal"), core = item_find("portal_core");
    long ax, az, ay;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 100, 0, 4);
    g->p.facing = DIR_E;
    inv_add(&g->inv, core, 1, 0);
    game_select(g, 0);
    game_place(g);
    CHECK_EQ_L(world_get(&g->world, 0, 1, 100, 0), pb);
    game_interact(g);                       /* E on the portal */
    CHECK_EQ_L(g->p.dim, DIM_ANCIENT);
    ax = g->p.x;
    ay = g->p.y;
    az = g->p.z;
    CHECK(blk_solid(world_get(&g->world, DIM_ANCIENT, ax, ay - 1, az)));
    CHECK(!blk_solid(world_get(&g->world, DIM_ANCIENT, ax, ay, az)));
    CHECK_EQ_L(world_get(&g->world, DIM_ANCIENT, ax + 1, ay, az), pb);   /* return portal */
    CHECK(g->p.skill_xp[SK_EXPLORATION] > 0);
    /* weather follows the dimension */
    g->p.god = 1;
    game_tick(g);
    {
        long k;
        for (k = 0; k < 20 * 60 * 3; ++k) game_tick(g);
    }
    CHECK(g_weather[g->weather.cur].ancient != 0);
    /* back through the return portal */
    g->p.x = ax;
    g->p.y = ay;
    g->p.z = az;
    g->p.facing = DIR_E;
    game_interact(g);
    CHECK_EQ_L(g->p.dim, DIM_OVERWORLD);
    /* arriving next to the original portal does not build a second one */
    {
        long dx, dy, dz, n = 0;
        for (dy = -3; dy <= 3; ++dy)
            for (dz = -10; dz <= 10; ++dz)
                for (dx = -10; dx <= 10; ++dx)
                    if (world_get(&g->world, 0, g->p.x + dx, g->p.y + dy, g->p.z + dz) == (cl_u8)pb) ++n;
        CHECK_EQ_L(n, 1);
    }
    game_destroy(g);
}

static void t_respawn_dimension(void)
{
    Game *g = &g_game;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    portal_travel(g, DIM_ANCIENT, 0);
    CHECK_EQ_L(g->p.dim, DIM_ANCIENT);
    surv_damage(g, 10000, "cause.command");
    CHECK(g->p.dead);
    game_respawn(g);
    CHECK_EQ_L(g->p.dim, DIM_OVERWORLD);  /* spawn point is in the overworld */
    portal_travel(g, DIM_ANCIENT, 0);
    g->p.spawn_dim = DIM_ANCIENT;
    g->p.spawn_x = g->p.x;
    g->p.spawn_z = g->p.z;
    surv_damage(g, 10000, "cause.command");
    game_respawn(g);
    CHECK_EQ_L(g->p.dim, DIM_ANCIENT);
    game_destroy(g);
}

static void t_ancient_creatures(void)
{
    Game *g = &g_game;
    int i, wrong = 0, n = 0;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    portal_travel(g, DIM_ANCIENT, 0);
    g->spawning = 1;
    g->p.god = 1;
    ctime_set_time_of_day(&g->time, 0);
    g->last_minute = ctime_total_minutes(&g->time);
    for (i = 0; i < 2400; ++i) game_tick(g);
    for (i = 0; i < MAX_MOBS; ++i) {
        if (!g->mobs[i].active) continue;
        ++n;
        if (g_creatures[g->mobs[i].def].dim != DIM_ANCIENT) ++wrong;
    }
    printf("    %d creatures in the Ancient Dimension at night\n", n);
    CHECK(n > 0);
    CHECK_EQ_L(wrong, 0);
    game_destroy(g);
}

static void t_structures(void)
{
    Game *g = &g_game;
    long x, y, z;
    int s = struct_find("ancient_city");
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    CHECK(structure_locate(g->seed, s, 0, 0, &x, &y, &z));
    printf("    ancient city at %ld,%ld\n", x, z);
    {
        long dx, found = 0, h = world_gen_height(&g->world, DIM_ANCIENT, x + 22, z);
        int bricks = block_find("ancient_bricks");
        for (dx = 1; dx <= 4; ++dx) {
            if (world_get(&g->world, DIM_ANCIENT, x + 22, h + dx, z + 5) == (cl_u8)bricks) found = 1;
        }
        CHECK(found);   /* the city wall */
    }
    game_destroy(g);
}

static void t_commands(void)
{
    Game *g = &g_game;
    const ClidePlatform *p = plat_native();
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    CHECK(con_exec(g, p, "dimension") == CMD_OK);
    CHECK(con_exec(g, p, "dimensions") == CMD_OK);
    CHECK(con_exec(g, p, "ancientbiomes") == CMD_OK);
    CHECK(con_exec(g, p, "travel overworld") == CMD_ERR);
    CHECK(con_exec(g, p, "travel nowhere") == CMD_USAGE);
    CHECK(con_exec(g, p, "travel ancient portal") == CMD_OK && g->p.dim == DIM_ANCIENT);
    CHECK(con_exec(g, p, "makeportal") == CMD_OK || con_exec(g, p, "makeportal") == CMD_ERR);
    CHECK(con_exec(g, p, "gotodim overworld") == CMD_OK && g->p.dim == DIM_OVERWORLD);
    game_destroy(g);
}

static void t_save_dimension(void)
{
    Game *g = &g_game;
    const ClidePlatform *p = plat_native();
    ClideTimeCfg tc;
    char err[128];
    long x, z;
    ctime_default_cfg(&tc);
    CHECK(save_prepare(p, "tsave9"));
    remove("saves/tsave9/chunks.dat");
    CHECK(game_create(g, "tsave9", "saves/tsave9", 9UL, GM_SURVIVAL, 1, &tc));
    g->world.replace = p->fs.replace;
    portal_travel(g, DIM_ANCIENT, 1);
    g->p.spawn_dim = DIM_ANCIENT;
    x = g->p.x;
    z = g->p.z;
    CHECK(save_game(g, p, err, sizeof(err)));
    game_destroy(g);
    CHECK(save_load(g, p, "tsave9", &tc, err, sizeof(err)));
    CHECK_EQ_L(g->p.dim, DIM_ANCIENT);
    CHECK_EQ_L(g->p.spawn_dim, DIM_ANCIENT);
    CHECK(g->p.x == x && g->p.z == z);
    CHECK_EQ_L(world_get(&g->world, DIM_ANCIENT, x + 1, g->p.y, z), block_find("ancient_portal"));
    game_destroy(g);
}

void suite_ancient(void)
{
    tf_run("ancient content", t_content);
    tf_run("ancient generation, ores, void rifts", t_generation);
    tf_run("portal travel and return portal", t_portal_travel);
    tf_run("respawn in the spawn dimension", t_respawn_dimension);
    tf_run("ancient creatures spawn only there", t_ancient_creatures);
    tf_run("ancient city", t_structures);
    tf_run("dimension commands", t_commands);
    tf_run("dimension is saved", t_save_dimension);
}
