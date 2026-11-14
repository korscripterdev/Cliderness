/*
 * Cliderness - gameplay/entities/mob.c
 * Living creatures: spawning, AI, movement, combat, loot and trading.
 *
 * Combat formulas
 *   player melee  = weapon damage (fist 2, tools 3) x (100 + 3 STR) / 130
 *                   x (100 + 2 melee level) / 100, x2 on a critical hit
 *                   (chance = LCK %), minus the creature's defense (min 1)
 *   player bow    = 6 x (100 + 2 ranged level) / 100, range 12
 *   creature hit  = damage x difficulty (50 / 100 / 150 %)
 *                   x (100 - armour defense) / 100, min 1
 */
#include <stdio.h>
#include <string.h>
#include "gameplay/game.h"
#include "gameplay/entities/creature.h"
#include "world/biomes/biome.h"
#include "world/structures/structure.h"
#include "content/content.h"
#include "content/effects.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/log.h"
#include "core/mathx.h"
#include "core/str.h"

#define MOB_DESPAWN_DIST 72L

static cl_u8 at(Game *g, int dim, long x, long y, long z)
{
    return world_get(&g->world, dim, x, y, z);
}

static const char *const g_diff_keys[DIFF_COUNT] = {
    "diff.peaceful", "diff.easy", "diff.normal", "diff.hard"
};

const char *difficulty_key(int d)
{
    return g_diff_keys[(d >= 0 && d < DIFF_COUNT) ? d : DIFF_NORMAL];
}

int difficulty_find(const char *name)
{
    static const char *const names[DIFF_COUNT] = { "peaceful", "easy", "normal", "hard" };
    int i;
    long n;
    if (str_to_long(name, &n) && n >= 0 && n < DIFF_COUNT) {
        return (int)n;
    }
    for (i = 0; i < DIFF_COUNT; ++i) {
        if (str_ieq(name, names[i]) || str_ieq(name, loc_get(g_diff_keys[i]))) {
            return i;
        }
    }
    return -1;
}

int mob_count(const Game *g)
{
    int i, n = 0;
    for (i = 0; i < MAX_MOBS; ++i) {
        n += g->mobs[i].active;
    }
    return n;
}

int mob_at(Game *g, int dim, long x, long y, long z)
{
    int i;
    for (i = 0; i < MAX_MOBS; ++i) {
        Mob *m = &g->mobs[i];
        if (m->active && m->dim == dim && m->x == x && m->z == z && (m->y == y || m->y == y - 1)) {
            return i;
        }
    }
    return -1;
}

int mob_spawn(Game *g, int def, int dim, long x, long y, long z)
{
    int i;
    if (def < 0 || def >= g_creature_count) {
        return -1;
    }
    for (i = 0; i < MAX_MOBS; ++i) {
        Mob *m = &g->mobs[i];
        if (!m->active) {
            memset(m, 0, sizeof(*m));
            m->active = 1;
            m->def = (cl_s16)def;
            m->dim = (cl_u8)dim;
            m->x = m->home_x = x;
            m->y = y;
            m->z = m->home_z = z;
            m->hp = g_creatures[def].hp;
            m->state = MS_WANDER;
            m->think = (cl_s16)(i % 10);
            return i;
        }
    }
    return -1;
}

static int is_night(Game *g)
{
    return ctime_daylight(&g->time) < 25;
}

static int hostile_now(Game *g, const Mob *m)
{
    const CreatureDef *d = &g_creatures[m->def];
    if (g->difficulty == 0 && d->kind == MK_MONSTER) {
        return 0;
    }
    if (m->aggro) {
        return d->aggression != AGG_NEVER;
    }
    if (d->aggression == AGG_ALWAYS) {
        return 1;
    }
    if (d->aggression == AGG_NIGHT) {
        return is_night(g) || game_is_covered(g, m->x, m->y, m->z);
    }
    return 0;
}

int ent_hostile_near(Game *g, int radius)
{
    int i;
    for (i = 0; i < MAX_MOBS; ++i) {
        Mob *m = &g->mobs[i];
        long dx, dz;
        if (!m->active || m->dim != g->p.dim || m->state == MS_SLEEP) continue;
        if (!hostile_now(g, m)) continue;
        dx = m->x - g->p.x;
        dz = m->z - g->p.z;
        if (dx * dx + dz * dz <= (long)radius * radius && mx_abs(m->y - g->p.y) < 6) {
            return 1;
        }
    }
    return 0;
}

