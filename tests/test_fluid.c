/* Cliderness - tests/test_fluid.c : Phase 6 - ClideFluid, rivers, lakes, structures, buckets */
#include <stdio.h>
#include <string.h>
#include "tf.h"
#include "gameplay/game.h"
#include "content/content.h"
#include "world/structures/structure.h"
#include "world/terrain/terrain.h"
#include "core/log.h"
#include "core/mathx.h"

/* a sealed stone box high in the air: inner area x/z 0..w-1, y 101..110 */
static void box(Game *g, int w)
{
    long x, y, z;
    for (y = 100; y <= 111; ++y) {
        for (z = -1; z <= w; ++z) {
            for (x = -1; x <= w; ++x) {
                int wall = x < 0 || z < 0 || x >= w || z >= w || y == 100;
                world_set_raw(&g->world, 0, x, y, z, wall ? (cl_u8)g_cb.stone : BLOCK_AIR, 0);
            }
        }
    }
}

static long run(Game *g, int steps)
{
    long n = 0;
    while (steps-- > 0) {
        n += fluid_step(&g->world);
    }
    return n;
}

static long vol(Game *g, int w)
{
    return fluid_volume(&g->world, 0, g_cb.water, 0, 101, 0, w - 1, 111, w - 1);
}

static void t_conservation(void)
{
    Game *g = &g_game;
    long v0;
    int k;
    CHECK(tf_make_game(g, GM_CREATIVE, NULL));
    g->world.fluid.enabled = 1;
    box(g, 9);
    /* a column of water poured in one corner */
    for (k = 0; k < 6; ++k) {
        fluid_put(&g->world, 0, 0, 104 + k, 0, g_cb.water, 15);
    }
    v0 = vol(g, 9);
    CHECK_EQ_L(v0, 90);
    run(g, 400);
    CHECK_EQ_L(vol(g, 9), v0);                        /* volume is conserved */
    CHECK(fluid_level(&g->world, 0, 0, 105, 0) == 0); /* the column collapsed */
    CHECK(fluid_level(&g->world, 0, 5, 101, 0) > 0);  /* and spread across the floor */
    CHECK(fluid_level(&g->world, 0, 0, 101, 5) > 0);
    {
        /* settled: neighbouring cells differ by at most one level
         * (1/15 block), i.e. the surface is flat at block resolution */
        int steep = 0, hi = 0, x, z;
        for (z = 0; z < 9; ++z) for (x = 0; x < 9; ++x) {
            int l = fluid_level(&g->world, 0, x, 101, z);
            if (l > hi) hi = l;
            if (x < 8 && mx_abs(l - fluid_level(&g->world, 0, x + 1, 101, z)) > 1) ++steep;
            if (z < 8 && mx_abs(l - fluid_level(&g->world, 0, x, 101, z + 1)) > 1) ++steep;
        }
        printf("    settled: highest level %d, steep steps %d, %ld cells processed\n", hi, steep, g->world.fluid.processed);
        CHECK(steep == 0);
        CHECK(hi <= 6);
        CHECK(g->world.fluid.count == 0);            /* the simulation went idle */
    }
    game_destroy(g);
}

static void t_dam(void)
{
    Game *g = &g_game;
    long x, y;
    CHECK(tf_make_game(g, GM_CREATIVE, NULL));
    g->world.fluid.enabled = 1;
    box(g, 12);
    /* dam: wall at x = 4 separates a reservoir (x 0..3) from a dry valley */
    for (y = 101; y <= 111; ++y) {
        long z;
        for (z = 0; z < 12; ++z) world_set_raw(&g->world, 0, 4, y, z, (cl_u8)g_cb.stone, 0);
    }
    for (y = 101; y <= 104; ++y) {
        long z;
        for (z = 0; z < 12; ++z) for (x = 0; x < 4; ++x) fluid_put(&g->world, 0, x, y, z, g_cb.water, 15);
    }
    run(g, 200);
    CHECK(fluid_level(&g->world, 0, 8, 101, 5) == 0);   /* dam holds */
    /* break the dam: the valley floods, volume stays the same */
    {
        long v0 = vol(g, 12);
        for (y = 101; y <= 104; ++y) world_set(&g->world, 0, 4, y, 5, BLOCK_AIR, 0);
        run(g, 1500);
        CHECK(fluid_level(&g->world, 0, 10, 101, 9) > 0);
        CHECK_EQ_L(vol(g, 12), v0);
    }
    game_destroy(g);
}

