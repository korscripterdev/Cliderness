/* Cliderness - gameplay/game.c : session lifecycle, tick, player actions */
#include <string.h>
#include "gameplay/game.h"
#include "content/content.h"
#include "content/effects.h"
#include "world/biomes/biome.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/log.h"
#include "core/str.h"
#include "core/mathx.h"

Game g_game;

#define MSG_INFO ATTR(C_LIGHTGRAY, C_BLACK)
#define MSG_GOOD ATTR(C_LIGHTGREEN, C_BLACK)
#define MSG_WARN ATTR(C_YELLOW, C_BLACK)
#define MSG_BAD  ATTR(C_LIGHTRED, C_BLACK)

static const char *const g_mode_ids[GM_COUNT] = { "survival", "creative", "adventure", "spectator" };
static const char *const g_mode_keys[GM_COUNT] = {
    "mode.survival", "mode.creative", "mode.adventure", "mode.spectator"
};

int game_mode_find(const char *name)
{
    int i;
    long n;
    for (i = 0; i < GM_COUNT; ++i) {
        if (str_ieq(name, g_mode_ids[i]) || (name[0] && !name[1] && name[0] == g_mode_ids[i][0])) {
            return i;
        }
    }
    if (str_to_long(name, &n) && n >= 0 && n < GM_COUNT) {
        return (int)n;
    }
    return -1;
}

const char *game_mode_key(int mode)
{
    return g_mode_keys[(mode >= 0 && mode < GM_COUNT) ? mode : 0];
}

int game_can_edit_world(const Game *g)
{
    return g->mode == GM_SURVIVAL || g->mode == GM_CREATIVE;
}

static void creative_kit(Game *g)
{
    static const char *const kit[] = {
        "stone", "cobblestone", "dirt", "oak_planks", "glass", "stone_bricks", "sand", "oak_log", "torch"
    };
    int i;
    for (i = 0; i < (int)CL_COUNTOF(kit); ++i) {
        int it = item_find(kit[i]);
        if (it >= 0) {
            inv_add(&g->inv, it, g_items[it].max_stack, 0);
        }
    }
}

int game_create(Game *g, const char *name, const char *dir, cl_u32 seed, int mode, int cheats,
                const ClideTimeCfg *tcfg)
{
    memset(g, 0, sizeof(*g));
    str_copy(g->name, name, sizeof(g->name));
    if (dir) {
        str_copy(g->dir, dir, sizeof(g->dir));
    }
    g->seed = CL_U32(seed);
    g->mode = mode;
    g->cheats = cheats;
    if (!world_open(&g->world, g->seed, dir)) {
        return 0;
    }
    if (!edit_init(&g->journal)) {
        world_close(&g->world);
        return 0;
    }
    rng_seed(&g->rng, CL_U32(g->seed ^ 0xC11DE5UL));
    player_init(&g->p);
    inv_init(&g->inv);
    ctime_init(&g->time, tcfg, 8, 0);
    weather_init(&g->weather, g->seed);
    world_find_spawn(&g->world, &g->p.x, &g->p.y, &g->p.z);
    g->p.spawn_x = g->p.x;
    g->p.spawn_y = g->p.y;
    g->p.spawn_z = g->p.z;
    g->last_minute = ctime_total_minutes(&g->time);
    g->difficulty = DIFF_NORMAL;
    g->spawning = 1;
    g->aboard = -1;
    game_set_mode(g, mode);
    if (mode == GM_CREATIVE) {
        creative_kit(g);
    }
    g->active = 1;
    return 1;
}

void game_destroy(Game *g)
{
    if (!g->active) {
        return;
    }
    edit_free(&g->journal);
    world_close(&g->world);
    g->active = 0;
}

void game_set_mode(Game *g, int mode)
{
    g->mode = mode;
    g->p.noclip = 0;
    if (mode == GM_SPECTATOR) {
        g->p.noclip = 1;
        g->p.fly = 1;
    } else if (mode == GM_CREATIVE) {
        g->p.fly = 0;
    } else {
        g->p.fly = 0;
    }
    g->p.mining = 0;
}

static cl_u8 at(Game *g, long x, long y, long z)
{
    return world_get(&g->world, g->p.dim, x, y, z);
}