static void drop_loot(Game *g, Mob *m)
{
    const CreatureDef *d = &g_creatures[m->def];
    int l = d->loot[0] ? loot_find(d->loot) : -1, k;
    if (l < 0) {
        return;
    }
    for (k = 0; k < g_loot[l].n; ++k) {
        const LootDef *t = &g_loot[l];
        if (rng_range(&g->rng, 100) < t->chance[k]) {
            int n = t->lo[k] + (int)rng_range(&g->rng, t->hi[k] - t->lo[k] + 1);
            n += (rng_range(&g->rng, 100) < g->p.stats[ST_LCK] / 2) ? 1 : 0;
            if (n > 0) {
                ent_spawn(g, m->dim, m->x, m->y, m->z, t->item[k], n, 0);
            }
        }
    }
}

int mob_damage(Game *g, int idx, int dmg, int skill)
{
    Mob *m;
    const CreatureDef *d;
    char num[12];
    if (idx < 0 || idx >= MAX_MOBS || !g->mobs[idx].active) {
        return 0;
    }
    m = &g->mobs[idx];
    d = &g_creatures[m->def];
    dmg -= d->defense;
    if (dmg < 1) {
        dmg = 1;
    }
    m->hp = (cl_s16)(m->hp - dmg);
    m->aggro = 1;
    if (m->state == MS_SLEEP) {
        m->state = MS_WANDER;
    }
    sprintf(num, "%d", dmg);
    if (m->hp > 0) {
        if (skill >= 0) {
            skill_gain(g, skill, 1);
        }
        log_add(ATTR(C_YELLOW, C_BLACK), loc_fmt("msg.you_hit", creature_name(m->def), num, NULL));
        return 1;
    }
    log_add(ATTR(C_LIGHTGREEN, C_BLACK), loc_fmt("msg.you_killed", creature_name(m->def), NULL, NULL));
    drop_loot(g, m);
    if (skill >= 0) {
        skill_gain(g, skill, d->xp > 0 ? d->xp : 2);
    }
    ++g->kills;
    m->active = 0;
    return 2;
}

/* player attacks the creature in front of him; returns 1 if handled */
int game_attack(Game *g)
{
    long dx, dz;
    int idx, k;
    ItemStack *st = inv_selected(&g->inv);
    Player *p = &g->p;
    int bow = item_find("bow"), arrow = item_find("arrow");
    if (p->dead || g->mode == GM_SPECTATOR) {
        return 0;
    }
    dir_delta(p->facing, &dx, &dz);
    if (st->item >= 0 && st->item == bow && bow >= 0) {
        long x = p->x, z = p->z;
        if (p->attack_cd > 0) {
            return 1;
        }
        if (inv_count(&g->inv, arrow) < 1) {
            log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get("msg.no_arrows"));
            return 1;
        }
        inv_remove(&g->inv, arrow, 1);
        p->attack_cd = 12;
        for (k = 1; k <= 12; ++k) {
            x += dx;
            z += dz;
            idx = mob_at(g, p->dim, x, p->y, z);
            if (idx >= 0) {
                int dmg = 6 * (100 + player_skill_level(p, SK_RANGED) * 2) / 100;
                mob_damage(g, idx, dmg, SK_RANGED);
                if (rng_range(&g->rng, 2) == 0) {
                    ent_spawn(g, p->dim, x, p->y, z, arrow, 1, 0);
                }
                if (g_items[bow].durability > 0 && g->mode != GM_CREATIVE && --st->dur <= 0) {
                    inv_remove_slot(&g->inv, g->inv.selected, 1);
                }
                return 1;
            }
            if (blk_solid(at(g, p->dim, x, p->y, z))) {
                x -= dx;
                z -= dz;
                break;
            }
        }
        ent_spawn(g, p->dim, x, p->y, z, arrow, 1, 0);
        log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_get("msg.missed"));
        return 1;
    }
    idx = mob_at(g, p->dim, p->x + dx, p->y, p->z + dz);
    if (idx < 0) {
        return 0;
    }
    if (p->attack_cd > 0) {
        return 1;
    }
    {
        int base = 2, speed = 10, dmg;
        if (st->item >= 0) {
            const ItemDef *it = &g_items[st->item];
            if (it->damage > 0) {
                base = it->damage;
                speed = it->speed > 0 ? it->speed : 10;
            } else if (it->type == ITEM_TOOL) {
                base = 3;
            }
        }
        dmg = base * (100 + p->stats[ST_STR] * 3) / 130;
        dmg = dmg * (100 + player_skill_level(p, SK_MELEE) * 2) / 100;
        if (rng_range(&g->rng, 100) < p->stats[ST_LCK]) {
            dmg *= 2;
            stat_train(g, ST_LCK, 2);
            log_add(ATTR(C_LIGHTCYAN, C_BLACK), loc_get("msg.critical"));
        }
        if (dmg < 1) {
            dmg = 1;
        }
        p->attack_cd = speed;
        if (g->mode == GM_SURVIVAL && !p->god) {
            p->stamina = CL_MAX(0, p->stamina - 10);
        }
        if (st->item >= 0 && g_items[st->item].durability > 0 && g->mode != GM_CREATIVE &&
            (g_items[st->item].type == ITEM_WEAPON || g_items[st->item].type == ITEM_TOOL)) {
            if (--st->dur <= 0) {
                log_add(ATTR(C_YELLOW, C_BLACK), loc_fmt("msg.tool_broke", item_name(st->item), NULL, NULL));
                inv_remove_slot(&g->inv, g->inv.selected, 1);
            }
        }
        mob_damage(g, idx, dmg, SK_MELEE);
        p->noise = 60;
    }
    return 1;
}

