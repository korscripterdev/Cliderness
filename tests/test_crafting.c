/* Cliderness - tests/test_crafting.c : Phase 4 - crafting, farming, chests, equipment, effects, sleep */
#include <stdio.h>
#include <string.h>
#include "tf.h"
#include "gameplay/game.h"
#include "content/content.h"
#include "content/recipes.h"
#include "content/effects.h"
#include "save/save.h"
#include "platform/platform.h"
#include "core/log.h"

static void ticks(Game *g, long n)
{
    while (n-- > 0) {
        game_tick(g);
    }
}

static int R(const char *id)
{
    return recipe_find(id);
}

static void t_recipes_loaded(void)
{
    int i, ok = 1;
    printf("    %d recipes, %d effects\n", g_recipe_count, g_effect_count);
    CHECK(g_recipe_count >= 50);
    CHECK(g_effect_count >= 8);
    for (i = 0; i < g_recipe_count; ++i) {
        if (g_recipes[i].result < 0 || g_recipes[i].nin == 0) ok = 0;
    }
    CHECK(ok);
    CHECK(g_items[item_find("coal")].fuel == 8);
    CHECK(g_items[item_find("oak_log")].fuel == 4);
    CHECK(g_items[item_find("wheat_seeds")].place_block == block_find("wheat_0"));
    CHECK(g_items[item_find("bandage")].cure == effect_find("bleeding"));
    CHECK(g_items[item_find("iron_pickaxe")].repair_item == item_find("iron_ingot"));
    CHECK(g_blocks[block_find("oak_leaves")].drop2 == item_find("oak_sapling"));
    CHECK(g_blocks[block_find("workbench")].station == STATION_WORKBENCH);
}

static void t_crafting(void)
{
    Game *g = &g_game;
    int planks = item_find("oak_planks"), wb = block_find("workbench");
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 8);
    inv_add(&g->inv, item_find("oak_log"), 3, 0);
    CHECK(craft_check(g, R("oak_planks")) == CRAFT_OK);
    CHECK(craft_do(g, R("oak_planks")) == CRAFT_OK);
    CHECK_EQ_L(inv_count(&g->inv, planks), 4);
    CHECK_EQ_L(inv_count(&g->inv, item_find("oak_log")), 2);
    CHECK(craft_max(g, R("oak_planks")) == 2);
    CHECK(g->p.skill_xp[SK_WOODCUTTING] > 0);
    /* a chest needs a workbench nearby */
    inv_add(&g->inv, planks, 8, 0);
    CHECK(craft_check(g, R("chest")) == CRAFT_NEED_STATION);
    world_set(&g->world, 0, 2, 70, 0, (cl_u8)wb, META_PLACED);
    CHECK(craft_station_near(g, STATION_WORKBENCH));
    CHECK(craft_check(g, R("chest")) == CRAFT_OK);
    CHECK(craft_do(g, R("chest")) == CRAFT_OK);
    CHECK_EQ_L(inv_count(&g->inv, item_find("chest")), 1);
    /* smelting needs a furnace and fuel */
    world_set(&g->world, 0, -2, 70, 0, (cl_u8)block_find("furnace"), META_PLACED);
    inv_add(&g->inv, item_find("raw_iron"), 2, 0);
    inv_remove(&g->inv, planks, 999);
    inv_remove(&g->inv, item_find("oak_log"), 999);
    inv_remove(&g->inv, item_find("chest"), 999);         /* chests burn too */
    CHECK(craft_check(g, R("iron_ingot")) == CRAFT_NEED_FUEL);
    inv_add(&g->inv, item_find("stick"), 1, 0);      /* 1 unit: not enough */
    CHECK(craft_check(g, R("iron_ingot")) == CRAFT_NEED_FUEL);
    inv_add(&g->inv, item_find("coal"), 1, 0);       /* 8 units */
    CHECK(craft_do(g, R("iron_ingot")) == CRAFT_OK);
    CHECK_EQ_L(inv_count(&g->inv, item_find("iron_ingot")), 1);
    CHECK_EQ_L(inv_count(&g->inv, item_find("stick")), 0);  /* cheapest fuel burnt first */
    /* missing skill */
    CHECK(craft_check(g, R("anvil")) == CRAFT_NEED_SKILL || craft_check(g, R("anvil")) == CRAFT_NEED_ITEMS);
    /* creative crafts for free */
    game_set_mode(g, GM_CREATIVE);
    CHECK(craft_do(g, R("iron_chestplate")) == CRAFT_OK);
    CHECK_EQ_L(inv_count(&g->inv, item_find("iron_chestplate")), 1);
    game_destroy(g);
}