int game_is_covered(Game *g, long x, long y, long z)
{
    long k;
    for (k = 1; k <= 12; ++k) {
        cl_u8 b = at(g, x, y + k, z);
        if (blk_solid(b)) {
            return 1;
        }
    }
    return 0;
}

int game_light_radius(Game *g)
{
    int per = g->p.stats[ST_PER];
    int r;
    if (game_is_covered(g, g->p.x, g->p.y, g->p.z) && g->p.y < world_gen_height(&g->world, g->p.dim, g->p.x, g->p.z) - 1) {
        r = 4 + per / 4;
    } else {
        r = 4 + (g->p.visibility * 32) / 100 + per / 3;
    }
    return r;
}

static int move_cooldown(Game *g, int mode, cl_u8 target)
{
    int base, cd;
    long carry = inv_weight(&g->inv);
    long cap = game_carry_capacity(g);
    const WeatherDef *w = weather_cur(&g->weather);
    switch (mode) {
    case MOVE_SPRINT: base = 2; break;
    case MOVE_CROUCH: base = 7; break;
    case MOVE_SWIM: base = 6 - player_skill_level(&g->p, SK_SWIMMING) / 40; break;
    case MOVE_FLY: base = 2; break;
    default: base = 4; break;
    }
    cd = base * 100 / (90 + g->p.stats[ST_DEX]);
    if (cd < 1) {
        cd = 1;
    }
    if (g_blocks[target].flags & BF_SLOW) {
        cd += 3;
    }
    cd += fx_move(g);
    if (mode != MOVE_FLY) {
        if (carry > cap * 3 / 2) {
            cd += 6;
        } else if (carry > cap) {
            cd += 2;
        }
        cd = cd * (100 + w->move_penalty) / 100;
    }
    return cd;
}

void game_request_move(Game *g, int dir, int sprint)
{
    if (g->p.dead) {
        return;
    }
    if (g->aboard >= 0) {
        ship_steer(g, dir);
        return;
    }
    g->p.facing = dir & 3;
    g->p.pending_dir = dir & 3;
    g->p.pending_sprint = sprint;
}

void game_toggle_crouch(Game *g)
{
    g->p.crouch = !g->p.crouch;
    log_add(MSG_INFO, loc_get(g->p.crouch ? "msg.crouch_on" : "msg.crouch_off"));
}

static void apply_gravity(Game *g)
{
    Player *p = &g->p;
    int fall = 0;
    if (p->fly || p->noclip) {
        return;
    }
    /* inside a liquid: float up to the surface */
    while (p->y < WORLD_H - 1 && blk_liquid(at(g, p->x, p->y, p->z))) {
        ++p->y;
    }
    while (p->y > 0) {
        cl_u8 below = at(g, p->x, p->y - 1, p->z);
        if (blk_solid(below) || blk_liquid(below)) {
            break;
        }
        --p->y;
        ++fall;
    }
    if (fall >= 4 && !blk_liquid(at(g, p->x, p->y - 1, p->z))) {
        int dmg = (fall - 3) * 8;
        surv_damage(g, dmg, "cause.fall");
        if (!p->dead && !p->god && (g->mode == GM_SURVIVAL || g->mode == GM_ADVENTURE)) {
            if (fall >= 7) {
                fx_add(g, effect_find("fracture"), 0);
            }
            if (dmg >= 16) {
                fx_add(g, effect_find("bleeding"), 0);
            }
        }
    }
}

static void explore_xp(Game *g)
{
    long dx = g->p.x - g->p.spawn_x;
    long dz = g->p.z - g->p.spawn_z;
    long d = mx_isqrt(dx * dx + dz * dz);
    if (d > g->p.best_dist + 16) {
        g->p.best_dist = d;
        skill_gain(g, SK_EXPLORATION, 3);
    }
}