static void hurt_player(Game *g, Mob *m)
{
    const CreatureDef *d = &g_creatures[m->def];
    static const int diff_pct[4] = { 0, 50, 100, 150 };
    int dmg = d->damage * diff_pct[g->difficulty & 3] / 100;
    int def = inv_defense(&g->inv);
    dmg = dmg * (100 - def) / 100;
    if (dmg < 1 && d->damage > 0) {
        dmg = 1;
    }
    if (dmg <= 0) {
        return;
    }
    if (def > 0 && g->mode != GM_CREATIVE) {
        /* armour wears */
        int k = (int)rng_range(&g->rng, INV_EQUIP - 1);
        ItemStack *e = &g->inv.equip[k];
        if (e->item >= 0 && g_items[e->item].durability > 0 && --e->dur <= 0) {
            log_add(ATTR(C_YELLOW, C_BLACK), loc_fmt("msg.tool_broke", item_name(e->item), NULL, NULL));
            e->item = -1;
            e->count = 0;
        }
    }
    log_add(ATTR(C_LIGHTRED, C_BLACK), loc_fmt("msg.mob_hits", creature_name(m->def), NULL, NULL));
    surv_damage(g, dmg, "cause.creature");
    if (d->effect >= 0 && rng_range(&g->rng, 100) < d->effect_chance) {
        fx_add(g, d->effect, 0);
    }
}

/* can a creature stand in (x, y, z)? */
static int walkable(Game *g, const CreatureDef *d, int dim, long x, long y, long z)
{
    cl_u8 b = at(g, dim, x, y, z);
    cl_u8 below = at(g, dim, x, y - 1, z);
    if (d->aquatic) {
        return b == (cl_u8)g_cb.water;
    }
    if (blk_solid(b) || b == (cl_u8)g_cb.lava || (g_blocks[b].flags & BF_HURT)) {
        return 0;
    }
    /* ground directly below (falls are found by the step search) */
    return blk_solid(below) || blk_liquid(below);
}

static int occupied(Game *g, int self, int dim, long x, long y, long z)
{
    int i;
    if (g->p.dim == dim && g->p.x == x && g->p.z == z && mx_abs(g->p.y - y) <= 1) {
        return 1;
    }
    for (i = 0; i < MAX_MOBS; ++i) {
        if (i != self && g->mobs[i].active && g->mobs[i].dim == dim && g->mobs[i].x == x &&
            g->mobs[i].z == z && g->mobs[i].y == y) {
            return 1;
        }
    }
    return 0;
}

