/* Cliderness - tests/test_ship.c : Phase 8 - ships, sailing, wind, damage, flooding */
#include <stdio.h>
#include <string.h>
#include "tf.h"
#include "gameplay/game.h"
#include "gameplay/vehicles/ship.h"
#include "content/content.h"
#include "content/effects.h"
#include "world/weather/weather.h"
#include "commands/console.h"
#include "save/save.h"
#include "platform/platform.h"
#include "core/log.h"

#define SEA_Y 100   /* water surface cells are at y = 99, ships at y = 100 */

static void ticks(Game *g, long n)
{
    while (n-- > 0) {
        game_tick(g);
    }
}

static void calm(Game *g, const char *weather, int wind_dir)
{
    weather_force(&g->weather, weather_find(weather), 9999);
    g->weather.wind_dir = wind_dir;
}

/* a lake of 40 x 21 cells east of a small beach at x = -2..0 */
static Game *harbour(int mode)
{
    Game *g = &g_game;
    long x, z, y;
    CHECK(tf_make_game(g, mode, NULL));
    for (z = -10; z <= 10; ++z) {
        for (x = -4; x <= 40; ++x) {
            world_set(&g->world, 0, x, SEA_Y - 3, z, (cl_u8)g_cb.stone, 0);
            world_set(&g->world, 0, x, SEA_Y - 2, z, x <= 0 ? (cl_u8)g_cb.stone : (cl_u8)g_cb.water, 0);
            world_set(&g->world, 0, x, SEA_Y - 1, z, x <= 0 ? (cl_u8)g_cb.stone : (cl_u8)g_cb.water, 0);
            for (y = SEA_Y; y < SEA_Y + 6; ++y) {
                world_set(&g->world, 0, x, y, z, BLOCK_AIR, 0);
            }
        }
    }
    game_teleport(g, 0, SEA_Y, 0);
    g->p.facing = DIR_E;
    g->p.god = 0;
    calm(g, "clear", 0);
    return g;
}

static int launch(Game *g, const char *item)
{
    int it = item_find(item), before = ship_count(g), i;
    inv_add(&g->inv, it, 1, g_items[it].durability);
    game_select(g, 0);
    for (i = 0; i < INV_SLOTS; ++i) {
        if (g->inv.s[i].item == it) {
            g->inv.selected = i;
        }
    }
    game_place(g);
    CHECK_EQ_L(ship_count(g), before + 1);
    for (i = MAX_SHIPS - 1; i >= 0; --i) {
        if (g->ships[i].active && g_ships[g->ships[i].def].item == it) return i;
    }
    return -1;
}

static void t_defs(void)
{
    int r = shipdef_find("raft"), l = shipdef_find("large_ship");
    printf("    %d ship types\n", g_ship_count);
    CHECK(g_ship_count >= 4);
    CHECK(r >= 0 && g_ships[r].item == item_find("raft") && g_ships[r].cargo == 0);
    CHECK(l >= 0 && g_ships[l].width == 3 && g_ships[l].length == 5 && g_ships[l].oar_speed == 0);
    CHECK(shipdef_for_item(item_find("sailboat")) == shipdef_find("sailboat"));
    CHECK(g_items[item_find("rowboat")].durability == g_ships[shipdef_find("rowboat")].hull);
}

static void t_launch_board_leave(void)
{
    Game *g = harbour(GM_SURVIVAL);
    int s;
    /* not on land */
    g->p.facing = DIR_W;
    inv_add(&g->inv, item_find("raft"), 1, 30);
    game_select(g, 0);
    game_place(g);
    CHECK_EQ_L(ship_count(g), 0);
    CHECK_EQ_L(inv_count(&g->inv, item_find("raft")), 1);
    g->p.facing = DIR_E;
    game_place(g);
    CHECK_EQ_L(ship_count(g), 1);
    CHECK_EQ_L(inv_count(&g->inv, item_find("raft")), 0);
    s = ship_at(g, 0, 1, SEA_Y, 0);
    CHECK(s >= 0);
    game_interact(g);                       /* E boards */
    CHECK_EQ_L(g->aboard, s);
    CHECK(g->p.x == 1 && g->p.z == 0);
    ticks(g, 5);
    CHECK_EQ_L(g->p.move_mode, MOVE_SAIL);
    CHECK(!fx_has(g, effect_find("wet")));
    game_interact(g);                       /* E leaves: land is next to the raft */
    CHECK_EQ_L(g->aboard, -1);
    CHECK(g->p.x == 0);
    /* F on the empty ship hauls it ashore */
    g->p.facing = DIR_E;
    game_mine(g);
    CHECK_EQ_L(ship_count(g), 0);
    CHECK_EQ_L(inv_count(&g->inv, item_find("raft")), 1);
    game_destroy(g);
}