static void do_move(Game *g)
{
    Player *p = &g->p;
    long dx, dz, nx, nz;
    cl_u8 b;
    int mode;
    int sprint = p->pending_sprint;
    int swimming = blk_liquid(at(g, p->x, p->y - 1, p->z));
    dir_delta(p->pending_dir, &dx, &dz);
    p->pending_dir = -1;
    nx = p->x + dx;
    nz = p->z + dz;
    b = at(g, nx, p->y, nz);
    mode = p->fly ? MOVE_FLY : swimming ? MOVE_SWIM : p->crouch ? MOVE_CROUCH :
           (sprint && p->stamina >= 12) ? MOVE_SPRINT : MOVE_WALK;
    if (!p->noclip && blk_solid(b)) {
        cl_u8 above = at(g, nx, p->y + 1, nz);
        cl_u8 head = at(g, p->x, p->y + 1, p->z);
        if (!p->fly && !blk_solid(above) && !blk_solid(head) && !blk_liquid(above)) {
            /* step / climb up one block */
            int cost = 25;
            if (g->mode == GM_SURVIVAL || g->mode == GM_ADVENTURE) {
                if (p->stamina < cost && !p->god) {
                    log_add(MSG_WARN, loc_get("msg.too_tired"));
                    p->cooldown = 6;
                    return;
                }
                if (!p->god) {
                    p->stamina -= cost;
                }
            }
            p->x = nx;
            p->z = nz;
            ++p->y;
            p->cooldown = move_cooldown(g, mode, b) + 2;
            p->move_mode = mode;
            return;
        }
        if (g->blocked_msg == 0) {
            log_add(MSG_INFO, loc_fmt("msg.blocked", block_name(b), NULL, NULL));
            g->blocked_msg = 20;
        }
        p->cooldown = 2;
        return;
    }
    p->x = nx;
    p->z = nz;
    p->move_mode = mode;
    p->cooldown = move_cooldown(g, mode, b);
    if (g->mode == GM_SURVIVAL || g->mode == GM_ADVENTURE) {
        if (mode == MOVE_SPRINT && !p->god) {
            p->stamina -= 12 * 100 / (80 + p->stats[ST_END] * 2);
            ++p->sprinted;
        } else if (mode == MOVE_SWIM && !p->god) {
            p->stamina -= 4;
        }
        if (p->stamina < 0) {
            p->stamina = 0;
        }
    }
    p->noise = mode == MOVE_SPRINT ? 80 : mode == MOVE_CROUCH ? 10 : mode == MOVE_SWIM ? 40 :
               mode == MOVE_FLY ? 5 : 30;
    if (mode == MOVE_SWIM) {
        skill_gain(g, SK_SWIMMING, 1);
    }
    if (g_blocks[b].flags & BF_HURT && !p->god && g->mode != GM_CREATIVE && g->mode != GM_SPECTATOR) {
        surv_damage(g, 3, "cause.block");
    }
    p->mining = 0;
    apply_gravity(g);
    explore_xp(g);
}

void game_tick(Game *g)
{
    long m;
    int guard = 0;
    Player *p = &g->p;
    ++g->tick;
    ctime_advance(&g->time, TICK_MS);
    weather_tick(&g->weather);
    if (g->blocked_msg > 0) {
        --g->blocked_msg;
    }
    if (!p->dead) {
        if (p->cooldown > 0) {
            --p->cooldown;
        }
        if (p->attack_cd > 0) {
            --p->attack_cd;
        }
        if (p->pending_dir >= 0 && p->cooldown == 0) {
            do_move(g);
        } else if (g->aboard >= 0) {
            p->move_mode = MOVE_SAIL;
            p->pending_dir = -1;
        } else if (p->cooldown == 0) {
            p->move_mode = p->fly ? MOVE_FLY : blk_liquid(at(g, p->x, p->y - 1, p->z)) ? MOVE_SWIM :
                           p->crouch ? MOVE_CROUCH : MOVE_IDLE;
            if (p->noise > 0) {
                p->noise -= 2;
                if (p->noise < 0) {
                    p->noise = 0;
                }
            }
        }
        if (p->mining) {
            p->mine_progress += 1;
            if (p->mine_progress >= p->mine_total) {
                mine_finish(g);
            }
        }
        apply_gravity(g);
        surv_tick(g);
    }
    ent_tick(g);
    mob_tick(g);
    mob_spawn_tick(g);
    ship_tick(g);
    if ((g->tick % 5L) == 0) {
        fluid_step(&g->world);
    }
    m = ctime_total_minutes(&g->time);
    while (g->last_minute < m && guard < 240) {
        ++g->last_minute;
        ++guard;
        surv_minute(g);
        farm_minute(g);
        game_rain_minute(g);
        weather_minute(&g->weather, world_climate_temp(&g->world, p->dim, p->x, p->z),
                       g_biomes[world_biome(&g->world, p->dim, p->x, p->z)].humidity, p->dim);
    }
    g->last_minute = m;
}