/* one step toward (tx, tz) (away if flee); returns 1 if moved */
static int step_toward(Game *g, int idx, long tx, long tz, int flee)
{
    Mob *m = &g->mobs[idx];
    const CreatureDef *d = &g_creatures[m->def];
    long dx = tx - m->x, dz = tz - m->z;
    long cand[4][2];
    int n = 0, k;
    if (flee) {
        dx = -dx;
        dz = -dz;
    }
    if (mx_abs(dx) >= mx_abs(dz)) {
        cand[n][0] = dx > 0 ? 1 : -1; cand[n][1] = 0; ++n;
        if (dz) { cand[n][0] = 0; cand[n][1] = dz > 0 ? 1 : -1; ++n; }
    } else {
        cand[n][0] = 0; cand[n][1] = dz > 0 ? 1 : -1; ++n;
        if (dx) { cand[n][0] = dx > 0 ? 1 : -1; cand[n][1] = 0; ++n; }
    }
    if (dx == 0 && dz == 0) {
        return 0;
    }
    cand[n][0] = cand[0][1]; cand[n][1] = cand[0][0]; ++n;
    cand[n][0] = -cand[0][1]; cand[n][1] = -cand[0][0]; ++n;
    for (k = 0; k < n; ++k) {
        long nx = m->x + cand[k][0], nz = m->z + cand[k][1];
        long ny;
        for (ny = m->y + 1; ny >= m->y - 3; --ny) {
            if (walkable(g, d, m->dim, nx, ny, nz) && !occupied(g, idx, m->dim, nx, ny, nz)) {
                /* never jump more than one block up, falls up to 3 */
                if (ny == m->y + 1 && blk_solid(at(g, m->dim, m->x, m->y + 1, m->z))) {
                    continue;
                }
                m->x = nx;
                m->y = ny;
                m->z = nz;
                return 1;
            }
        }
    }
    return 0;
}

/* the cell a creature reaches by stepping in direction dir, or 0 */
static int neighbour(Game *g, int idx, long x, long y, long z, int dir, long *ox, long *oy, long *oz)
{
    const Mob *m = &g->mobs[idx];
    const CreatureDef *d = &g_creatures[m->def];
    long dx, dz, ny;
    dir_delta(dir, &dx, &dz);
    for (ny = y + 1; ny >= y - 3; --ny) {
        if (walkable(g, d, m->dim, x + dx, ny, z + dz)) {
            if (ny == y + 1 && blk_solid(at(g, m->dim, x, y + 1, z))) {
                continue;   /* no head room to climb */
            }
            *ox = x + dx;
            *oy = ny;
            *oz = z + dz;
            return 1;
        }
    }
    return 0;
}

/*
 * A* over the cells a creature can walk (4 directions, one block up, falls of
 * up to three) inside a 24-cell box around the creature. Open cells live in a
 * binary heap, visited cells in a small hash table, so even PATH_NODES cells
 * cost only a few thousand block lookups. At most two searches run per tick
 * (g_path_budget), and only when the straight step is blocked.
 */
#ifdef CLIDE_TARGET_DOS16
#define PATH_NODES 160
#define PATH_HASH  256
#else
#define PATH_NODES 1200
#define PATH_HASH  4096
#endif

typedef struct PNode {
    long  x, y, z;
    short g, f, parent;
    cl_u8 dir, closed;
} PNode;

static PNode g_pn[PATH_NODES];
static short g_heap[PATH_NODES];
static short g_hash[PATH_HASH];   /* node index + 1, 0 = empty */
static int g_nheap;
static int g_path_budget = 0;
static long g_path_tick = -1;

static unsigned phash(long x, long y, long z)
{
    return (unsigned)(CL_U32((cl_u32)x * 73856093UL ^ (cl_u32)y * 19349663UL ^ (cl_u32)z * 83492791UL) % PATH_HASH);
}

static int pfind(long x, long y, long z)
{
    unsigned h = phash(x, y, z);
    while (g_hash[h]) {
        const PNode *n = &g_pn[g_hash[h] - 1];
        if (n->x == x && n->y == y && n->z == z) return g_hash[h] - 1;
        h = (h + 1) % PATH_HASH;
    }
    return -1;
}

static void pinsert(int idx)
{
    unsigned h = phash(g_pn[idx].x, g_pn[idx].y, g_pn[idx].z);
    while (g_hash[h]) h = (h + 1) % PATH_HASH;
    g_hash[h] = (short)(idx + 1);
}

static void heap_push(int idx)
{
    int i = g_nheap++;
    while (i > 0) {
        int par = (i - 1) / 2;
        if (g_pn[g_heap[par]].f <= g_pn[idx].f) break;
        g_heap[i] = g_heap[par];
        i = par;
    }
    g_heap[i] = (short)idx;
}

static int heap_pop(void)
{
    int top = g_heap[0], last, i = 0;
    last = g_heap[--g_nheap];
    while (g_nheap > 0) {
        int c = 2 * i + 1;
        if (c >= g_nheap) break;
        if (c + 1 < g_nheap && g_pn[g_heap[c + 1]].f < g_pn[g_heap[c]].f) ++c;
        if (g_pn[last].f <= g_pn[g_heap[c]].f) break;
        g_heap[i] = g_heap[c];
        i = c;
    }
    if (g_nheap > 0) g_heap[i] = (short)last;
    return top;
}

