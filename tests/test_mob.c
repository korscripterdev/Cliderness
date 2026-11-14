/* Cliderness - tests/test_mob.c : Phase 7 - creatures, AI, combat, trading */
#include <stdio.h>
#include <string.h>
#include "tf.h"
#include "gameplay/game.h"
#include "gameplay/entities/creature.h"
#include "content/content.h"
#include "content/effects.h"
#include "world/structures/structure.h"
#include "world/weather/weather.h"
#include "commands/console.h"
#include "save/save.h"
#include "platform/platform.h"
#include "core/log.h"

static void ticks(Game *g, long n)
{
    while (n-- > 0) {
        game_tick(g);
    }
}

static void at_time(Game *g, int hour)
{
    ctime_set_time_of_day(&g->time, (long)hour * 3600L);
    weather_force(&g->weather, weather_find("clear"), 9999);
    /* do not simulate the skipped hours */
    g->last_minute = ctime_total_minutes(&g->time);
}

/* survival test world: flat platform high in the open air, facing east */
static Game *arena(void)
{
    Game *g = &g_game;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 110, 0, 14);
    g->p.facing = DIR_E;
    at_time(g, 12);
    return g;
}

static int spawn(Game *g, const char *id, long dx, long dz)
{
    return mob_spawn(g, creature_find(id), 0, g->p.x + dx, g->p.y, g->p.z + dz);
}

static void t_defs(void)
{
    int i, ok = 1, tr = creature_find("trader");
    printf("    %d creatures, %d loot tables\n", g_creature_count, g_loot_count);
    CHECK(g_creature_count >= 20);
    for (i = 0; i < g_creature_count; ++i) {
        const CreatureDef *d = &g_creatures[i];
        if (d->hp <= 0 || d->speed < 1 || (d->loot[0] && loot_find(d->loot) < 0)) ok = 0;
    }
    CHECK(ok);
    CHECK(creature_find("cliderness:zombie") == creature_find("zombie"));
    CHECK(g_creatures[creature_find("zombie")].burns == 1);
    CHECK(g_creatures[creature_find("zombie")].kind == MK_MONSTER);
    CHECK(g_creatures[creature_find("fish")].aquatic == 1);
    CHECK(g_creatures[creature_find("spider")].effect == effect_find("poisoned"));
    CHECK(tr >= 0 && g_creatures[tr].ntrades == 3 && g_creatures[tr].trade_in[0] == item_find("coal"));
    CHECK(g_blocks[block_find("gravel")].drop2 == item_find("flint"));
    CHECK(strcmp(creature_name(creature_find("deer")), "Deer") == 0);
    CHECK(difficulty_find("hard") == DIFF_HARD && difficulty_find("2") == DIFF_NORMAL && difficulty_find("x") < 0);
}

static void t_melee(void)
{
    Game *g = arena();
    int m = spawn(g, "deer", 1, 0), guard = 0;
    long xp0 = g->p.skill_xp[SK_MELEE];
    inv_add(&g->inv, item_find("iron_sword"), 1, g_items[item_find("iron_sword")].durability);
    game_select(g, 0);
    CHECK(m >= 0);
    CHECK_EQ_L(mob_at(g, 0, g->p.x + 1, g->p.y, g->p.z), m);
    CHECK(game_attack(g) == 1);
    CHECK(g->mobs[m].hp < g_creatures[g->mobs[m].def].hp);
    CHECK(g->mobs[m].aggro == 1);
    CHECK(game_attack(g) == 1);   /* still cooling down: no second hit */
    while (g->mobs[m].active && guard++ < 20) {
        g->p.attack_cd = 0;
        game_attack(g);
    }
    CHECK(!g->mobs[m].active);
    CHECK_EQ_L(g->kills, 1);
    CHECK(ent_count(g) > 0);              /* meat / leather */
    CHECK(g->p.skill_xp[SK_MELEE] > xp0);
    CHECK(g->inv.s[0].dur < g_items[item_find("iron_sword")].durability);
    /* nothing in front: F mines instead */
    g->p.attack_cd = 0;
    CHECK(game_attack(g) == 0);
    game_destroy(g);
}