static int facing_cell(Game *g, long *x, long *y, long *z)
{
    long dx, dz;
    dir_delta(g->p.facing, &dx, &dz);
    *x = g->p.x + dx;
    *y = g->p.y;
    *z = g->p.z + dz;
    return 1;
}

static void start_mining(Game *g, long x, long y, long z)
{
    cl_u8 b = at(g, x, y, z);
    Player *p = &g->p;
    if (!game_can_edit_world(g)) {
        log_add(MSG_WARN, loc_get("msg.cannot_edit"));
        return;
    }
    if (b == BLOCK_AIR || blk_liquid(b)) {
        log_add(MSG_INFO, loc_get("msg.nothing_to_mine"));
        return;
    }
    if (g_blocks[b].flags & BF_NOMINE) {
        log_add(MSG_WARN, loc_fmt("msg.unbreakable", block_name(b), NULL, NULL));
        return;
    }
    p->mx = x;
    p->my = y;
    p->mz = z;
    p->mine_progress = 0;
    p->mine_total = g->mode == GM_CREATIVE ? 1 : mine_time(g, b);
    p->mining = 1;
    if (p->mine_total > 6) {
        log_add(MSG_INFO, loc_fmt("msg.mining_start", block_name(b), NULL, NULL));
    }
}

void game_mine(Game *g)
{
    long x, y, z;
    cl_u8 b;
    if (g->p.dead) {
        return;
    }
    if (game_attack(g)) {
        return;
    }
    facing_cell(g, &x, &y, &z);
    if (g->aboard < 0) {
        int s = ship_at(g, g->p.dim, x, y, z);
        if (s >= 0 && ship_pickup(g, s)) {
            return;
        }
    }
    b = at(g, x, y, z);
    if (b == BLOCK_AIR || blk_liquid(b)) {
        cl_u8 floor = at(g, x, y - 1, z);
        if (floor != BLOCK_AIR && !blk_liquid(floor)) {
            --y;
        }
    }
    start_mining(g, x, y, z);
}

void game_dig_down(Game *g)
{
    if (g->p.dead) {
        return;
    }
    if (g->p.fly) {
        if (g->p.noclip || !blk_solid(at(g, g->p.x, g->p.y - 1, g->p.z))) {
            if (g->p.y > 1) {
                --g->p.y;
            }
        } else {
            start_mining(g, g->p.x, g->p.y - 1, g->p.z);
        }
        return;
    }
    start_mining(g, g->p.x, g->p.y - 1, g->p.z);
}

static int consume_selected(Game *g)
{
    if (g->mode == GM_CREATIVE) {
        return 1;
    }
    return inv_remove_slot(&g->inv, g->inv.selected, 1) == 1;
}