int mob_find_path(Game *g, int idx, long tx, long ty, long tz)
{
    Mob *m = &g->mobs[idx];
    int n = 1, k, goal = -1, expanded = 0;
    memset(g_hash, 0, sizeof(g_hash));
    g_nheap = 0;
    g_pn[0].x = m->x;
    g_pn[0].y = m->y;
    g_pn[0].z = m->z;
    g_pn[0].g = 0;
    g_pn[0].f = (short)(mx_abs(tx - m->x) + mx_abs(tz - m->z));
    g_pn[0].parent = -1;
    g_pn[0].closed = 0;
    pinsert(0);
    heap_push(0);
    m->npath = 0;
    m->pathpos = 0;
    while (g_nheap > 0 && expanded < PATH_NODES) {
        int best = heap_pop();
        PNode *c = &g_pn[best];
        if (c->closed) continue;   /* stale heap entry */
        c->closed = 1;
        ++expanded;
        if (mx_abs(c->x - tx) + mx_abs(c->z - tz) <= 1 && mx_abs(c->y - ty) <= 1) {
            goal = best;
            break;
        }
        for (k = 0; k < 4; ++k) {
            long nx, ny, nz;
            int j, cost;
            if (!neighbour(g, idx, c->x, c->y, c->z, k, &nx, &ny, &nz)) continue;
            if (mx_abs(nx - m->x) > 24 || mx_abs(nz - m->z) > 24) continue;
            cost = c->g + 1 + (ny > c->y ? 1 : 0);
            j = pfind(nx, ny, nz);
            if (j >= 0) {
                if (!g_pn[j].closed && cost < g_pn[j].g && g_nheap < PATH_NODES) {
                    g_pn[j].f = (short)(g_pn[j].f - g_pn[j].g + cost);
                    g_pn[j].g = (short)cost;
                    g_pn[j].parent = (short)best;
                    g_pn[j].dir = (cl_u8)k;
                    heap_push(j);
                }
                continue;
            }
            if (n >= PATH_NODES || g_nheap >= PATH_NODES) continue;
            g_pn[n].x = nx;
            g_pn[n].y = ny;
            g_pn[n].z = nz;
            g_pn[n].g = (short)cost;
            g_pn[n].f = (short)(cost + mx_abs(tx - nx) + mx_abs(tz - nz) + mx_abs(ty - ny));
            g_pn[n].parent = (short)best;
            g_pn[n].dir = (cl_u8)k;
            g_pn[n].closed = 0;
            pinsert(n);
            heap_push(n);
            ++n;
        }
    }
    if (goal < 0) {
        return 0;
    }
    /* walk back from the goal; keep the first MOB_PATH steps */
    {
        int len = 0, j;
        for (j = goal; g_pn[j].parent >= 0; j = g_pn[j].parent) {
            ++len;
        }
        for (j = goal, k = len; g_pn[j].parent >= 0; j = g_pn[j].parent) {
            --k;
            if (k < MOB_PATH) m->path[k] = g_pn[j].dir;
        }
        m->npath = (cl_u8)(len < MOB_PATH ? len : MOB_PATH);
        return len;
    }
}

/* follow the stored route; returns 1 if a step was taken */
static int follow_path(Game *g, int idx)
{
    Mob *m = &g->mobs[idx];
    long nx, ny, nz;
    if (m->pathpos >= m->npath) {
        return 0;
    }
    if (!neighbour(g, idx, m->x, m->y, m->z, m->path[m->pathpos], &nx, &ny, &nz) ||
        occupied(g, idx, m->dim, nx, ny, nz)) {
        m->npath = 0;
        return 0;
    }
    m->x = nx;
    m->y = ny;
    m->z = nz;
    ++m->pathpos;
    return 1;
}

/* move toward a target: by the A* route when there is one, else greedily */
static void go_to(Game *g, int idx, long tx, long ty, long tz)
{
    if (!follow_path(g, idx)) {
        step_toward(g, idx, tx, tz, 0);
    }
    (void)ty;
}

