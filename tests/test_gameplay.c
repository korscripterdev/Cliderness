/* Cliderness - tests/test_gameplay.c : movement, stamina, inventory, drops, mining, survival */
#include <string.h>
#include "tf.h"
#include "gameplay/game.h"
#include "content/content.h"
#include "core/log.h"

static void ticks(Game *g, int n)
{
    while (n-- > 0) {
        game_tick(g);
    }
}

/* request a move and run ticks until it happened (or timeout) */
static int step(Game *g, int dir, int sprint)
{
    long ox = g->p.x, oz = g->p.z, oy = g->p.y;
    int t;
    for (t = 0; t < 40; ++t) {
        if (g->p.pending_dir < 0 && (g->p.x != ox || g->p.z != oz || g->p.y != oy)) {
            return t;
        }
        if (g->p.pending_dir < 0 && t == 0) {
            game_request_move(g, dir, sprint);
        }
        game_tick(g);
    }
    return (g->p.x != ox || g->p.z != oz) ? t : -1;
}

static void t_movement(void)
{
    Game *g = &g_game;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 12);
    CHECK(step(g, DIR_E, 0) >= 0);
    CHECK_EQ_L(g->p.x, 1);
    CHECK_EQ_L(g->p.y, 70);
    CHECK(step(g, DIR_S, 0) >= 0);
    CHECK_EQ_L(g->p.z, 1);
    /* wall two blocks high blocks movement */
    world_set(&g->world, 0, 2, 70, 1, (cl_u8)g_cb.stone, 0);
    world_set(&g->world, 0, 2, 71, 1, (cl_u8)g_cb.stone, 0);
    step(g, DIR_E, 0);
    CHECK_EQ_L(g->p.x, 1);
    /* a single block is a step that can be climbed */
    world_set(&g->world, 0, 1, 70, 2, (cl_u8)g_cb.stone, 0);
    CHECK(step(g, DIR_S, 0) >= 0);
    CHECK_EQ_L(g->p.z, 2);
    CHECK_EQ_L(g->p.y, 71);
    /* walking off the step falls back down */
    CHECK(step(g, DIR_W, 0) >= 0);
    CHECK_EQ_L(g->p.y, 70);
    /* a deep hole: falling hurts */
    {
        long y;
        int hp = g->p.hp;
        for (y = 60; y < 70; ++y) {
            world_set(&g->world, 0, -1, y, 2, BLOCK_AIR, 0);
        }
        world_set(&g->world, 0, -1, 59, 2, (cl_u8)g_cb.stone, 0);
        step(g, DIR_W, 0);
        CHECK_EQ_L(g->p.y, 60);
        CHECK(g->p.hp < hp);
    }
    game_destroy(g);
}

static void t_sprint_crouch(void)
{
    Game *g = &g_game;
    int t_walk, t_sprint, t_crouch, st;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 16);
    step(g, DIR_E, 0);
    ticks(g, 10);
    st = g->p.stamina;
    t_walk = step(g, DIR_E, 0);
    CHECK(g->p.stamina >= st);           /* walking costs nothing */
    ticks(g, 10);
    st = g->p.stamina;
    t_sprint = step(g, DIR_E, 1);
    CHECK(g->p.stamina < st);            /* sprinting costs stamina */
    CHECK(g->p.move_mode == MOVE_SPRINT);
    ticks(g, 10);
    game_toggle_crouch(g);
    t_crouch = step(g, DIR_E, 0);
    CHECK(g->p.move_mode == MOVE_CROUCH);
    CHECK(g->p.noise < 30);
    printf("    ticks per step: walk %d sprint %d crouch %d\n", t_walk, t_sprint, t_crouch);
    /* compare cooldowns directly */
    {
        int cw, cs, cc;
        game_toggle_crouch(g);
        step(g, DIR_W, 0); cw = g->p.cooldown;
        ticks(g, 10);
        step(g, DIR_W, 1); cs = g->p.cooldown;
        ticks(g, 10);
        game_toggle_crouch(g);
        step(g, DIR_W, 0); cc = g->p.cooldown;
        CHECK(cs < cw && cw < cc);
    }
    /* stamina regenerates when resting; exhausted players cannot sprint */
    g->p.stamina = 0;
    ticks(g, 40);
    CHECK(g->p.stamina > 0);
    game_toggle_crouch(g);
    g->p.stamina = 5;
    step(g, DIR_E, 1);
    CHECK(g->p.move_mode == MOVE_WALK);
    game_destroy(g);
}