void game_place(Game *g)
{
    long x, y, z;
    ItemStack *st = inv_selected(&g->inv);
    cl_u8 b;
    int blk;
    if (g->p.dead) {
        return;
    }
    if (ship_open_cargo(g)) {
        return;
    }
    if (!game_can_edit_world(g)) {
        log_add(MSG_WARN, loc_get("msg.cannot_edit"));
        return;
    }
    if (ship_place(g)) {
        return;
    }
    if (st->item >= 0 && (st->item == item_find("water_bucket") || st->item == item_find("lava_bucket"))) {
        int fl = st->item == item_find("water_bucket") ? g_cb.water : g_cb.lava;
        facing_cell(g, &x, &y, &z);
        b = at(g, x, y, z);
        if (!(g_blocks[b].flags & BF_REPLACEABLE) || (blk_liquid(b) && b != (cl_u8)fl)) {
            log_add(MSG_INFO, loc_get("msg.no_room"));
            return;
        }
        {
            int have = b == (cl_u8)fl ? fluid_level(&g->world, g->p.dim, x, y, z) : 0;
            int put = CL_MIN(15 - have, st->dur > 0 ? st->dur : 15);
            fluid_put(&g->world, g->p.dim, x, y, z, fl, have + put);
            st->dur = (cl_s16)(st->dur - put);
            if (st->dur <= 0 || g->mode == GM_CREATIVE) {
                st->item = (cl_s16)item_find("bucket");
                st->dur = 0;
            }
        }
        log_add(MSG_GOOD, loc_get("msg.bucket_poured"));
        g->unsaved = 1;
        return;
    }
    if (st->item < 0 || g_items[st->item].place_block < 0) {
        log_add(MSG_INFO, loc_get("msg.not_placeable"));
        return;
    }
    blk = g_items[st->item].place_block;
    facing_cell(g, &x, &y, &z);
    b = at(g, x, y, z);
    if (!(g_blocks[b].flags & BF_REPLACEABLE) && !(g_blocks[b].flags & BF_PLANT)) {
        ++y;
        b = at(g, x, y, z);
        if (!(g_blocks[b].flags & BF_REPLACEABLE)) {
            log_add(MSG_INFO, loc_get("msg.no_room"));
            return;
        }
    }
    if (g_blocks[blk].place_on && at(g, x, y - 1, z) != g_blocks[blk].place_on) {
        log_add(MSG_INFO, loc_fmt("msg.need_soil", block_name(g_blocks[blk].place_on), NULL, NULL));
        return;
    }
    if (g_blocks[blk].func == FUNC_CHEST && chest_find(g, g->p.dim, x, y, z) < 0) {
        int c = chest_create(g, g->p.dim, x, y, z);
        if (c < 0) {
            log_add(MSG_WARN, loc_get("msg.no_chest_slot"));
            return;
        }
    }
    if (!consume_selected(g)) {
        return;
    }
    world_set(&g->world, g->p.dim, x, y, z, (cl_u8)blk, META_PLACED);
    if (g_blocks[blk].grow_to) {
        farm_register(g, g->p.dim, x, y, z);
        log_add(MSG_GOOD, loc_fmt("msg.planted", block_name(blk), NULL, NULL));
    }
    g->unsaved = 1;
    skill_gain(g, SK_BUILDING, 1);
}

void game_build_up(Game *g)
{
    ItemStack *st = inv_selected(&g->inv);
    Player *p = &g->p;
    if (p->dead) {
        return;
    }
    if (p->fly) {
        if (p->noclip || !blk_solid(at(g, p->x, p->y + 1, p->z))) {
            if (p->y < WORLD_H - 2) {
                ++p->y;
            }
        }
        return;
    }
    if (!game_can_edit_world(g)) {
        log_add(MSG_WARN, loc_get("msg.cannot_edit"));
        return;
    }
    if (st->item < 0 || g_items[st->item].place_block < 0 ||
        !blk_solid(g_items[st->item].place_block)) {
        log_add(MSG_INFO, loc_get("msg.need_block"));
        return;
    }
    if (blk_solid(at(g, p->x, p->y + 1, p->z)) || p->y >= WORLD_H - 2) {
        log_add(MSG_INFO, loc_get("msg.no_room"));
        return;
    }
    {
        int blk = g_items[st->item].place_block;
        if (!consume_selected(g)) {
            return;
        }
        ++p->y;
        world_set(&g->world, p->dim, p->x, p->y - 1, p->z, (cl_u8)blk, META_PLACED);
        g->unsaved = 1;
        skill_gain(g, SK_BUILDING, 1);
    }
}

void game_interact(Game *g)
{
    long x, y, z;
    int n;
    if (g->p.dead) {
        game_respawn(g);
        return;
    }
    if (g->aboard >= 0) {
        ship_disembark(g);
        return;
    }
    if (ship_board(g)) {
        return;
    }
    facing_cell(g, &x, &y, &z);
    {
        cl_u8 b = at(g, x, y, z);
        const BlockDef *bd = &g_blocks[b];
        if (bd->func == FUNC_CHEST) {
            int c = chest_find(g, g->p.dim, x, y, z);
            if (c < 0) {
                c = chest_create(g, g->p.dim, x, y, z);
                if (c >= 0 && bd->loot[0]) {
                    chest_fill_loot(g, c, bd->loot);
                }
            }
            if (c >= 0) {
                g->ui_request = UIREQ_CHEST;
                g->ui_arg = c;
            }
            return;
        }
        if (bd->func == FUNC_BED) {
            if (game_sleep(g, 0)) {
                g->p.spawn_x = g->p.x;
                g->p.spawn_y = g->p.y;
                g->p.spawn_z = g->p.z;
            }
            return;
        }
        if (bd->func == FUNC_STATION) {
            g->ui_request = UIREQ_CRAFT;
            g->ui_arg = bd->station;
            return;
        }
        if (ent_interact(g, x, y, z)) {
            return;
        }
    }
    n = ent_pickup(g, g->p.x, g->p.y, g->p.z);
    n += ent_pickup(g, x, y, z);
    if (n == 0) {
        log_add(MSG_INFO, loc_get("msg.nothing_here"));
    }
}