static void t_repair(void)
{
    Game *g = &g_game;
    int pick = item_find("stone_pickaxe");
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 4);
    inv_add(&g->inv, pick, 1, 0);
    g->inv.selected = 0;
    g->inv.s[0].dur = 10;
    CHECK(repair_check(g, 0) != CRAFT_OK);                 /* no station, no material */
    world_set(&g->world, 0, 1, 70, 0, (cl_u8)block_find("workbench"), 0);
    CHECK(repair_check(g, 0) == CRAFT_NEED_ITEMS);
    inv_add(&g->inv, item_find("cobblestone"), 2, 0);
    CHECK(repair_do(g, 0) == CRAFT_OK);
    CHECK(g->inv.s[0].dur > 10 + g_items[pick].durability / 5);
    CHECK_EQ_L(inv_count(&g->inv, item_find("cobblestone")), 1);
    game_destroy(g);
}

static void t_farming(void)
{
    Game *g = &g_game;
    int seeds = item_find("wheat_seeds"), wheat = item_find("wheat");
    int ripe = block_find("wheat_3");
    long guard;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 4);
    world_set(&g->world, 0, 1, 69, 0, (cl_u8)g_cb.grass, 0);
    g->p.facing = DIR_E;
    inv_add(&g->inv, item_find("wooden_hoe"), 1, 0);
    inv_add(&g->inv, seeds, 3, 0);
    g->inv.selected = 0;
    game_use(g);                                         /* till */
    CHECK(world_get(&g->world, 0, 1, 69, 0) == block_find("farmland"));
    CHECK(g->inv.s[0].dur == g_items[item_find("wooden_hoe")].durability - 1);
    /* seeds only grow on farmland */
    g->inv.selected = 1;
    world_set(&g->world, 0, 0, 69, 1, (cl_u8)g_cb.stone, 0);
    game_place(g);
    CHECK(world_get(&g->world, 0, 1, 70, 0) == block_find("wheat_0"));
    CHECK_EQ_L(farm_count(g), 1);
    CHECK_EQ_L(inv_count(&g->inv, seeds), 2);
    g->p.facing = DIR_S;                                 /* stone below: refused */
    game_place(g);
    CHECK(world_get(&g->world, 0, 0, 70, 1) == BLOCK_AIR);
    g->p.facing = DIR_E;
    g->p.god = 1;                                        /* survive the long wait */
    /* grow: 3 stages of ~240 game minutes; 1 game minute = 16.7 ticks */
    for (guard = 0; guard < 20000L && world_get(&g->world, 0, 1, 70, 0) != ripe; ++guard) {
        game_tick(g);
        g->p.hunger = g->p.thirst = 900;
    }
    CHECK(world_get(&g->world, 0, 1, 70, 0) == ripe);
    printf("    wheat ripened after %ld ticks (%ld game minutes)\n", guard, guard * 36L / 600L);
    CHECK_EQ_L(farm_count(g), 0);
    /* harvest */
    game_mine(g);
    ticks(g, 30);
    CHECK_EQ_L(inv_count(&g->inv, wheat), 1);
    CHECK_EQ_L(inv_count(&g->inv, seeds), 4);
    /* sapling -> tree */
    world_set(&g->world, 0, 1, 70, 0, BLOCK_AIR, 0);
    inv_add(&g->inv, item_find("oak_sapling"), 1, 0);
    g->inv.selected = inv_count(&g->inv, seeds) ? 2 : 1;
    {
        int i;
        for (i = 0; i < INV_HOTBAR; ++i) {
            if (g->inv.s[i].item == item_find("oak_sapling")) g->inv.selected = i;
        }
    }
    game_place(g);
    CHECK(world_get(&g->world, 0, 1, 70, 0) == block_find("oak_sapling"));
    CHECK(farm_grow_all(g, 8) >= 1);
    CHECK(world_get(&g->world, 0, 1, 70, 0) == g_cb.log);
    CHECK(world_get(&g->world, 0, 1, 71, 0) == g_cb.leaves);
    game_destroy(g);
}

