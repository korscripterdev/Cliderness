/*
 * Cliderness - gameplay/building/edit.c
 * Undo/redo journal for world edits and the mining formula.
 *
 * Mining time (ticks, 20 per second):
 *   t = hardness
 *   right tool:   t = hardness * 100 / tool_power
 *   needs a tool but bare hands / wrong tool: t = hardness * 3
 *   t = t * 100 / (100 + STR * 2)           strength bonus
 *   t = t * 100 / (100 + skill_level * 3)   mining / woodcutting skill bonus
 *   minimum 1 tick
 * Experience is only granted for blocks that were NOT placed by a player
 * (META_PLACED), which prevents place-and-mine farming.
 */
#include <string.h>
#include "gameplay/game.h"
#include "content/content.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/log.h"
#include "core/mem.h"
#include "core/rng.h"

int edit_init(EditJournal *j)
{
    memset(j, 0, sizeof(*j));
    j->cap = EDIT_MAX_RECORDS;
    j->r = (EditRec *)mem_alloc(sizeof(EditRec) * (size_t)j->cap);
    return j->r != NULL;
}

void edit_free(EditJournal *j)
{
    mem_free(j->r);
    j->r = NULL;
}

void edit_begin(EditJournal *j)
{
    /* a new operation discards the redo history */
    j->nops = j->pos;
    j->count = j->op_start[j->pos];
    j->cur_start = j->count;
    j->recording = 1;
    j->overflow = 0;
}

static void drop_oldest(EditJournal *j)
{
    long shift = j->op_start[1];
    int i;
    memmove(j->r, j->r + shift, sizeof(EditRec) * (size_t)(j->count - shift));
    for (i = 0; i < j->nops; ++i) {
        j->op_start[i] = j->op_start[i + 1] - shift;
    }
    j->count -= shift;
    j->cur_start -= shift;
    --j->nops;
    --j->pos;
}

void edit_set(Game *g, int dim, long x, long y, long z, cl_u8 b, cl_u8 m)
{
    EditJournal *j = &g->journal;
    cl_u8 ob, om;
    if (y < 0 || y >= WORLD_H) {
        return;
    }
    ob = world_get(&g->world, dim, x, y, z);
    om = world_get_meta(&g->world, dim, x, y, z);
    if (ob == b && om == m) {
        return;
    }
    if (j->recording && !j->overflow) {
        if (j->count >= j->cap) {
            j->overflow = 1;
        } else {
            EditRec *r = &j->r[j->count++];
            r->x = x;
            r->z = z;
            r->y = (cl_s16)y;
            r->dim = (cl_u8)dim;
            r->oldb = ob;
            r->oldm = om;
            r->newb = b;
            r->newm = m;
        }
    }
    world_set(&g->world, dim, x, y, z, b, m);
    g->unsaved = 1;
}

int edit_end(EditJournal *j)
{
    j->recording = 0;
    if (j->overflow || j->count == j->cur_start) {
        j->count = j->cur_start;
        return 0;
    }
    if (j->nops >= EDIT_MAX_OPS) {
        drop_oldest(j);
    }
    j->op_start[j->nops] = j->cur_start;
    ++j->nops;
    j->op_start[j->nops] = j->count;
    j->pos = j->nops;
    return 1;
}

int edit_undo(Game *g)
{
    EditJournal *j = &g->journal;
    long i, a, b;
    if (j->pos <= 0) {
        return -1;
    }
    --j->pos;
    a = j->op_start[j->pos];
    b = j->op_start[j->pos + 1];
    for (i = b - 1; i >= a; --i) {
        EditRec *r = &j->r[i];
        world_set(&g->world, r->dim, r->x, r->y, r->z, r->oldb, r->oldm);
    }
    g->unsaved = 1;
    return (int)(b - a > 32000L ? 32000L : b - a);
}