static void t_inventory(void)
{
    Inventory inv;
    int stone = item_find("stone");
    int pick = item_find("iron_pickaxe");
    int slot;
    inv_init(&inv);
    CHECK(inv_add(&inv, stone, 100, 0) == 0);
    CHECK(inv.s[0].count == 64 && inv.s[1].count == 36);    /* stacking */
    CHECK_EQ_L(inv_count(&inv, stone), 100);
    CHECK(inv_add(&inv, pick, 2, 0) == 0);
    CHECK(inv.s[2].count == 1 && inv.s[3].count == 1);     /* tools do not stack */
    CHECK(inv.s[2].dur == g_items[pick].durability);
    slot = inv_split(&inv, 0);
    CHECK(slot >= 0 && inv.s[slot].count == 32 && inv.s[0].count == 32);
    CHECK_EQ_L(inv_remove(&inv, stone, 50), 50);
    CHECK_EQ_L(inv_count(&inv, stone), 50);
    CHECK_EQ_L(inv_weight(&inv), 50L * g_items[stone].weight + 2L * g_items[pick].weight);
    inv_add(&inv, item_find("dirt"), 5, 0);
    inv.s[20] = inv.s[0];
    inv.s[0].item = -1;
    inv.s[0].count = 0;
    inv_sort(&inv);
    CHECK(inv.s[INV_HOTBAR].item >= 0);
    {
        int i, ordered = 1, last = -1;
        for (i = INV_HOTBAR; i < INV_SLOTS && inv.s[i].item >= 0; ++i) {
            if (inv.s[i].item < last) ordered = 0;
            last = inv.s[i].item;
        }
        CHECK(ordered);
    }
    /* full inventory reports leftovers */
    inv_clear(&inv);
    CHECK(inv_add(&inv, stone, 64 * INV_SLOTS + 10, 0) == 10);
    CHECK_EQ_L(inv_free_slots(&inv), 0);
}

static void t_drop_pickup(void)
{
    Game *g = &g_game;
    int stone = item_find("stone");
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 6);
    inv_add(&g->inv, stone, 10, 0);
    g->inv.selected = 0;
    g->p.facing = DIR_E;
    game_drop(g, 0, 0);
    CHECK_EQ_L(inv_count(&g->inv, stone), 9);
    CHECK_EQ_L(ent_count(g), 1);
    CHECK(g->ents[0].x == 1 && g->ents[0].y == 70);
    game_drop(g, 0, 1);                       /* Ctrl+Q drops the whole stack */
    CHECK_EQ_L(inv_count(&g->inv, stone), 0);
    CHECK_EQ_L(ent_count(g), 1);              /* merged on the same cell */
    CHECK(g->ents[0].count == 10);
    game_interact(g);
    CHECK_EQ_L(inv_count(&g->inv, stone), 10);
    CHECK_EQ_L(ent_count(g), 0);
    game_destroy(g);
}

static void t_buoyancy(void)
{
    Game *g = &g_game;
    long x, z, y;
    int e1, e2;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    /* a water pool 5 deep at y 60..64 */
    for (z = -3; z <= 3; ++z) {
        for (x = -3; x <= 3; ++x) {
            world_set(&g->world, 0, x, 59, z, (cl_u8)g_cb.stone, 0);
            for (y = 60; y <= 64; ++y) world_set(&g->world, 0, x, y, z, (cl_u8)g_cb.water, 15);
            for (y = 65; y <= 70; ++y) world_set(&g->world, 0, x, y, z, BLOCK_AIR, 0);
        }
    }
    e1 = ent_spawn(g, 0, 0, 67, 0, item_find("oak_log"), 1, 0);    /* wood, density 700 */
    e2 = ent_spawn(g, 0, 1, 67, 0, item_find("stone"), 1, 0);      /* stone, density 2600 */
    for (y = 0; y < 200; ++y) {
        game_tick(g);
    }
    CHECK(g->ents[e1].floating && g->ents[e1].y == 65);   /* rests on the surface */
    CHECK(!g->ents[e2].floating && g->ents[e2].y == 60);  /* sinks to the bottom */
    game_destroy(g);
}