static void t_rowing(void)
{
    Game *g = harbour(GM_SURVIVAL);
    int s = launch(g, "rowboat"), st0;
    long x0;
    game_interact(g);
    CHECK_EQ_L(g->aboard, s);
    x0 = g->ships[s].x;
    st0 = g->p.stamina;
    game_request_move(g, DIR_E, 0);         /* forward: throttle 1 */
    game_request_move(g, DIR_E, 0);
    game_request_move(g, DIR_E, 0);
    CHECK_EQ_L(g->ships[s].throttle, 3);
    ticks(g, 100);
    CHECK(g->ships[s].x > x0 + 20);         /* ~55 cells per 100 ticks */
    CHECK(g->p.x == g->ships[s].x);
    CHECK(g->p.stamina < st0);
    CHECK(g->p.skill_xp[SK_SAILING] > 0);
    /* turn south, then stop */
    game_request_move(g, DIR_S, 0);
    CHECK_EQ_L(g->ships[s].heading, DIR_S);
    while (g->ships[s].throttle > 0) {
        game_request_move(g, DIR_N, 0);
    }
    CHECK_EQ_L(g->ships[s].throttle, 0);
    x0 = g->ships[s].z;
    ticks(g, 40);
    CHECK_EQ_L(g->ships[s].z, x0);
    game_destroy(g);
}

static void t_wind(void)
{
    Game *g = harbour(GM_CREATIVE);
    int s = launch(g, "sailboat"), down, up;
    game_interact(g);
    g->ships[s].throttle = 3;
    calm(g, "windy", 0);                    /* blowing east = tailwind */
    down = ship_speed(g, s);
    calm(g, "windy", 128);                  /* blowing west = headwind */
    up = ship_speed(g, s);
    printf("    sailing boat: tailwind %d, headwind %d cells/100 ticks\n", down, up);
    CHECK(down > up * 3);
    CHECK_EQ_L(ship_wind_pct(g, s), 80 * 15 / 100);
    calm(g, "windy", 64);                   /* beam reach */
    CHECK_EQ_L(ship_wind_pct(g, s), 80 * 45 / 100);
    /* the merchant ship has no oars: no wind, no speed */
    game_interact(g);
    game_destroy(g);
    g = harbour(GM_CREATIVE);
    s = launch(g, "large_ship");
    game_interact(g);
    CHECK_EQ_L(g->aboard, s);
    g->ships[s].throttle = 3;
    calm(g, "fog", 0);
    CHECK(ship_speed(g, s) <= 3);
    calm(g, "windy", 0);
    CHECK(ship_speed(g, s) > 50);
    game_destroy(g);
}

static void t_aground(void)
{
    Game *g = harbour(GM_SURVIVAL);
    int s = launch(g, "rowboat"), hull;
    long x;
    g->p.god = 1;
    game_interact(g);
    /* turn back toward the beach and row hard into it */
    g->ships[s].x = 6;
    g->p.x = 6;
    g->ships[s].heading = DIR_W;
    g->ships[s].throttle = 3;
    hull = g->ships[s].hull;
    ticks(g, 60);
    x = g->ships[s].x;
    CHECK(x >= 1 && x <= 3);                /* stopped at the shore */
    CHECK_EQ_L(g->ships[s].throttle, 0);
    CHECK(g->ships[s].hull < hull);
    game_destroy(g);
}

static void t_damage_stages_sinking(void)
{
    Game *g = harbour(GM_SURVIVAL);
    int s = launch(g, "raft"), item;
    g->p.god = 1;
    game_interact(g);
    CHECK_EQ_L(ship_stage(g, s), SHIP_INTACT);
    g->ships[s].hull = 15;
    CHECK_EQ_L(ship_stage(g, s), SHIP_DAMAGED);
    ticks(g, 100);
    CHECK(g->ships[s].water >= 4);
    /* bail */
    {
        int w = g->ships[s].water;
        g->inv.selected = 5;
        game_use(g);
        CHECK(g->ships[s].water < w);
    }
    /* repair with logs */
    item = item_find("oak_log");
    inv_add(&g->inv, item, 2, 0);
    {
        int i;
        for (i = 0; i < INV_SLOTS; ++i) if (g->inv.s[i].item == item) g->inv.selected = i;
    }
    game_use(g);
    CHECK(g->ships[s].hull > 15);
    /* a storm wrecks the raft */
    g->ships[s].hull = 6;
    CHECK_EQ_L(ship_stage(g, s), SHIP_CRITICAL);
    calm(g, "storm", 0);
    ticks(g, 20 * 60);
    CHECK(!g->ships[s].active);
    CHECK_EQ_L(g->aboard, -1);
    CHECK(ent_count(g) > 0);                /* wreckage floats */
    game_destroy(g);
}

