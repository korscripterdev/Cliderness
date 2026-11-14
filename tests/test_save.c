/* Cliderness - tests/test_save.c : world save / load integrity */
#include <stdio.h>
#include <string.h>
#include "tf.h"
#include "save/save.h"
#include "content/content.h"
#include "commands/console.h"
#include "core/log.h"
#include "core/str.h"

static void t_names(void)
{
    CHECK(save_valid_name("world1"));
    CHECK(save_valid_name("a_b-C9"));
    CHECK(!save_valid_name(""));
    CHECK(!save_valid_name("toolongname"));   /* DOS 8.3 */
    CHECK(!save_valid_name("../x"));
    CHECK(!save_valid_name("a b"));
}

static void t_roundtrip(void)
{
    Game *g = &g_game;
    const ClidePlatform *p = plat_native();
    char err[128];
    ClideTimeCfg tc;
    int glass = block_find("glass");
    long px, py, pz, day, ms;
    int dirt = item_find("dirt");
    ctime_default_cfg(&tc);
    CHECK(save_prepare(p, "tsave1"));
    remove("saves/tsave1/chunks.dat");
    CHECK(game_create(g, "tsave1", "saves/tsave1", 4242UL, GM_SURVIVAL, 1, &tc));
    g->world.replace = p->fs.replace;
    tf_flat_area(g, 3, 75, -4, 3);
    world_set(&g->world, 0, 3, 75, -3, (cl_u8)glass, META_PLACED);
    inv_add(&g->inv, dirt, 17, 0);
    inv_add(&g->inv, item_find("iron_axe"), 1, 0);
    g->inv.s[1].dur = 77;
    g->inv.selected = 1;
    g->p.hunger = 432;
    g->p.stats[ST_LCK] = 33;
    g->p.skill_xp[SK_SWIMMING] = 1234;
    g->p.god = 1;
    ent_spawn(g, 0, 4, 75, -4, item_find("stone"), 5, 0);
    con_exec(g, p, "/time set 15:30");
    con_exec(g, p, "/weather set snow");
    px = g->p.x; py = g->p.y; pz = g->p.z;
    day = g->time.day; ms = g->time.ms;
    CHECK(save_game(g, p, err, sizeof(err)));
    CHECK(save_verify_file("saves/tsave1/world.ini"));
    game_destroy(g);

    CHECK(save_load(g, p, "tsave1", &tc, err, sizeof(err)));
    if (err[0]) printf("    load error: %s\n", err);
    CHECK(g->seed == 4242UL);
    CHECK(g->p.x == px && g->p.y == py && g->p.z == pz);
    CHECK(g->time.day == day && g->time.ms == ms);
    CHECK(str_ieq(g_weather[g->weather.cur].id, "snow"));
    CHECK(world_get(&g->world, 0, 3, 75, -3) == glass);
    CHECK(world_get(&g->world, 0, 3, 74, -4) == g_cb.stone);
    CHECK_EQ_L(inv_count(&g->inv, dirt), 17);
    CHECK(g->inv.s[1].dur == 77 && g->inv.selected == 1);
    CHECK(g->p.hunger == 432 && g->p.stats[ST_LCK] == 33 && g->p.skill_xp[SK_SWIMMING] == 1234);
    CHECK(g->p.god == 1);
    CHECK(g->cheats == 1 && g->cheated == 1);
    CHECK_EQ_L(ent_count(g), 1);
    /* unmodified terrain is regenerated from the seed, not stored: only the
     * 8 levels touched by the flat area (1 chunk each) are in the store */
    printf("    stored chunks: %d\n", g->world.store.count);
    CHECK(g->world.store.count >= 1 && g->world.store.count <= 10);
    game_destroy(g);
}

static void t_corruption(void)
{
    Game *g = &g_game;
    const ClidePlatform *p = plat_native();
    char err[128];
    ClideTimeCfg tc;
    FILE *f;
    long size;
    ctime_default_cfg(&tc);
    CHECK(save_prepare(p, "tsave2"));
    remove("saves/tsave2/chunks.dat");
    CHECK(game_create(g, "tsave2", "saves/tsave2", 1UL, GM_SURVIVAL, 0, &tc));
    CHECK(save_game(g, p, err, sizeof(err)));
    game_destroy(g);
    /* flip one byte in the middle of the file */
    f = fopen("saves/tsave2/world.ini", "r+b");
    CHECK(f != NULL);
    if (f) {
        int c;
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, size / 2, SEEK_SET);
        c = fgetc(f);
        fseek(f, size / 2, SEEK_SET);
        fputc(c == 'x' ? 'y' : 'x', f);
        fclose(f);
    }
    CHECK(!save_verify_file("saves/tsave2/world.ini"));
    CHECK(!save_load(g, p, "tsave2", &tc, err, sizeof(err)));
    CHECK(strstr(err, "CRC") != NULL);
    CHECK(!save_load(g, p, "no_such", &tc, err, sizeof(err)));
}

static void t_world_list(void)
{
    SaveList l;
    int i, found = 0;
    save_list(&l);
    for (i = 0; i < l.count; ++i) {
        if (str_ieq(l.names[i], "tsave1")) found = 1;
    }
    CHECK(found);
    CHECK(save_exists("tsave1"));
    CHECK(!save_exists("nothere"));
}

void suite_save(void)
{
    tf_run("world names are DOS safe", t_names);
    tf_run("save / load round trip", t_roundtrip);
    tf_run("corrupt saves are detected", t_corruption);
    tf_run("world list", t_world_list);
    log_clear();
}