static void t_mining(void)
{
    Game *g = &g_game;
    int cobble = item_find("cobblestone");
    int pick = item_find("stone_pickaxe");
    int t_hand, t_tool, dur;
    long xp;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 6);
    world_set(&g->world, 0, 1, 70, 0, (cl_u8)g_cb.stone, 0);
    g->p.facing = DIR_E;
    t_hand = mine_time(g, g_cb.stone);
    game_mine(g);
    CHECK(g->p.mining);
    xp = g->p.skill_xp[SK_MINING];
    ticks(g, t_hand + 1);
    CHECK(!g->p.mining);
    CHECK(world_get(&g->world, 0, 1, 70, 0) == BLOCK_AIR);
    CHECK_EQ_L(inv_count(&g->inv, cobble), 1);
    CHECK(g->p.skill_xp[SK_MINING] > xp);                   /* natural block gives xp */
    /* with the right tool it is faster and the tool wears */
    inv_add(&g->inv, pick, 1, 0);
    g->inv.selected = 1;
    CHECK(g->inv.s[1].item == pick);
    t_tool = mine_time(g, g_cb.stone);
    CHECK(t_tool < t_hand);
    dur = g->inv.s[1].dur;
    world_set(&g->world, 0, 1, 70, 0, (cl_u8)g_cb.stone, META_PLACED);
    ticks(g, 30);                                            /* reset per-minute xp cap */
    xp = g->p.skill_xp[SK_MINING];
    game_mine(g);
    ticks(g, t_tool + 1);
    CHECK(world_get(&g->world, 0, 1, 70, 0) == BLOCK_AIR);
    CHECK(g->inv.s[1].dur == dur - 1);
    CHECK(g->p.skill_xp[SK_MINING] == xp);                   /* placed block: no xp */
    /* tool breaks at zero durability */
    g->inv.s[1].dur = 1;
    world_set(&g->world, 0, 1, 70, 0, (cl_u8)g_cb.stone, 0);
    game_mine(g);
    ticks(g, t_tool + 1);
    CHECK(g->inv.s[1].item < 0);
    /* bedrock cannot be mined */
    world_set(&g->world, 0, 1, 70, 0, (cl_u8)g_cb.bedrock, 0);
    game_mine(g);
    CHECK(!g->p.mining);
    /* placing consumes the item and marks the block as placed */
    world_set(&g->world, 0, 1, 70, 0, BLOCK_AIR, 0);
    g->inv.selected = 0;
    {
        int before = inv_count(&g->inv, cobble);
        game_place(g);
        CHECK(world_get(&g->world, 0, 1, 70, 0) == g_items[cobble].place_block);
        CHECK(world_get_meta(&g->world, 0, 1, 70, 0) & META_PLACED);
        CHECK_EQ_L(inv_count(&g->inv, cobble), before - 1);
    }
    /* dig down: player descends */
    {
        long y0 = g->p.y;
        game_dig_down(g);
        ticks(g, 400);
        CHECK(g->p.y == y0 - 1);
    }
    /* spectator / adventure cannot edit */
    game_set_mode(g, GM_ADVENTURE);
    world_set(&g->world, 0, 1, g->p.y, 0, (cl_u8)g_cb.stone, 0);
    game_mine(g);
    CHECK(!g->p.mining);
    game_destroy(g);
}

static void t_survival(void)
{
    Game *g = &g_game;
    int h0, t0;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 4);
    h0 = g->p.hunger;
    t0 = g->p.thirst;
    ticks(g, 20 * 60 * 2);       /* two real minutes = 2.4 game hours */
    CHECK(g->p.hunger < h0);
    CHECK(g->p.thirst < t0);
    CHECK(g->p.thirst < g->p.hunger);
    /* starvation damages, god mode protects */
    g->p.hunger = 0;
    g->p.thirst = 0;
    h0 = g->p.hp;
    ticks(g, 20 * 30);
    CHECK(g->p.hp < h0);
    g->p.god = 1;
    h0 = g->p.hp;
    ticks(g, 20 * 30);
    CHECK(g->p.hp == h0);
    /* eating restores hunger */
    g->p.god = 0;
    g->p.hunger = 100;
    inv_add(&g->inv, item_find("apple"), 1, 0);
    g->inv.selected = 0;
    game_use(g);
    CHECK(g->p.hunger == 100 + g_items[item_find("apple")].food);
    /* death drops the inventory and respawn restores the player */
    inv_add(&g->inv, item_find("dirt"), 5, 0);
    surv_damage(g, 1000, "cause.command");
    CHECK(g->p.dead);
    CHECK_EQ_L(inv_count(&g->inv, item_find("dirt")), 0);
    CHECK(ent_count(g) >= 1);
    game_respawn(g);
    CHECK(!g->p.dead && g->p.hp == g->p.hp_max);
    /* creative mode never drains */
    game_set_mode(g, GM_CREATIVE);
    g->p.hunger = 900;
    ticks(g, 20 * 60);
    CHECK(g->p.hunger >= 500);
    game_destroy(g);
}

static void t_weather_cycle(void)
{
    WeatherState ws;
    int seen[MAX_WEATHER], i, n = 0;
    long m;
    memset(seen, 0, sizeof(seen));
    weather_init(&ws, 5UL);
    for (m = 0; m < 60L * 24L * 200L; ++m) {
        weather_minute(&ws, 50, 70, 0);
        seen[ws.cur] = 1;
        CHECK(!g_weather[ws.cur].ancient || g_weather[ws.cur].ancient == 2);
        if (g_weather[ws.cur].ancient == 1) break;
    }
    for (i = 0; i < g_weather_count; ++i) n += seen[i];
    CHECK(n >= 6);
    CHECK(g_weather_count >= 18);
    /* climate filters: no snow in hot deserts */
    weather_init(&ws, 9UL);
    for (m = 0; m < 60L * 24L * 60L; ++m) {
        weather_minute(&ws, 95, 5, 0);
        if (m > 500 && g_weather[ws.cur].precip == PRECIP_SNOW) break;
    }
    CHECK(m == 60L * 24L * 60L);
}

void suite_gameplay(void)
{
    tf_run("movement, steps, walls and falling", t_movement);
    tf_run("sprint costs stamina, crouch is quiet", t_sprint_crouch);
    tf_run("inventory stacking / split / sort", t_inventory);
    tf_run("item drop and pick up", t_drop_pickup);
    tf_run("items float or sink by density", t_buoyancy);
    tf_run("mining, tools, durability, xp", t_mining);
    tf_run("hunger, thirst, damage, food, death", t_survival);
    tf_run("weather transitions", t_weather_cycle);
    log_clear();
}