static void t_cargo_and_load(void)
{
    Game *g = harbour(GM_SURVIVAL);
    int s = launch(g, "rowboat"), ch, k;
    game_interact(g);
    ch = g->ships[s].chest;
    CHECK(ch >= 0);
    game_place(g);                          /* R aboard opens the hold */
    CHECK_EQ_L(g->ui_request, UIREQ_CHEST);
    CHECK_EQ_L(g->ui_arg, ch);
    g->ui_request = UIREQ_NONE;
    CHECK(ship_load(g, s) >= 75);
    for (k = 0; k < 10; ++k) {
        g->chests[ch].s[k].item = (cl_s16)item_find("stone");
        g->chests[ch].s[k].count = 64;
    }
    printf("    rowing boat load %ld / %d kg\n", ship_load(g, s), g_ships[g->ships[s].def].capacity);
    CHECK(ship_load(g, s) > g_ships[g->ships[s].def].capacity);
    g->ships[s].throttle = 2;
    {
        int heavy = ship_speed(g, s);
        for (k = 0; k < 10; ++k) g->chests[ch].s[k].item = -1;
        CHECK(ship_speed(g, s) > heavy);
    }
    /* cannot haul a loaded ship ashore */
    g->chests[ch].s[0].item = (cl_s16)item_find("stone");
    g->chests[ch].s[0].count = 1;
    game_interact(g);
    CHECK_EQ_L(g->aboard, -1);
    g->p.facing = DIR_E;
    game_mine(g);
    CHECK_EQ_L(ship_count(g), 1);
    game_destroy(g);
}

static void t_commands(void)
{
    Game *g = harbour(GM_SURVIVAL);
    const ClidePlatform *p = plat_native();
    CHECK(con_exec(g, p, "ships") == CMD_OK);
    CHECK(con_exec(g, p, "board") == CMD_ERR);
    CHECK(con_exec(g, p, "spawnship sailboat") == CMD_OK);
    CHECK_EQ_L(ship_count(g), 1);
    CHECK(con_exec(g, p, "spawnship zeppelin") == CMD_ERR);
    CHECK(con_exec(g, p, "board") == CMD_OK && g->aboard >= 0);
    CHECK(con_exec(g, p, "throttle 2") == CMD_OK && g->ships[g->aboard].throttle == 2);
    CHECK(con_exec(g, p, "anchor on") == CMD_OK && g->ships[g->aboard].anchored);
    CHECK_EQ_L(ship_speed(g, g->aboard), 0);
    CHECK(con_exec(g, p, "shipinfo") == CMD_OK);
    CHECK(con_exec(g, p, "boats") == CMD_OK);
    CHECK(con_exec(g, p, "wind sw") == CMD_OK && g->weather.wind_dir == 96);
    CHECK(con_exec(g, p, "hull set 50") == CMD_OK && ship_stage(g, g->aboard) == SHIP_DAMAGED);
    CHECK(con_exec(g, p, "bail") == CMD_OK);
    CHECK(con_exec(g, p, "disembark") == CMD_OK && g->aboard < 0);
    CHECK(con_exec(g, p, "board") == CMD_OK);
    CHECK(con_exec(g, p, "sinkship") == CMD_OK);
    ticks(g, 21);
    CHECK_EQ_L(ship_count(g), 0);
    game_destroy(g);
}