static void t_waterfall_and_drain(void)
{
    Game *g = &g_game;
    long y;
    CHECK(tf_make_game(g, GM_CREATIVE, NULL));
    g->world.fluid.enabled = 1;
    box(g, 5);
    /* a shelf at y 106 holding water over a pit */
    world_set_raw(&g->world, 0, 2, 105, 2, (cl_u8)g_cb.stone, 0);
    fluid_put(&g->world, 0, 2, 106, 2, g_cb.water, 15);
    run(g, 300);
    CHECK(fluid_level(&g->world, 0, 2, 106, 2) <= 1);
    for (y = 102; y <= 104; ++y) {
        CHECK(fluid_level(&g->world, 0, 0, y, 0) == 0); /* water fell down to the floor */
    }
    CHECK_EQ_L(vol(g, 5), 15);
    /* drain: a hole in the floor under water lets the water out */
    fluid_put(&g->world, 0, 0, 101, 0, g_cb.water, 15);
    run(g, 200);
    world_set(&g->world, 0, 0, 100, 0, BLOCK_AIR, 0);
    world_set_raw(&g->world, 0, 0, 99, 0, (cl_u8)g_cb.stone, 0);
    run(g, 800);
    CHECK(fluid_level(&g->world, 0, 0, 100, 0) > 0);
    game_destroy(g);
}

static void t_lava(void)
{
    Game *g = &g_game;
    int obs = block_find("obsidian");
    CHECK(tf_make_game(g, GM_CREATIVE, NULL));
    g->world.fluid.enabled = 1;
    box(g, 6);
    fluid_put(&g->world, 0, 0, 101, 0, g_cb.lava, 15);
    fluid_put(&g->world, 0, 1, 101, 0, g_cb.water, 15);
    run(g, 60);
    CHECK(world_get(&g->world, 0, 0, 101, 0) == obs || world_get(&g->world, 0, 0, 101, 0) == g_cb.cobble);
    game_destroy(g);
}

static void t_sea_source(void)
{
    Game *g = &g_game;
    long x, z, sx = 0, sz = 0;
    int found = 0;
    CHECK(tf_make_game(g, GM_CREATIVE, NULL));
    g->world.fluid.enabled = 1;
    /* find open sea */
    for (x = -2000; x < 2000 && !found; x += 37) {
        for (z = -2000; z < 2000 && !found; z += 41) {
            if (world_gen_height(&g->world, 0, x, z) < SEA_LEVEL - 8) {
                sx = x;
                sz = z;
                found = 1;
            }
        }
    }
    CHECK(found);
    CHECK(fluid_is_source(&g->world, 0, sx, SEA_LEVEL - 1, sz));
    /* scoop the top cell: it refills */
    world_set(&g->world, 0, sx, SEA_LEVEL - 1, sz, BLOCK_AIR, 0);
    run(g, 20);
    CHECK(fluid_level(&g->world, 0, sx, SEA_LEVEL - 1, sz) == 15);
    game_destroy(g);
}

static void t_bucket(void)
{
    Game *g = &g_game;
    int bucket = item_find("bucket");
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    g->world.fluid.enabled = 1;
    box(g, 6);
    game_teleport(g, 2, 101, 2);
    fluid_put(&g->world, 0, 3, 101, 2, g_cb.water, 9);
    g->p.facing = DIR_E;
    inv_add(&g->inv, bucket, 1, 0);
    g->inv.selected = 0;
    game_use(g);
    CHECK(g->inv.s[0].item == item_find("water_bucket") && g->inv.s[0].dur == 9);
    CHECK(fluid_level(&g->world, 0, 3, 101, 2) == 0);
    g->p.facing = DIR_W;
    game_place(g);
    CHECK(g->inv.s[0].item == bucket);
    CHECK(fluid_level(&g->world, 0, 1, 101, 2) == 9);
    game_destroy(g);
}