void game_drop(Game *g, int slot, int whole_stack)
{
    ItemStack *st;
    long x, y, z;
    int n;
    if (g->p.dead || slot < 0 || slot >= INV_SLOTS) {
        return;
    }
    st = &g->inv.s[slot];
    if (st->item < 0) {
        log_add(MSG_INFO, loc_get("msg.empty_hand"));
        return;
    }
    facing_cell(g, &x, &y, &z);
    if (blk_solid(at(g, x, y, z))) {
        x = g->p.x;
        z = g->p.z;
    }
    n = whole_stack ? st->count : 1;
    if (ent_spawn(g, g->p.dim, x, y, z, st->item, n, st->dur) < 0) {
        log_add(MSG_WARN, loc_get("msg.too_many_items"));
        return;
    }
    log_add(MSG_INFO, loc_fmt("msg.dropped", item_name(st->item), NULL, NULL));
    inv_remove_slot(&g->inv, slot, n);
    g->unsaved = 1;
}

void game_use(Game *g)
{
    ItemStack *st = inv_selected(&g->inv);
    Player *p = &g->p;
    if (p->dead) {
        return;
    }
    if (ship_use(g)) {
        return;
    }
    if (st->item >= 0 && g_items[st->item].tool_type == TOOL_HOE && farm_till(g)) {
        return;
    }
    if (st->item >= 0 && st->item == item_find("bone_meal")) {
        if (farm_grow_all(g, 2) > 0) {
            inv_remove_slot(&g->inv, g->inv.selected, 1);
            log_add(MSG_GOOD, loc_get("msg.fertilised"));
        } else {
            log_add(MSG_INFO, loc_get("msg.no_crops_near"));
        }
        return;
    }
    if (st->item >= 0 && st->item == item_find("bucket")) {
        long x, y, z, k;
        facing_cell(g, &x, &y, &z);
        for (k = 0; k < 3; ++k) {
            long tx = k == 2 ? p->x : x, ty = k == 0 ? y : y - 1, tz = k == 2 ? p->z : z;
            cl_u8 b = at(g, tx, ty, tz);
            if (b == (cl_u8)g_cb.water || b == (cl_u8)g_cb.lava) {
                int lvl = fluid_level(&g->world, p->dim, tx, ty, tz);
                int full = item_find(b == (cl_u8)g_cb.water ? "water_bucket" : "lava_bucket");
                if (full < 0 || !game_can_edit_world(g)) {
                    break;
                }
                world_set(&g->world, p->dim, tx, ty, tz, BLOCK_AIR, 0);
                st->item = (cl_s16)full;
                st->dur = (cl_s16)lvl;
                log_add(MSG_GOOD, loc_get("msg.bucket_filled"));
                g->unsaved = 1;
                return;
            }
        }
        log_add(MSG_INFO, loc_get("msg.no_fluid_here"));
        return;
    }
    if (st->item >= 0 && g_items[st->item].equip) {
        int worn = st->item;
        if (inv_equip(&g->inv, g->inv.selected)) {
            log_add(MSG_GOOD, loc_fmt("msg.equipped", item_name(worn), NULL, NULL));
        }
        return;
    }
    if (st->item >= 0 && (g_items[st->item].food > 0 || g_items[st->item].water > 0 ||
                          g_items[st->item].cure >= 0 || g_items[st->item].effect >= 0)) {
        const ItemDef *it = &g_items[st->item];
        int id = st->item;
        p->hunger = CL_MIN(1000, p->hunger + it->food);
        p->thirst = CL_MIN(1000, p->thirst + it->water);
        log_add(MSG_GOOD, loc_fmt("msg.ate", item_name(id), NULL, NULL));
        if (it->cure >= 0 && fx_remove(g, it->cure)) {
            log_add(MSG_GOOD, loc_fmt("msg.cured", effect_name(it->cure), NULL, NULL));
        }
        if (it->effect >= 0 && (it->effect_chance == 0 || rng_range(&g->rng, 100) < it->effect_chance)) {
            fx_add(g, it->effect, 0);
        }
        inv_remove_slot(&g->inv, g->inv.selected, 1);
        return;
    }
    {
        long x, y, z;
        facing_cell(g, &x, &y, &z);
        if (blk_liquid(at(g, x, y, z)) || blk_liquid(at(g, x, y - 1, z)) ||
            blk_liquid(at(g, p->x, p->y - 1, p->z))) {
            if (at(g, x, y - 1, z) == g_cb.lava || at(g, x, y, z) == g_cb.lava) {
                log_add(MSG_BAD, loc_get("msg.lava_no"));
                return;
            }
            p->thirst = CL_MIN(1000, p->thirst + 150);
            log_add(MSG_GOOD, loc_get("msg.drank"));
            return;
        }
    }
    log_add(MSG_INFO, loc_get("msg.cannot_use"));
}