static void t_drift_and_crew(void)
{
    Game *g = harbour(GM_SURVIVAL);
    int s = launch(g, "rowboat"), contract = item_find("crew_contract"), st0, i;
    long x0, z0;
    CHECK(contract >= 0);
    g->p.god = 1;
    game_interact(g);
    CHECK_EQ_L(g->aboard, s);
    /* next to the beach the ship is moored: no drift even in a gale */
    calm(g, "storm", 64);
    CHECK(ship_moored(g, s));
    CHECK_EQ_L(ship_drift_speed(g, s), 0);
    /* out on the lake it drifts with the wind (blowing south) */
    g->ships[s].x = 20;
    g->ships[s].z = -6;
    g->p.x = 20;
    g->p.z = -6;
    CHECK(!ship_moored(g, s));
    CHECK_EQ_L(ship_drift_dir(g), DIR_S);
    CHECK(ship_drift_speed(g, s) > 0);
    x0 = g->ships[s].x;
    z0 = g->ships[s].z;
    ticks(g, 100);
    printf("    storm drift: %ld cells in 100 ticks\n", g->ships[s].z - z0);
    CHECK(g->ships[s].z > z0 && g->ships[s].x == x0);
    CHECK(g->p.z == g->ships[s].z);
    /* drifting stops at the shore of the lake */
    ticks(g, 400);
    CHECK(g->ships[s].z <= 10);
    /* the anchor holds, light winds do not move the ship */
    g->ships[s].z = 0;
    g->p.z = 0;
    g->ships[s].anchored = 1;
    CHECK_EQ_L(ship_drift_speed(g, s), 0);
    g->ships[s].anchored = 0;
    calm(g, "clear", 64);
    CHECK_EQ_L(ship_drift_speed(g, s), 0);
    /* hire a sailor: U with a crew contract aboard */
    g->p.god = 0;
    inv_add(&g->inv, contract, 2, 0);
    for (i = 0; i < INV_SLOTS; ++i) {
        if (g->inv.s[i].item == contract) g->inv.selected = i;
    }
    game_use(g);
    CHECK_EQ_L(g->ships[s].crew, 1);
    CHECK_EQ_L(inv_count(&g->inv, contract), 1);
    game_use(g);                            /* a rowing boat has one berth */
    CHECK_EQ_L(g->ships[s].crew, 1);
    CHECK_EQ_L(inv_count(&g->inv, contract), 1);
    /* the crew rows: no stamina used, and a little faster */
    g->ships[s].heading = DIR_E;
    g->ships[s].throttle = 3;
    g->p.stamina = 100;
    st0 = g->p.stamina;
    g->ships[s].water = 0;
    {
        int with = ship_speed(g, s), without;
        g->ships[s].crew = 0;
        without = ship_speed(g, s);
        g->ships[s].crew = 1;
        CHECK(with > without);
    }
    ticks(g, 20);
    CHECK(g->p.stamina >= st0);
    /* the crew bails */
    g->ships[s].throttle = 0;
    g->ships[s].water = 20;
    ticks(g, 100);
    CHECK(g->ships[s].water < 20);
    CHECK(ship_load(g, s) >= 150);          /* player and sailor */
    game_destroy(g);
}

static void t_save_ships(void)
{
    Game *g = &g_game;
    const ClidePlatform *p = plat_native();
    ClideTimeCfg tc;
    char err[128];
    int s, ch;
    long x, z;
    ctime_default_cfg(&tc);
    CHECK(save_prepare(p, "tsave8"));
    remove("saves/tsave8/chunks.dat");
    CHECK(game_create(g, "tsave8", "saves/tsave8", 7UL, GM_SURVIVAL, 1, &tc));
    g->world.replace = p->fs.replace;
    g->spawning = 0;
    g->world.fluid.enabled = 0;
    {
        long xx, zz;
        for (zz = -6; zz <= 6; ++zz) {
            for (xx = -3; xx <= 12; ++xx) {
                world_set(&g->world, 0, xx, SEA_Y - 2, zz, (cl_u8)g_cb.stone, 0);
                world_set(&g->world, 0, xx, SEA_Y - 1, zz, xx <= 0 ? (cl_u8)g_cb.stone : (cl_u8)g_cb.water, 0);
                world_set(&g->world, 0, xx, SEA_Y, zz, BLOCK_AIR, 0);
                world_set(&g->world, 0, xx, SEA_Y + 1, zz, BLOCK_AIR, 0);
            }
        }
    }
    game_teleport(g, 0, SEA_Y, 0);
    g->p.facing = DIR_E;
    s = launch(g, "rowboat");
    game_interact(g);
    g->ships[s].hull = 33;
    g->ships[s].water = 7;
    g->ships[s].crew = 1;
    ch = g->ships[s].chest;
    g->chests[ch].s[3].item = (cl_s16)item_find("bread");
    g->chests[ch].s[3].count = 4;
    x = g->ships[s].x;
    z = g->ships[s].z;
    CHECK(save_game(g, p, err, sizeof(err)));
    game_destroy(g);
    CHECK(save_load(g, p, "tsave8", &tc, err, sizeof(err)));
    CHECK(g->ships[s].active && g->ships[s].x == x && g->ships[s].z == z);
    CHECK_EQ_L(g->ships[s].hull, 33);
    CHECK_EQ_L(g->ships[s].water, 7);
    CHECK_EQ_L(g->ships[s].crew, 1);
    CHECK_EQ_L(g->aboard, s);
    ch = g->ships[s].chest;
    CHECK(ch >= 0 && g->chests[ch].s[3].item == item_find("bread") && g->chests[ch].s[3].count == 4);
    game_destroy(g);
}

void suite_ship(void)
{
    tf_run("ship definitions", t_defs);
    tf_run("launch, board, leave, haul ashore", t_launch_board_leave);
    tf_run("rowing, steering, stamina, skill", t_rowing);
    tf_run("wind and sails", t_wind);
    tf_run("running aground", t_aground);
    tf_run("damage stages, leaks, bailing, repair, sinking", t_damage_stages_sinking);
    tf_run("cargo hold and overloading", t_cargo_and_load);
    tf_run("ship commands", t_commands);
    tf_run("wind drift, mooring, hired crew", t_drift_and_crew);
    tf_run("ships are saved", t_save_ships);
}