static void t_bow(void)
{
    Game *g = arena();
    int bow = item_find("bow"), arrow = item_find("arrow");
    int m = spawn(g, "sheep", 5, 0);
    inv_add(&g->inv, bow, 1, g_items[bow].durability);
    game_select(g, 0);
    CHECK(game_attack(g) == 1);           /* no arrows */
    CHECK(g->mobs[m].hp == g_creatures[g->mobs[m].def].hp);
    inv_add(&g->inv, arrow, 5, 0);
    CHECK(game_attack(g) == 1);
    CHECK_EQ_L(inv_count(&g->inv, arrow), 4);
    CHECK(!g->mobs[m].active || g->mobs[m].hp < g_creatures[g->mobs[m].def].hp);
    CHECK(g->p.skill_xp[SK_RANGED] > 0 || !g->mobs[m].active);
    /* a wall stops arrows */
    g->p.attack_cd = 0;
    world_set(&g->world, 0, g->p.x + 2, g->p.y, g->p.z, (cl_u8)g_cb.stone, 0);
    {
        int hp = g->mobs[m].hp;
        game_attack(g);
        CHECK(!g->mobs[m].active || g->mobs[m].hp == hp);
    }
    game_destroy(g);
}

static void t_monster_attacks(void)
{
    Game *g = arena();
    int z, hp0;
    at_time(g, 0);
    z = spawn(g, "zombie", 2, 0);
    hp0 = g->p.hp;
    CHECK(ent_hostile_near(g, 12));
    CHECK(!game_sleep(g, 0));             /* cannot sleep with a monster near */
    ticks(g, 200);
    CHECK(g->mobs[z].active);
    CHECK(g->p.hp < hp0);
    CHECK(g->mobs[z].state == MS_ATTACK || g->mobs[z].state == MS_CHASE);
    /* armour reduces the damage */
    {
        int d1, d2;
        g->p.hp = g->p.hp_max;
        g->mobs[z].cd_attack = 0;
        hp0 = g->p.hp;
        ticks(g, 30);
        d1 = hp0 - g->p.hp;
        inv_add(&g->inv, item_find("iron_chestplate"), 1, g_items[item_find("iron_chestplate")].durability);
        inv_equip(&g->inv, 0);
        g->p.hp = g->p.hp_max;
        hp0 = g->p.hp;
        ticks(g, 30);
        d2 = hp0 - g->p.hp;
        CHECK(d1 > 0 && d2 > 0 && d2 <= d1);
    }
    /* creative players are ignored */
    g->p.hp = g->p.hp_max;
    game_set_mode(g, GM_CREATIVE);
    ticks(g, 100);
    CHECK_EQ_L(g->p.hp, g->p.hp_max);
    game_destroy(g);
}

static void t_daylight_burns(void)
{
    Game *g = arena();
    int z = spawn(g, "zombie", 6, 6), s = spawn(g, "spider", -6, 6);
    g->p.god = 1;
    ticks(g, 400);
    CHECK(!g->mobs[z].active || g->mobs[z].hp < g_creatures[g->mobs[z].def].hp);
    CHECK(g->mobs[s].active);             /* spiders do not burn */
    game_destroy(g);
}

static void t_animals_flee(void)
{
    Game *g = arena();
    int d = spawn(g, "deer", 2, 0);
    long d0 = 2, d1;
    g->p.noise = 80;
    ticks(g, 120);
    d1 = (g->mobs[d].x - g->p.x) * (g->mobs[d].x - g->p.x) + (g->mobs[d].z - g->p.z) * (g->mobs[d].z - g->p.z);
    CHECK(g->mobs[d].active);
    CHECK(d1 > d0 * d0);
    CHECK(!ent_hostile_near(g, 12));
    game_destroy(g);
}

static void t_difficulty(void)
{
    Game *g = arena();
    const ClidePlatform *p = plat_native();
    int z;
    at_time(g, 0);
    z = spawn(g, "zombie", 1, 0);
    spawn(g, "sheep", -3, 0);
    g->difficulty = DIFF_PEACEFUL;
    CHECK(!ent_hostile_near(g, 12));
    ticks(g, 100);
    CHECK_EQ_L(g->p.hp, g->p.hp_max);
    CHECK(con_exec(g, p, "difficulty hard") == CMD_OK && g->difficulty == DIFF_HARD);
    CHECK(con_exec(g, p, "difficulty peaceful") == CMD_OK);
    CHECK(!g->mobs[z].active);
    CHECK_EQ_L(mob_count(g), 1);          /* the sheep stays */
    game_destroy(g);
}

static void t_trading(void)
{
    Game *g = arena();
    int t = spawn(g, "trader", 1, 0);
    CHECK(t >= 0);
    CHECK(ent_interact(g, g->p.x + 1, g->p.y, g->p.z));   /* lists offers */
    CHECK_EQ_L(inv_count(&g->inv, item_find("trade_token")), 0);
    inv_add(&g->inv, item_find("coal"), 7, 0);
    CHECK(ent_interact(g, g->p.x + 1, g->p.y, g->p.z));
    CHECK_EQ_L(inv_count(&g->inv, item_find("trade_token")), 1);
    CHECK_EQ_L(inv_count(&g->inv, item_find("coal")), 1);
    CHECK(!ent_interact(g, g->p.x - 1, g->p.y, g->p.z));  /* nobody there */
    game_destroy(g);
}