static void t_chests(void)
{
    Game *g = &g_game;
    int chest = item_find("chest"), stone = item_find("stone"), c;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 4);
    g->p.facing = DIR_E;
    inv_add(&g->inv, chest, 1, 0);
    g->inv.selected = 0;
    game_place(g);
    c = chest_find(g, 0, 1, 70, 0);
    CHECK(c >= 0 && chest_count(g) == 1);
    game_interact(g);
    CHECK(g->ui_request == UIREQ_CHEST && g->ui_arg == c);
    g->ui_request = UIREQ_NONE;
    g->chests[c].s[0].item = (cl_s16)stone;
    g->chests[c].s[0].count = 20;
    /* breaking the chest drops its contents */
    game_mine(g);
    ticks(g, 400);
    CHECK(chest_count(g) == 0);
    CHECK(world_get(&g->world, 0, 1, 70, 0) == BLOCK_AIR);
    game_interact(g);
    CHECK_EQ_L(inv_count(&g->inv, stone), 20);
    CHECK_EQ_L(inv_count(&g->inv, chest), 1);
    game_destroy(g);
}

static void t_equipment(void)
{
    Game *g = &g_game;
    long cap0;
    int shirt = item_find("fiber_shirt"), bag = item_find("fiber_bag"), plate = item_find("iron_chestplate");
    int env0;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 3);
    cap0 = game_carry_capacity(g);
    inv_add(&g->inv, shirt, 1, 0);
    inv_add(&g->inv, bag, 1, 0);
    inv_add(&g->inv, plate, 1, 0);
    CHECK(inv_equip(&g->inv, 0));
    CHECK(g->inv.equip[EQ_BODY].item == shirt && g->inv.s[0].item < 0);
    CHECK(inv_insulation(&g->inv) == 4);
    CHECK(inv_equip(&g->inv, 1));
    CHECK(game_carry_capacity(g) == cap0 + 15000L);
    CHECK(inv_equip(&g->inv, 2));                          /* swaps the shirt out */
    CHECK(g->inv.equip[EQ_BODY].item == plate);
    CHECK(g->inv.s[2].item == shirt);
    CHECK(inv_defense(&g->inv) == 20);
    CHECK(!inv_equip(&g->inv, 5));                         /* empty slot */
    CHECK(inv_unequip(&g->inv, EQ_BODY));
    CHECK(g->inv.equip[EQ_BODY].item < 0 && inv_count(&g->inv, plate) == 1);
    /* clothing warms in the cold */
    inv_equip(&g->inv, 2);
    g->weather.cur = weather_find("cold_snap");
    env0 = surv_env_temp(g);
    inv_unequip(&g->inv, EQ_BODY);
    CHECK(surv_env_temp(g) < env0 || env0 >= 20);
    game_destroy(g);
}

static void t_effects(void)
{
    Game *g = &g_game;
    int bleed = effect_find("bleeding"), hp;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 3);
    fx_add(g, bleed, 5);
    CHECK(fx_has(g, bleed));
    hp = g->p.hp;
    ticks(g, 17 * 3);                                       /* ~3 game minutes */
    CHECK(g->p.hp < hp);
    inv_add(&g->inv, item_find("bandage"), 1, 0);
    g->inv.selected = 0;
    game_use(g);
    CHECK(!fx_has(g, bleed));
    /* fracture slows movement */
    fx_add(g, effect_find("fracture"), 0);
    CHECK(fx_move(g) == 4);
    fx_clear(g);
    CHECK(fx_move(g) == 0);
    /* swimming makes you wet */
    {
        long x, z;
        for (z = -1; z <= 1; ++z) for (x = -1; x <= 1; ++x) {
            world_set(&g->world, 0, x, 69, z, (cl_u8)g_cb.water, 15);
            world_set(&g->world, 0, x, 68, z, (cl_u8)g_cb.stone, 0);
        }
    }
    ticks(g, 40);
    CHECK(fx_has(g, effect_find("wet")));
    /* effects expire */
    fx_clear(g);
    fx_add(g, effect_find("well_fed"), 2);
    ticks(g, 17 * 4);
    CHECK(!fx_has(g, effect_find("well_fed")));
    /* falls of 7+ blocks fracture bones */
    fx_clear(g);
    tf_flat_area(g, 20, 70, 20, 2);
    {
        long y;
        for (y = 60; y < 70; ++y) world_set(&g->world, 0, 21, y, 20, BLOCK_AIR, 0);
        world_set(&g->world, 0, 21, 61, 20, (cl_u8)g_cb.stone, 0);
    }
    game_request_move(g, DIR_E, 0);
    ticks(g, 10);
    CHECK(g->p.y == 62);
    CHECK(fx_has(g, effect_find("fracture")));
    CHECK(fx_has(g, effect_find("bleeding")));
    game_destroy(g);
}