/* plan a route when the straight way is blocked (budgeted) */
static void plan(Game *g, int idx, long tx, long ty, long tz)
{
    Mob *m = &g->mobs[idx];
    long dx = tx - m->x, dz = tz - m->z, nx, ny, nz;
    int dir;
    if (g_path_tick != g->tick) {
        g_path_tick = g->tick;
        g_path_budget = 2;
    }
    if (mx_abs(dx) + mx_abs(dz) <= 1 || mx_abs(dx) > 20 || mx_abs(dz) > 20) {
        m->npath = 0;
        return;
    }
    /* keep a route while it lasts and the target stayed close to its end */
    if (m->pathpos < m->npath && mx_abs(tx - m->path_tx) + mx_abs(tz - m->path_tz) <= 2) {
        return;
    }
    dir = mx_abs(dx) >= mx_abs(dz) ? (dx > 0 ? DIR_E : DIR_W) : (dz > 0 ? DIR_S : DIR_N);
    if (m->npath == 0 && neighbour(g, idx, m->x, m->y, m->z, dir, &nx, &ny, &nz) &&
        (mx_abs(dx) <= 2 || mx_abs(dz) <= 2)) {
        return;   /* open ground straight ahead: walk greedily */
    }
    if (g_path_budget > 0) {
        --g_path_budget;
        mob_find_path(g, idx, tx, ty, tz);
        m->path_tx = tx;
        m->path_tz = tz;
    }
}

static void think(Game *g, int idx)
{
    Mob *m = &g->mobs[idx];
    const CreatureDef *d = &g_creatures[m->def];
    Player *p = &g->p;
    long dx = p->x - m->x, dz = p->z - m->z, dist = mx_isqrt(dx * dx + dz * dz);
    int vis = weather_cur(&g->weather)->visibility;
    int light = ctime_daylight(&g->time);
    int sight = d->sight * vis / 100, hear;
    int sleepy;
    int perceives;
    if (d->activity == ACTV_DAY) sleepy = light < 20;
    else if (d->activity == ACTV_NIGHT) sleepy = light > 60 && !game_is_covered(g, m->x, m->y, m->z);
    else sleepy = 0;
    if (d->activity != ACTV_NIGHT && light < 20 && d->kind != MK_MONSTER) {
        sight /= 2;
    }
    if (p->crouch) {
        sight = sight * 2 / 3 * (100 - player_skill_level(p, SK_STEALTH) / 2) / 100;
    }
    hear = d->hearing * p->noise / 100;
    if (m->state == MS_SLEEP) {
        sight /= 2;
        hear /= 2;
    }
    perceives = !p->dead && p->dim == m->dim && mx_abs(p->y - m->y) < 5 &&
                (dist <= sight || dist <= hear) && g->mode != GM_SPECTATOR;
    /* goal 1: survive */
    if (d->flee_pct > 0 && m->hp * 100 < g_creatures[m->def].hp * d->flee_pct && perceives) {
        m->state = MS_FLEE;
        return;
    }
    /* goal 2: attack */
    if (perceives && hostile_now(g, m) && !p->god && g->mode != GM_CREATIVE) {
        m->state = dist <= 1 ? MS_ATTACK : MS_CHASE;
        if (m->state == MS_CHASE) {
            plan(g, idx, p->x, p->y, p->z);
        }
        return;
    }
    /* timid animals run from loud or close players */
    if (perceives && d->kind == MK_ANIMAL && d->aggression == AGG_NEVER && (dist < 4 || (p->noise >= 60 && dist < 8))) {
        m->state = MS_FLEE;
        return;
    }
    /* goal 3: shelter (burning in daylight) */
    if (d->burns && light > 60 && !game_is_covered(g, m->x, m->y, m->z)) {
        m->state = MS_SHELTER;
        return;
    }
    /* goal 4: NPCs go home at night */
    if (d->kind == MK_NPC && light < 25) {
        long hx = m->home_x - m->x, hz = m->home_z - m->z;
        m->state = (hx * hx + hz * hz > 4) ? MS_HOME : MS_SLEEP;
        if (m->state == MS_HOME && m->npath == 0) {
            plan(g, idx, m->home_x, m->y, m->home_z);
        }
        return;
    }
    /* goal 5: rest */
    if (sleepy && !m->aggro) {
        m->state = MS_SLEEP;
        return;
    }
    /* goal 6: wander (less in rain) */
    {
        int chance = weather_cur(&g->weather)->precip ? 25 : 60;
        m->state = rng_range(&g->rng, 100) < chance ? MS_WANDER : MS_IDLE;
    }
}

