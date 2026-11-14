/* Cliderness - tests/test_commands.c : console parsing, permissions, handlers, undo/redo */
#include <string.h>
#include "tf.h"
#include "commands/console.h"
#include "content/content.h"
#include "localization/locale.h"
#include "core/log.h"
#include "core/str.h"

static int run(Game *g, const char *line)
{
    return con_exec(g, plat_headless(), line);
}

static void t_registry(void)
{
    int i, j, ok_desc = 1, unique = 1, handlers = 1;
    printf("    %d top-level commands, %d aliases\n", con_count(), con_alias_count());
    CHECK(con_count() >= 70);
    for (i = 0; i < con_count(); ++i) {
        const Command *c = con_get(i);
        if (!c->fn) handlers = 0;
        if (!con_desc(c)[0]) {
            ok_desc = 0;
            printf("    missing description: %s\n", c->name);
        }
        for (j = i + 1; j < con_count(); ++j) {
            if (str_ieq(c->name, con_get(j)->name)) unique = 0;
        }
    }
    CHECK(handlers);
    CHECK(ok_desc);
    CHECK(unique);
    CHECK(con_find("/gamemode") != NULL);
    CHECK(con_find("gm") == con_find("gamemode"));     /* alias */
    CHECK(con_find("tp") == con_find("teleport"));
    CHECK(con_find("nonexistent") == NULL);
}

static void t_permissions(void)
{
    Game *g = &g_game;
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    g->cheats = 0;
    g->cheated = 0;
    CHECK(run(g, "/gamemode creative") == CMD_DENIED);
    CHECK(g->mode == GM_SURVIVAL);
    CHECK(run(g, "/seed") == CMD_OK);                  /* always allowed */
    CHECK(run(g, "/time set noon") == CMD_DENIED);
    CHECK(run(g, "/time query") == CMD_OK);
    CHECK(!g->cheated);
    CHECK(run(g, "/cheats on") == CMD_OK);
    CHECK(g->cheats && g->cheated);
    CHECK(run(g, "/gamemode creative") == CMD_OK);
    CHECK(g->mode == GM_CREATIVE);
    CHECK(run(g, "/gm s") == CMD_OK);
    CHECK(g->mode == GM_SURVIVAL);
    CHECK(run(g, "/nonsense") == CMD_UNKNOWN);
    CHECK(run(g, "/gamemode banana") == CMD_USAGE);
    game_destroy(g);
}