static void t_rain_puddles(void)
{
    Game *g = &g_game;
    int k;
    long added = 0;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    g->world.fluid.enabled = 1;
    tf_flat_area(g, 0, 100, 0, 30);
    /* a grid of one-cell dents */
    for (k = -24; k <= 24; k += 2) {
        long j;
        for (j = -24; j <= 24; j += 2) {
            world_set_raw(&g->world, 0, k, 99, j, BLOCK_AIR, 0);
            world_set_raw(&g->world, 0, k, 98, j, (cl_u8)g_cb.stone, 0);
        }
    }
    weather_force(&g->weather, weather_find("heavy_rain"), 600);
    for (k = 0; k < 300; ++k) {
        game_rain_minute(g);
    }
    added = fluid_volume(&g->world, 0, g_cb.water, -25, 95, -25, 25, 101, 25);
    printf("    rain added %ld water units\n", added);
    CHECK(added > 50);
    game_destroy(g);
}

static void t_rivers_lakes(void)
{
    Game *g = &g_game;
    long x, z;
    int river = 0, lake = 0;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    for (z = -3000; z < 3000; z += 25) {
        for (x = -3000; x < 3000; x += 25) {
            int h = world_gen_height(&g->world, 0, x, z);
            if (h < SEA_LEVEL && h >= SEA_LEVEL - 6 && world_gen_height(&g->world, 0, x + 60, z) > SEA_LEVEL + 3) {
                ++river;
            }
        }
    }
    for (x = 0; x < 400 && !lake; ++x) {
        int y;
        for (y = 12; y <= 26; ++y) {
            if (world_get(&g->world, 0, x * 7, y, x * 3) == g_cb.water) {
                lake = 1;
                break;
            }
        }
    }
    printf("    river samples %d, underground lake %s\n", river, lake ? "found" : "not found");
    CHECK(river > 5);
    CHECK(lake);
    game_destroy(g);
}

static void t_structures(void)
{
    Game *g = &g_game;
    long cx, cy, cz;
    int mine = struct_find("abandoned_mine"), ruins = struct_find("stone_ruins");
    int crate = block_find("mine_crate");
    CHECK(mine >= 0 && ruins >= 0 && g_loot_count >= 2);
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    CHECK(structure_locate(g->seed, mine, 0, 0, &cx, &cy, &cz));
    printf("    nearest mine at %ld %ld %ld\n", cx, cy, cz);
    if (cy < world_gen_height(&g->world, 0, cx, cz) - 4) {
        CHECK(world_get(&g->world, 0, cx, cy, cz) == BLOCK_AIR || g_blocks[world_get(&g->world, 0, cx, cy, cz)].flags & BF_LIGHT);
        CHECK(world_get(&g->world, 0, cx + 6, cy + 2, cz) == block_find("oak_planks"));
    }
    /* the crate at the end of the east corridor fills with loot when opened */
    {
        long ex = cx + g_structs[mine].length;
        if (world_get(&g->world, 0, ex, cy, cz) == crate) {
            int c;
            game_teleport(g, ex - 1, cy, cz);
            g->p.facing = DIR_E;
            game_interact(g);
            c = chest_find(g, 0, ex, cy, cz);
            CHECK(c >= 0);
            if (c >= 0) {
                int k, n = 0;
                for (k = 0; k < CHEST_SLOTS; ++k) n += g->chests[c].s[k].item >= 0;
                CHECK(n > 0);
            }
        }
    }
    CHECK(structure_locate(g->seed, ruins, 0, 0, &cx, &cy, &cz));
    /* structures are part of the deterministic generator */
    {
        ColumnData col;
        int idx;
        memset(&col, 0, sizeof(col));
        col.cx = cx >= 0 ? cx / 32 : -((-cx + 31) / 32);
        col.cz = cz >= 0 ? cz / 32 : -((-cz + 31) / 32);
        terrain_gen_column(g->seed, 0, &col);
        idx = (int)((cz - col.cz * 32) * 32 + (cx - col.cx * 32));
        CHECK(terrain_block(g->seed, 0, &col, idx, cx, col.height[idx] + 1, cz) ==
              world_get(&g->world, 0, cx, col.height[idx] + 1, cz));
    }
    game_destroy(g);
}

void suite_fluid(void)
{
    tf_run("fluid volume is conserved", t_conservation);
    tf_run("dams hold and break", t_dam);
    tf_run("waterfalls and drainage", t_waterfall_and_drain);
    tf_run("water meets lava", t_lava);
    tf_run("the sea refills", t_sea_source);
    tf_run("buckets carry water", t_bucket);
    tf_run("rain fills depressions", t_rain_puddles);
    tf_run("rivers and underground lakes", t_rivers_lakes);
    tf_run("mines, ruins and loot", t_structures);
    log_clear();
}