static void act(Game *g, int idx)
{
    Mob *m = &g->mobs[idx];
    const CreatureDef *d = &g_creatures[m->def];
    Player *p = &g->p;
    switch (m->state) {
    case MS_CHASE:
        go_to(g, idx, p->x, p->y, p->z);
        break;
    case MS_ATTACK: {
        long dx = mx_abs(p->x - m->x), dz = mx_abs(p->z - m->z);
        if (dx + dz <= 1 && mx_abs(p->y - m->y) <= 1) {
            if (m->cd_attack == 0) {
                hurt_player(g, m);
                m->cd_attack = d->attack_cd;
            }
        } else {
            go_to(g, idx, p->x, p->y, p->z);
        }
        break;
    }
    case MS_FLEE:
        step_toward(g, idx, p->x, p->z, 1);
        break;
    case MS_HOME:
        go_to(g, idx, m->home_x, m->y, m->home_z);
        break;
    case MS_SHELTER:
    case MS_WANDER: {
        long tx = m->x + rng_range(&g->rng, 3) - 1, tz = m->z + rng_range(&g->rng, 3) - 1;
        if (d->kind == MK_NPC) {
            /* stay near home */
            if (mx_abs(m->x - m->home_x) > 12) tx = m->home_x;
            if (mx_abs(m->z - m->home_z) > 12) tz = m->home_z;
        }
        step_toward(g, idx, tx, tz, 0);
        break;
    }
    default:
        break;
    }
}

void mob_tick(Game *g)
{
    int i;
    for (i = 0; i < MAX_MOBS; ++i) {
        Mob *m = &g->mobs[i];
        const CreatureDef *d;
        long dx, dz;
        if (!m->active) continue;
        d = &g_creatures[m->def];
        dx = m->x - g->p.x;
        dz = m->z - g->p.z;
        if (m->dim != g->p.dim || dx * dx + dz * dz > MOB_DESPAWN_DIST * MOB_DESPAWN_DIST) {
            m->active = 0;
            continue;
        }
        if (m->cd_attack > 0) --m->cd_attack;
        if (g->ai_off) {
            continue;
        }
        if (--m->think <= 0) {
            m->think = 10;
            think(g, i);
        }
        if (++m->cd_move >= (m->state == MS_FLEE || m->state == MS_CHASE ? CL_MAX(1, d->speed * 2 / 3) : d->speed)) {
            m->cd_move = 0;
            act(g, i);
        }
        /* daylight burns, lava and suffocation hurt */
        if ((g->tick % 20L) == 0) {
            if (d->burns && ctime_daylight(&g->time) > 60 && !game_is_covered(g, m->x, m->y, m->z)) {
                m->hp = (cl_s16)(m->hp - 2);
            }
            if (!d->aquatic && blk_solid(at(g, m->dim, m->x, m->y, m->z))) {
                m->hp = (cl_s16)(m->hp - 2);
            }
            if (d->aquatic && !blk_liquid(at(g, m->dim, m->x, m->y, m->z))) {
                m->hp = (cl_s16)(m->hp - 2);
            }
            if (m->hp <= 0) {
                m->active = 0;
            }
        }
    }
}

static int biome_ok(const CreatureDef *d, int biome)
{
    char buf[96];
    char *tok;
    if (!d->biomes[0]) {
        return 1;
    }
    str_copy(buf, d->biomes, sizeof(buf));
    for (tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
        if (str_ieq(str_trim(tok), g_biomes[biome].id)) {
            return 1;
        }
    }
    return 0;
}

static int spawn_cap(const Game *g)
{
#ifdef CLIDE_TARGET_DOS16
    return 8 + g->difficulty * 1;
#else
    return 16 + g->difficulty * 6;
#endif
}