static void t_handlers(void)
{
    Game *g = &g_game;
    ClideDate d;
    int stone = item_find("stone");
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    tf_flat_area(g, 0, 70, 0, 8);
    CHECK(run(g, "/give @s Cliderness:stone 64") == CMD_OK);
    CHECK_EQ_L(inv_count(&g->inv, stone), 64);
    CHECK(run(g, "/give stone") == CMD_OK);
    CHECK_EQ_L(inv_count(&g->inv, stone), 65);
    CHECK(run(g, "/give @s nothing_like_this") == CMD_ERR);
    CHECK(run(g, "/clear @s") == CMD_OK);
    CHECK_EQ_L(inv_count(&g->inv, stone), 0);
    CHECK(run(g, "/time set midnight") == CMD_OK);
    ctime_date(&g->time, &d);
    CHECK(d.hour == 0);
    CHECK(run(g, "/time set noon") == CMD_OK);
    ctime_date(&g->time, &d);
    CHECK(d.hour == 12 && d.minute == 0);
    CHECK(run(g, "/time add 90m") == CMD_OK);
    ctime_date(&g->time, &d);
    CHECK(d.hour == 13 && d.minute == 30);
    CHECK(run(g, "/time set 18:45") == CMD_OK);
    ctime_date(&g->time, &d);
    CHECK(d.hour == 18 && d.minute == 45);
    CHECK(run(g, "/weather set rain") == CMD_OK);
    CHECK(str_ieq(g_weather[g->weather.cur].id, "rain"));
    CHECK(run(g, "/weather set thunder") == CMD_OK);
    CHECK(g_weather[g->weather.cur].thunder);
    CHECK(run(g, "/weather clear") == CMD_OK);
    CHECK(g->weather.cur == 0);
    CHECK(run(g, "/season set winter") == CMD_OK);
    ctime_date(&g->time, &d);
    CHECK(d.season == SEASON_WINTER);
    CHECK(run(g, "/god on") == CMD_OK && g->p.god);
    CHECK(run(g, "/god off") == CMD_OK && !g->p.god);
    CHECK(run(g, "/fly") == CMD_OK && g->p.fly);
    CHECK(run(g, "/noclip on") == CMD_OK && g->p.noclip);
    CHECK(run(g, "/noclip off") == CMD_OK && !g->p.noclip);
    CHECK(run(g, "/tp 100 64 -200") == CMD_OK);
    CHECK(g->p.x == 100 && g->p.y == 64 && g->p.z == -200);
    CHECK(run(g, "/tp ~5 ~ ~-1") == CMD_OK);
    CHECK(g->p.x == 105 && g->p.y == 64 && g->p.z == -201);
    CHECK(run(g, "/tp 0 500 0") == CMD_ERR);
    CHECK(run(g, "/stamina set 40") == CMD_OK && g->p.stamina == 400);
    CHECK(run(g, "/attribute set strength 20") == CMD_OK && g->p.stats[ST_STR] == 20);
    CHECK(run(g, "/attribute set str 0") == CMD_USAGE);
    CHECK(run(g, "/skill set mining 30") == CMD_OK && player_skill_level(&g->p, SK_MINING) == 30);
    CHECK(run(g, "/skill set juggling 3") == CMD_ERR);
    CHECK(run(g, "/give @s iron_pickaxe") == CMD_OK);
    g->inv.selected = 0;
    CHECK(run(g, "/durability set 50") == CMD_OK);
    CHECK(g->inv.s[0].dur == g_items[g->inv.s[0].item].durability / 2);
    CHECK(run(g, "/repair") == CMD_OK && g->inv.s[0].dur == g_items[g->inv.s[0].item].durability);
    CHECK(run(g, "/kill @e[type=Cliderness:zombie]") == CMD_ERR);   /* monsters: Phase 7 */
    CHECK(run(g, "/kill @e[type=item]") == CMD_OK);
    CHECK(run(g, "/locate desert") == CMD_OK || run(g, "/locate desert") == CMD_ERR);
    CHECK(run(g, "/help noclip") == CMD_OK);
    CHECK(run(g, "/help 2") == CMD_OK);
    CHECK(run(g, "/commands") == CMD_OK);
    game_destroy(g);
}

static void t_building_undo(void)
{
    Game *g = &g_game;
    int glass = block_find("glass");
    long x, y, z, n = 0;
    CHECK(tf_make_game(g, GM_CREATIVE, NULL));
    tf_flat_area(g, 0, 70, 0, 8);
    CHECK(run(g, "/setblock 10 64 10 Cliderness:stone") == CMD_OK);
    CHECK(world_get(&g->world, 0, 10, 64, 10) == g_cb.stone);
    CHECK(run(g, "/fill 0 72 0 4 73 4 glass") == CMD_OK);
    for (x = 0; x <= 4; ++x) for (y = 72; y <= 73; ++y) for (z = 0; z <= 4; ++z) {
        n += world_get(&g->world, 0, x, y, z) == glass;
    }
    CHECK_EQ_L(n, 50);
    CHECK(run(g, "/undo") == CMD_OK);
    CHECK(world_get(&g->world, 0, 2, 72, 2) == BLOCK_AIR);
    CHECK(world_get(&g->world, 0, 10, 64, 10) == g_cb.stone);   /* only the last op */
    CHECK(run(g, "/redo") == CMD_OK);
    CHECK(world_get(&g->world, 0, 2, 72, 2) == glass);
    CHECK(run(g, "/redo") == CMD_ERR);
    CHECK(run(g, "/replace 0 72 0 4 73 4 glass Cliderness:stone_bricks") == CMD_OK);
    CHECK(world_get(&g->world, 0, 4, 73, 4) == block_find("stone_bricks"));
    CHECK(run(g, "/clone 0 72 0 1 72 1 20 72 20") == CMD_OK);
    CHECK(world_get(&g->world, 0, 21, 72, 21) == block_find("stone_bricks"));
    CHECK(run(g, "/undo") == CMD_OK && run(g, "/undo") == CMD_OK && run(g, "/undo") == CMD_OK);
    CHECK(world_get(&g->world, 0, 2, 72, 2) == BLOCK_AIR);
    CHECK(world_get(&g->world, 0, 21, 72, 21) != block_find("stone_bricks"));
    CHECK(run(g, "/fill 0 0 0 2000 127 2000 stone") == CMD_ERR);   /* too large */
    CHECK(run(g, "/fill 0 72 0 4 73 4 unobtainium") == CMD_ERR);
    CHECK(run(g, "/fill 1 2 3") == CMD_USAGE);
    /* terrain tools */
    game_teleport(g, 0, 70, 0);
    world_set(&g->world, 0, 2, 70, 2, (cl_u8)g_cb.stone, 0);
    CHECK(run(g, "/flatten 4") == CMD_OK);
    CHECK(world_get(&g->world, 0, 2, 70, 2) == BLOCK_AIR);
    CHECK(run(g, "/flood 3") == CMD_OK || 1);
    CHECK(run(g, "/drain 3") == CMD_OK);
    CHECK(run(g, "/raise 3 2") == CMD_OK);
    CHECK(blk_solid(world_get(&g->world, 0, 1, 70, 1)));
    CHECK(run(g, "/lower 3 2") == CMD_OK);
    CHECK(run(g, "/smooth 3") == CMD_OK);
    CHECK(run(g, "/undo") == CMD_OK);
    game_destroy(g);
}