static void t_natural_spawning(void)
{
    Game *g = &g_game;
    int monsters = 0, i;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    g->spawning = 1;
    g->p.god = 1;
    at_time(g, 0);
    ticks(g, 2400);
    for (i = 0; i < MAX_MOBS; ++i) {
        if (g->mobs[i].active && g_creatures[g->mobs[i].def].kind == MK_MONSTER) ++monsters;
    }
    printf("    %d creatures after 2 minutes at night (%d monsters)\n", mob_count(g), monsters);
    CHECK(mob_count(g) > 0);
    CHECK(mob_count(g) <= 16 + 6 * g->difficulty);
    g->spawning = 0;
    game_teleport(g, g->p.x + 500, g->p.y, g->p.z);
    ticks(g, 2);
    CHECK_EQ_L(mob_count(g), 0);          /* far away creatures despawn */
    game_destroy(g);
}

static void t_commands(void)
{
    Game *g = arena();
    const ClidePlatform *p = plat_native();
    CHECK(con_exec(g, p, "summon wolf 3") == CMD_OK);
    CHECK_EQ_L(mob_count(g), 3);
    CHECK(con_exec(g, p, "summon dragon") == CMD_ERR);
    CHECK(con_exec(g, p, "creatures monster") == CMD_OK);
    CHECK(con_exec(g, p, "bestiary trader") == CMD_OK);
    CHECK(con_exec(g, p, "mobs") == CMD_OK);
    CHECK(con_exec(g, p, "summon sheep") == CMD_OK);
    CHECK(con_exec(g, p, "kill @e[type=wolf]") == CMD_OK);
    CHECK_EQ_L(mob_count(g), 1);
    CHECK(con_exec(g, p, "kill @e[type=dragon]") == CMD_ERR);
    CHECK(con_exec(g, p, "despawn all") == CMD_OK);
    CHECK_EQ_L(mob_count(g), 0);
    CHECK(con_exec(g, p, "spawning off") == CMD_OK && !g->spawning);
    CHECK(con_exec(g, p, "threat") == CMD_OK);
    CHECK(con_exec(g, p, "attack") == CMD_ERR);
    game_destroy(g);
}

static void t_save_mobs(void)
{
    Game *g = &g_game;
    const ClidePlatform *p = plat_native();
    ClideTimeCfg tc;
    char err[128];
    int m, i, found = 0;
    ctime_default_cfg(&tc);
    CHECK(save_prepare(p, "tsave7"));
    remove("saves/tsave7/chunks.dat");
    CHECK(game_create(g, "tsave7", "saves/tsave7", 7UL, GM_SURVIVAL, 1, &tc));
    g->world.replace = p->fs.replace;
    g->spawning = 0;
    tf_flat_area(g, 0, 110, 0, 4);
    m = mob_spawn(g, creature_find("bear"), 0, 2, 110, 2);
    g->mobs[m].hp = 17;
    g->mobs[m].aggro = 1;
    g->difficulty = DIFF_EASY;
    g->kills = 5;
    CHECK(save_game(g, p, err, sizeof(err)));
    game_destroy(g);
    CHECK(save_load(g, p, "tsave7", &tc, err, sizeof(err)));
    for (i = 0; i < MAX_MOBS; ++i) {
        const Mob *b = &g->mobs[i];
        if (b->active && b->def == creature_find("bear") && b->x == 2 && b->z == 2 && b->hp == 17 && b->aggro) {
            found = 1;
        }
    }
    CHECK(found);
    CHECK_EQ_L(g->difficulty, DIFF_EASY);
    CHECK_EQ_L(g->kills, 5);
    game_destroy(g);
}

void suite_mob(void)
{
    tf_run("creature definitions and loot", t_defs);
    tf_run("melee combat, loot, xp, durability", t_melee);
    tf_run("bow and arrows", t_bow);
    tf_run("monsters attack at night, armour", t_monster_attacks);
    tf_run("undead burn in daylight", t_daylight_burns);
    tf_run("animals flee from noise", t_animals_flee);
    tf_run("difficulty and peaceful", t_difficulty);
    tf_run("NPC trading", t_trading);
    tf_run("natural spawning and despawning", t_natural_spawning);
    tf_run("creature commands", t_commands);
    tf_run("creatures are saved", t_save_mobs);
}