/* natural spawning every 40 ticks around the player */
void mob_spawn_tick(Game *g)
{
    Player *p = &g->p;
    int tries, light = ctime_daylight(&g->time);
    int covered = game_is_covered(g, p->x, p->y, p->z);
    if ((g->tick % 40L) != 0 || !g->spawning || g->mode == GM_SPECTATOR || mob_count(g) >= spawn_cap(g)) {
        return;
    }
    for (tries = 0; tries < 4; ++tries) {
        long total = 0, roll;
        int k, def = -1, habitat;
        long ang = rng_range(&g->rng, 256), dist = 16 + rng_range(&g->rng, 14);
        long x = p->x + (long)mx_cos256((int)ang) * dist / 1024L;
        long z = p->z + (long)mx_sin256((int)ang) * dist / 1024L;
        long y, top = world_top(&g->world, p->dim, x, z, WORLD_H - 2);
        int biome = world_biome(&g->world, p->dim, x, z);
        cl_u8 tb = at(g, p->dim, x, top, z);
        if (covered && p->y < world_gen_height(&g->world, p->dim, p->x, p->z) - 4) {
            habitat = SPAWN_CAVE;
            y = p->y + rng_range(&g->rng, 5) - 2;
        } else if (tb == (cl_u8)g_cb.water) {
            habitat = SPAWN_WATER;
            y = top;
        } else {
            habitat = SPAWN_SURFACE;
            y = top + 1;
        }
        for (k = 0; k < g_creature_count; ++k) {
            const CreatureDef *d = &g_creatures[k];
            if (d->habitat != habitat || d->dim != p->dim || !biome_ok(d, biome) || d->weight <= 0) continue;
            if (d->kind == MK_MONSTER && g->difficulty == 0) continue;
            if (habitat == SPAWN_SURFACE) {
                if (d->activity == ACTV_DAY && light < 40) continue;
                if (d->activity == ACTV_NIGHT && light > 25) continue;
            }
            total += d->weight;
        }
        if (total <= 0) continue;
        roll = rng_range(&g->rng, total);
        for (k = 0; k < g_creature_count; ++k) {
            const CreatureDef *d = &g_creatures[k];
            if (d->habitat != habitat || d->dim != p->dim || !biome_ok(d, biome) || d->weight <= 0) continue;
            if (d->kind == MK_MONSTER && g->difficulty == 0) continue;
            if (habitat == SPAWN_SURFACE) {
                if (d->activity == ACTV_DAY && light < 40) continue;
                if (d->activity == ACTV_NIGHT && light > 25) continue;
            }
            roll -= d->weight;
            if (roll < 0) {
                def = k;
                break;
            }
        }
        if (def < 0 || !walkable(g, &g_creatures[def], p->dim, x, y, z)) continue;
        {
            const CreatureDef *d = &g_creatures[def];
            int n = d->group_min + (int)rng_range(&g->rng, d->group_max - d->group_min + 1), j;
            for (j = 0; j < n && mob_count(g) < spawn_cap(g); ++j) {
                long gx = x + (j % 2), gz = z + (j / 2);
                if (walkable(g, d, p->dim, gx, y, gz)) {
                    mob_spawn(g, def, p->dim, gx, y, gz);
                }
            }
        }
        return;
    }
}

int mob_kill_type(Game *g, int def)
{
    int i, n = 0;
    for (i = 0; i < MAX_MOBS; ++i) {
        if (g->mobs[i].active && (def < 0 || g->mobs[i].def == def)) {
            g->mobs[i].active = 0;
            ++n;
        }
    }
    return n;
}

/* trade with an NPC: performs the first affordable offer, or lists them */
int ent_interact(Game *g, long x, long y, long z)
{
    int idx = mob_at(g, g->p.dim, x, y, z), k;
    const CreatureDef *d;
    char a[12], b[12];
    if (idx < 0) {
        return 0;
    }
    d = &g_creatures[g->mobs[idx].def];
    if (d->kind != MK_NPC || d->ntrades == 0) {
        log_add(ATTR(C_LIGHTGRAY, C_BLACK), loc_fmt("msg.mob_look", creature_name(g->mobs[idx].def),
                loc_get(mob_state_key(g->mobs[idx].state)), NULL));
        return 1;
    }
    for (k = 0; k < d->ntrades; ++k) {
        if (inv_count(&g->inv, d->trade_in[k]) >= d->trade_in_n[k]) {
            inv_remove(&g->inv, d->trade_in[k], d->trade_in_n[k]);
            if (inv_add(&g->inv, d->trade_out[k], d->trade_out_n[k], 0) > 0) {
                ent_spawn(g, g->p.dim, g->p.x, g->p.y, g->p.z, d->trade_out[k], d->trade_out_n[k], 0);
            }
            sprintf(b, "%d", d->trade_out_n[k]);
            log_add(ATTR(C_LIGHTGREEN, C_BLACK), loc_fmt("msg.traded", item_name(d->trade_out[k]), b, NULL));
            return 1;
        }
    }
    log_add(ATTR(C_YELLOW, C_BLACK), loc_fmt("msg.trade_offers", creature_name(g->mobs[idx].def), NULL, NULL));
    for (k = 0; k < d->ntrades; ++k) {
        char line[96], n1[32], n2[32];
        sprintf(a, "%d", d->trade_in_n[k]);
        sprintf(b, "%d", d->trade_out_n[k]);
        str_copy(n1, item_name(d->trade_in[k]), sizeof(n1));
        str_copy(n2, item_name(d->trade_out[k]), sizeof(n2));
        sprintf(line, "  %s x%s -> %s x%s", n1, a, n2, b);
        log_add(ATTR(C_LIGHTGRAY, C_BLACK), line);
    }
    return 1;
}