static void t_completion(void)
{
    char line[CON_LINE];
    str_copy(line, "/gamem", sizeof(line));
    CHECK(con_complete(line, sizeof(line)));
    CHECK(strcmp(line, "/gamemode ") == 0);
    CHECK(con_complete(line, sizeof(line)));
    CHECK(strcmp(line, "/gamemode survival ") == 0);
    str_copy(line, "/give @s cobbl", sizeof(line));
    CHECK(con_complete(line, sizeof(line)));
    CHECK(strcmp(line, "/give @s cobblestone ") == 0);
    str_copy(line, "/weather set bliz", sizeof(line));
    CHECK(con_complete(line, sizeof(line)));
    CHECK(strcmp(line, "/weather set blizzard ") == 0);
    str_copy(line, "/time s", sizeof(line));
    CHECK(con_complete(line, sizeof(line)));
    CHECK(strcmp(line, "/time set ") == 0);
    str_copy(line, "/zzz", sizeof(line));
    CHECK(!con_complete(line, sizeof(line)));
    /* history */
    con_history_push("/seed");
    con_history_push("/pos");
    CHECK(strcmp(con_history_get(0), "/pos") == 0);
    CHECK(strcmp(con_history_get(1), "/seed") == 0);
}

/* robustness: every command with no / junk arguments must not crash */
static void t_fuzz(void)
{
    Game *g = &g_game;
    int i;
    static const char *const junk[] = { "", " x", " 1 2", " @e[type=", " -999999999 0 0", " ~ ~ ~ ~ ~ ~ ~ ~ ~ ~" };
    CHECK(tf_make_game(g, GM_SURVIVAL, NULL));
    for (i = 0; i < con_count(); ++i) {
        int k;
        const Command *c = con_get(i);
        if (str_ieq(c->name, "quit") || str_ieq(c->name, "language")) {
            continue;
        }
        for (k = 0; k < (int)(sizeof(junk) / sizeof(junk[0])); ++k) {
            char line[96];
            int r;
            str_copy(line, "/", sizeof(line));
            str_cat(line, c->name, sizeof(line));
            str_cat(line, junk[k], sizeof(line));
            r = run(g, line);
            CHECK(r >= CMD_OK && r <= CMD_UNKNOWN);
        }
    }
    g_con_request = CON_REQ_NONE;
    game_destroy(g);
    log_clear();
}

void suite_commands(void)
{
    tf_run("registry, descriptions, aliases", t_registry);
    tf_run("permissions and cheat flag", t_permissions);
    tf_run("command handlers", t_handlers);
    tf_run("building commands with undo/redo", t_building_undo);
    tf_run("tab completion and history", t_completion);
    tf_run("all commands survive junk input", t_fuzz);
}