void game_select(Game *g, int slot)
{
    if (slot >= 0 && slot < INV_HOTBAR) {
        g->inv.selected = slot;
        if (g->inv.s[slot].item >= 0) {
            log_add(MSG_INFO, item_name(g->inv.s[slot].item));
        }
    }
}

/*
 * Rain fills depressions: every rainy game minute a few random columns near
 * the player get one unit of water (three in heavy rain) if their top block
 * is solid and lower than all four neighbours.
 */
void game_rain_minute(Game *g)
{
    const WeatherDef *w = weather_cur(&g->weather);
    int tries, k;
    if (w->precip != PRECIP_RAIN || g->p.dim != DIM_OVERWORLD) {
        return;
    }
    tries = w->intensity >= 40 ? 6 : 2;
    for (k = 0; k < tries; ++k) {
        long x = g->p.x + rng_range(&g->rng, 49) - 24;
        long z = g->p.z + rng_range(&g->rng, 49) - 24;
        long top = world_top(&g->world, 0, x, z, WORLD_H - 2);
        cl_u8 b = world_get(&g->world, 0, x, top, z);
        int lvl;
        if (blk_liquid(b)) {
            if (b != (cl_u8)g_cb.water) {
                continue;
            }
            lvl = fluid_level(&g->world, 0, x, top, z);
            if (lvl < 15 && !fluid_is_source(&g->world, 0, x, top, z)) {
                fluid_put(&g->world, 0, x, top, z, g_cb.water, lvl + 1);
            }
            continue;
        }
        if (!blk_solid(b) || world_top(&g->world, 0, x + 1, z, WORLD_H - 2) <= top ||
            world_top(&g->world, 0, x - 1, z, WORLD_H - 2) <= top ||
            world_top(&g->world, 0, x, z + 1, WORLD_H - 2) <= top ||
            world_top(&g->world, 0, x, z - 1, WORLD_H - 2) <= top) {
            continue;
        }
        fluid_put(&g->world, 0, x, top + 1, z, g_cb.water, 1);
    }
}

void game_teleport(Game *g, long x, long y, long z)
{
    g->p.x = x;
    g->p.y = CL_CLAMP(y, 1L, (long)(WORLD_H - 2));
    g->p.z = z;
    g->p.mining = 0;
    g->p.pending_dir = -1;
    g->aboard = -1;
}

void game_respawn(Game *g)
{
    Player *p = &g->p;
    int stats[ST_COUNT];
    long xp[SK_COUNT];
    long sx = p->spawn_x, sy = p->spawn_y, sz = p->spawn_z;
    int god = p->god, fly = p->fly, noclip = p->noclip;
    memcpy(stats, p->stats, sizeof(stats));
    memcpy(xp, p->skill_xp, sizeof(xp));
    player_init(p);
    memcpy(p->stats, stats, sizeof(stats));
    memcpy(p->skill_xp, xp, sizeof(xp));
    p->spawn_x = sx;
    p->spawn_y = sy;
    p->spawn_z = sz;
    p->god = god;
    p->fly = fly;
    p->noclip = noclip;
    p->x = sx;
    p->z = sz;
    p->y = world_top(&g->world, 0, sx, sz, WORLD_H - 1) + 1;
    p->hp_max = 80 + p->stats[ST_CON] * 2;
    p->hp = p->hp_max;
    log_add(MSG_GOOD, loc_get("msg.respawned"));
}