static void t_sleep(void)
{
    Game *g = &g_game;
    ClideDate d;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 3);
    world_set(&g->world, 0, 1, 70, 0, (cl_u8)block_find("bed"), 0);
    g->p.facing = DIR_E;
    ctime_set_time_of_day(&g->time, 12L * 3600L);
    game_interact(g);                                      /* noon, rested: refused */
    ctime_date(&g->time, &d);
    CHECK(d.hour == 12);
    ctime_set_time_of_day(&g->time, 22L * 3600L);
    g->last_minute = ctime_total_minutes(&g->time);
    g->p.fatigue = 700;
    game_interact(g);
    ctime_date(&g->time, &d);
    CHECK(d.hour >= 5 && d.hour <= 8);
    CHECK(g->p.fatigue < 100);
    CHECK(fx_has(g, effect_find("rested")));
    CHECK(g->p.spawn_x == g->p.x && g->p.spawn_z == g->p.z);
    game_destroy(g);
}

static void t_save_phase4(void)
{
    Game *g = &g_game;
    const ClidePlatform *p = plat_native();
    ClideTimeCfg tc;
    char err[128];
    int c;
    ctime_default_cfg(&tc);
    CHECK(save_prepare(p, "tsave4"));
    remove("saves/tsave4/chunks.dat");
    CHECK(game_create(g, "tsave4", "saves/tsave4", 7UL, GM_SURVIVAL, 1, &tc));
    g->world.replace = p->fs.replace;
    tf_flat_area(g, 0, 70, 0, 3);
    inv_add(&g->inv, item_find("iron_helmet"), 1, 0);
    inv_equip(&g->inv, 0);
    fx_add(g, effect_find("common_cold"), 99);
    c = chest_create(g, 0, 2, 70, 2);
    g->chests[c].s[5].item = (cl_s16)item_find("bread");
    g->chests[c].s[5].count = 3;
    world_set(&g->world, 0, -1, 70, 0, (cl_u8)block_find("wheat_1"), META_PLACED);
    farm_register(g, 0, -1, 70, 0);
    CHECK(save_game(g, p, err, sizeof(err)));
    game_destroy(g);
    CHECK(save_load(g, p, "tsave4", &tc, err, sizeof(err)));
    CHECK(g->inv.equip[EQ_HEAD].item == item_find("iron_helmet"));
    CHECK(fx_has(g, effect_find("common_cold")));
    c = chest_find(g, 0, 2, 70, 2);
    CHECK(c >= 0 && g->chests[c].s[5].item == item_find("bread") && g->chests[c].s[5].count == 3);
    CHECK_EQ_L(farm_count(g), 1);
    game_destroy(g);
}

void suite_crafting(void)
{
    tf_run("recipes, effects and new item fields", t_recipes_loaded);
    tf_run("crafting, stations, smelting, fuel", t_crafting);
    tf_run("repair with materials", t_repair);
    tf_run("tilling, planting, growth, harvest, trees", t_farming);
    tf_run("chests", t_chests);
    tf_run("equipment, bags, warmth, defense", t_equipment);
    tf_run("injuries, illness, buffs", t_effects);
    tf_run("sleeping in a bed", t_sleep);
    tf_run("phase 4 state is saved", t_save_phase4);
    log_clear();
}