int edit_redo(Game *g)
{
    EditJournal *j = &g->journal;
    long i, a, b;
    if (j->pos >= j->nops) {
        return -1;
    }
    a = j->op_start[j->pos];
    b = j->op_start[j->pos + 1];
    for (i = a; i < b; ++i) {
        EditRec *r = &j->r[i];
        world_set(&g->world, r->dim, r->x, r->y, r->z, r->newb, r->newm);
    }
    ++j->pos;
    g->unsaved = 1;
    return (int)(b - a > 32000L ? 32000L : b - a);
}

static int skill_for(const BlockDef *b)
{
    return b->tool == TOOL_AXE ? SK_WOODCUTTING : SK_MINING;
}

int mine_time(Game *g, int block)
{
    const BlockDef *b = &g_blocks[block];
    ItemStack *st = inv_selected(&g->inv);
    long t = b->hardness;
    if (st->item >= 0 && g_items[st->item].type == ITEM_TOOL && b->tool != TOOL_NONE &&
        g_items[st->item].tool_type == b->tool) {
        t = t * 100L / g_items[st->item].tool_power;
    } else if (b->tool != TOOL_NONE && b->tool != TOOL_SHOVEL) {
        t = t * 3L;
    }
    t = t * 100L / (100L + (long)g->p.stats[ST_STR] * 2L);
    t = t * 100L / (100L + (long)player_skill_level(&g->p, skill_for(b)) * 3L);
    if (t < 1) {
        t = 1;
    }
    if (t > 3000L) {
        t = 3000L;
    }
    return (int)t;
}

static void wear_tool(Game *g, const BlockDef *b)
{
    ItemStack *st = inv_selected(&g->inv);
    if (g->mode == GM_CREATIVE || st->item < 0 || g_items[st->item].durability <= 0) {
        return;
    }
    if (g_items[st->item].type != ITEM_TOOL && g_items[st->item].type != ITEM_WEAPON) {
        return;
    }
    st->dur = (cl_s16)(st->dur - (g_items[st->item].tool_type == b->tool ? 1 : 2));
    if (st->dur <= 0) {
        log_add(ATTR(C_YELLOW, C_BLACK), loc_fmt("msg.tool_broke", item_name(st->item), NULL, NULL));
        inv_remove_slot(&g->inv, g->inv.selected, 1);
    } else if (st->dur == g_items[st->item].durability / 10) {
        log_add(ATTR(C_YELLOW, C_BLACK), loc_fmt("msg.tool_worn", item_name(st->item), NULL, NULL));
    }
}

void mine_finish(Game *g)
{
    Player *p = &g->p;
    cl_u8 b = world_get(&g->world, p->dim, p->mx, p->my, p->mz);
    cl_u8 m = world_get_meta(&g->world, p->dim, p->mx, p->my, p->mz);
    const BlockDef *bd = &g_blocks[b];
    p->mining = 0;
    if (b == BLOCK_AIR || blk_liquid(b) || (bd->flags & BF_NOMINE)) {
        return;
    }
    world_set(&g->world, p->dim, p->mx, p->my, p->mz, BLOCK_AIR, 0);
    g->unsaved = 1;
    if (g->mode != GM_CREATIVE && bd->drop_item >= 0 && bd->drop_count > 0) {
        int n = bd->drop_count;
        int left;
        if (rng_range(&g->rng, 100) < p->stats[ST_LCK] / 2 && !(m & META_PLACED)) {
            ++n;
        }
        left = inv_add(&g->inv, bd->drop_item, n, 0);
        if (left > 0) {
            ent_spawn(g, p->dim, p->mx, p->my, p->mz, bd->drop_item, left, 0);
            log_add(ATTR(C_YELLOW, C_BLACK), loc_get("msg.inventory_full"));
        }
    }
    wear_tool(g, bd);
    if (!(m & META_PLACED)) {
        skill_gain(g, skill_for(bd), bd->hardness / 4 + 1);
    }
    if (g->mode == GM_SURVIVAL && !p->god) {
        p->stamina -= 6;
        if (p->stamina < 0) {
            p->stamina = 0;
        }
    }
}
