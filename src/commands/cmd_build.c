/*
 * Cliderness - commands/cmd_build.c : block, building and terrain commands.
 * Every edit goes through the edit journal so /undo and /redo work.
 */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "localization/locale.h"
#include "core/str.h"
#include "core/mathx.h"
#include "core/mem.h"

static int block_arg(ConCtx *c, int i, int *out)
{
    int b;
    if (i >= c->argc) {
        return 0;
    }
    b = block_find(c->argv[i]);
    if (b < 0) {
        con_err(loc_fmt("con.unknown_block", c->argv[i], NULL, NULL));
        return -1;
    }
    *out = b;
    return 1;
}

static int box_args(ConCtx *c, int first, long *x1, long *y1, long *z1, long *x2, long *y2, long *z2)
{
    Player *p = &c->g->p;
    long t;
    if (!con_arg_coord(c, first, p->x, x1) || !con_arg_coord(c, first + 1, p->y, y1) ||
        !con_arg_coord(c, first + 2, p->z, z1) || !con_arg_coord(c, first + 3, p->x, x2) ||
        !con_arg_coord(c, first + 4, p->y, y2) || !con_arg_coord(c, first + 5, p->z, z2)) {
        return 0;
    }
    if (*x1 > *x2) { t = *x1; *x1 = *x2; *x2 = t; }
    if (*y1 > *y2) { t = *y1; *y1 = *y2; *y2 = t; }
    if (*z1 > *z2) { t = *z1; *z1 = *z2; *z2 = t; }
    *y1 = CL_MAX(*y1, 0L);
    *y2 = CL_MIN(*y2, (long)(WORLD_H - 1));
    return 1;
}

static int volume_ok(long x1, long y1, long z1, long x2, long y2, long z2)
{
    long dx = x2 - x1 + 1, dy = y2 - y1 + 1, dz = z2 - z1 + 1;
    char buf[64];
    if (dx <= 0 || dy <= 0 || dz <= 0) {
        return 0;
    }
    if (dx > 1024 || dz > 1024 || dx * dy > EDIT_MAX_VOLUME || dx * dy * dz > EDIT_MAX_VOLUME) {
        sprintf(buf, "%ld", (long)EDIT_MAX_VOLUME);
        con_err(loc_fmt("con.too_big", buf, NULL, NULL));
        return 0;
    }
    return 1;
}

static void report(ConCtx *c, long n, int undoable)
{
    char buf[32];
    (void)c;
    sprintf(buf, "%ld", n);
    con_good(loc_fmt("con.changed", buf, NULL, NULL));
    if (!undoable && n > 0) {
        con_out(loc_get("con.no_undo"));
    }
}

static long count_changes_start(ConCtx *c)
{
    return c->g->journal.count;
}

static int c_setblock(ConCtx *c)
{
    long x, y, z;
    int b, r;
    Player *p = &c->g->p;
    if (c->argc < 5 || !con_arg_coord(c, 1, p->x, &x) || !con_arg_coord(c, 2, p->y, &y) ||
        !con_arg_coord(c, 3, p->z, &z)) {
        return con_usage(c);
    }
    r = block_arg(c, 4, &b);
    if (r <= 0) {
        return r < 0 ? CMD_ERR : con_usage(c);
    }
    if (y < 0 || y >= WORLD_H) {
        con_err(loc_get("con.bad_height"));
        return CMD_ERR;
    }
    edit_begin(&c->g->journal);
    edit_set(c->g, p->dim, x, y, z, (cl_u8)b, b ? META_PLACED : 0);
    report(c, 1, edit_end(&c->g->journal));
    return CMD_OK;
}

static int c_fill(ConCtx *c)
{
    long x1, y1, z1, x2, y2, z2, x, y, z, start;
    int b, r, und;
    int dim = c->g->p.dim;
    if (c->argc < 8 || !box_args(c, 1, &x1, &y1, &z1, &x2, &y2, &z2)) {
        return con_usage(c);
    }
    r = block_arg(c, 7, &b);
    if (r <= 0) {
        return r < 0 ? CMD_ERR : con_usage(c);
    }
    if (!volume_ok(x1, y1, z1, x2, y2, z2)) {
        return CMD_ERR;
    }
    edit_begin(&c->g->journal);
    start = count_changes_start(c);
    for (y = y1; y <= y2; ++y) {
        for (z = z1; z <= z2; ++z) {
            for (x = x1; x <= x2; ++x) {
                edit_set(c->g, dim, x, y, z, (cl_u8)b, b ? META_PLACED : 0);
            }
        }
    }
    {
        long n = c->g->journal.count - start;
        und = edit_end(&c->g->journal);
        report(c, und ? n : (x2 - x1 + 1) * (y2 - y1 + 1) * (z2 - z1 + 1), und);
    }
    return CMD_OK;
}

static int c_replace(ConCtx *c)
{
    long x1, y1, z1, x2, y2, z2, x, y, z, n = 0;
    int from, to, r, und;
    int dim = c->g->p.dim;
    if (c->argc < 9 || !box_args(c, 1, &x1, &y1, &z1, &x2, &y2, &z2)) {
        return con_usage(c);
    }
    r = block_arg(c, 7, &from);
    if (r <= 0) return r < 0 ? CMD_ERR : con_usage(c);
    r = block_arg(c, 8, &to);
    if (r <= 0) return r < 0 ? CMD_ERR : con_usage(c);
    if (!volume_ok(x1, y1, z1, x2, y2, z2)) {
        return CMD_ERR;
    }
    edit_begin(&c->g->journal);
    for (y = y1; y <= y2; ++y) {
        for (z = z1; z <= z2; ++z) {
            for (x = x1; x <= x2; ++x) {
                if (world_get(&c->g->world, dim, x, y, z) == from) {
                    edit_set(c->g, dim, x, y, z, (cl_u8)to, to ? META_PLACED : 0);
                    ++n;
                }
            }
        }
    }
    und = edit_end(&c->g->journal);
    report(c, n, und);
    return CMD_OK;
}

static int c_clone(ConCtx *c)
{
    long x1, y1, z1, x2, y2, z2, dx, dy, dz, x, y, z, n = 0;
    long sx, sy, sz;
    int und;
    int dim = c->g->p.dim;
    Player *p = &c->g->p;
    cl_u8 *buf, *mbuf;
    long vol, i = 0;
    if (c->argc < 10 || !box_args(c, 1, &x1, &y1, &z1, &x2, &y2, &z2) ||
        !con_arg_coord(c, 7, p->x, &dx) || !con_arg_coord(c, 8, p->y, &dy) || !con_arg_coord(c, 9, p->z, &dz)) {
        return con_usage(c);
    }
    if (!volume_ok(x1, y1, z1, x2, y2, z2)) {
        return CMD_ERR;
    }
    vol = (x2 - x1 + 1) * (y2 - y1 + 1) * (z2 - z1 + 1);
    if (vol > 32000L) {
        con_err(loc_fmt("con.too_big", "32000", NULL, NULL));
        return CMD_ERR;
    }
    buf = (cl_u8 *)mem_alloc((size_t)vol);
    mbuf = (cl_u8 *)mem_alloc((size_t)vol);
    if (!buf || !mbuf) {
        mem_free(buf);
        mem_free(mbuf);
        con_err("out of memory");
        return CMD_ERR;
    }
    /* copy first so overlapping regions work */
    for (y = y1; y <= y2; ++y) {
        for (z = z1; z <= z2; ++z) {
            for (x = x1; x <= x2; ++x) {
                buf[i] = world_get(&c->g->world, dim, x, y, z);
                mbuf[i] = world_get_meta(&c->g->world, dim, x, y, z);
                ++i;
            }
        }
    }
    edit_begin(&c->g->journal);
    i = 0;
    for (sy = 0; sy <= y2 - y1; ++sy) {
        for (sz = 0; sz <= z2 - z1; ++sz) {
            for (sx = 0; sx <= x2 - x1; ++sx) {
                edit_set(c->g, dim, dx + sx, dy + sy, dz + sz, buf[i], mbuf[i]);
                ++i;
                ++n;
            }
        }
    }
    und = edit_end(&c->g->journal);
    mem_free(buf);
    mem_free(mbuf);
    report(c, n, und);
    return CMD_OK;
}

static int c_undo(ConCtx *c)
{
    int n = edit_undo(c->g);
    char buf[16];
    if (n < 0) {
        con_err(loc_get("con.nothing_undo"));
        return CMD_ERR;
    }
    sprintf(buf, "%d", n);
    con_good(loc_fmt("con.undone", buf, NULL, NULL));
    return CMD_OK;
}

static int c_redo(ConCtx *c)
{
    int n = edit_redo(c->g);
    char buf[16];
    if (n < 0) {
        con_err(loc_get("con.nothing_redo"));
        return CMD_ERR;
    }
    sprintf(buf, "%d", n);
    con_good(loc_fmt("con.redone", buf, NULL, NULL));
    return CMD_OK;
}

static int radius_arg(ConCtx *c, int i, long def, long *r)
{
    *r = def;
    if (i < c->argc && (!con_arg_long(c, i, r) || *r < 0 || *r > 48)) {
        return 0;
    }
    return 1;
}

/* surface height of a column: top solid block at or below y_from */
static long col_top(ConCtx *c, long x, long z, long y_from)
{
    long y;
    for (y = y_from; y > 0; --y) {
        cl_u8 b = world_get(&c->g->world, c->g->p.dim, x, y, z);
        if (blk_solid(b) || blk_liquid(b)) {
            return y;
        }
    }
    return 0;
}

static int terrain_op(ConCtx *c, int op)
{
    long r, amount = 1, x, z, y, n = 0, base;
    int und;
    int dim = c->g->p.dim;
    Player *p = &c->g->p;
    cl_u8 floor;
    if (!radius_arg(c, 1, 5, &r)) {
        return con_usage(c);
    }
    if (c->argc >= 3 && (!con_arg_long(c, 2, &amount) || amount < 1 || amount > 32)) {
        return con_usage(c);
    }
    base = p->y - 1;
    floor = world_get(&c->g->world, dim, p->x, base, p->z);
    if (!blk_solid(floor)) {
        floor = (cl_u8)g_cb.dirt;
    }
    edit_begin(&c->g->journal);
    for (z = -r; z <= r; ++z) {
        for (x = -r; x <= r; ++x) {
            long wx = p->x + x, wz = p->z + z;
            long top;
            if (x * x + z * z > r * r) {
                continue;
            }
            top = col_top(c, wx, wz, p->y + 24);
            if (op == 0) {
                /* flatten: clear above base, fill below base */
                for (y = base + 1; y <= p->y + 24 && y < WORLD_H; ++y) {
                    if (world_get(&c->g->world, dim, wx, y, wz) != BLOCK_AIR) {
                        edit_set(c->g, dim, wx, y, wz, BLOCK_AIR, 0);
                        ++n;
                    }
                }
                for (y = base; y > base - 24 && y > 0; --y) {
                    cl_u8 b = world_get(&c->g->world, dim, wx, y, wz);
                    if (blk_solid(b)) {
                        break;
                    }
                    edit_set(c->g, dim, wx, y, wz, floor, META_PLACED);
                    ++n;
                }
            } else if (op == 1) {
                /* raise: add layers on top of each column */
                cl_u8 tb = world_get(&c->g->world, dim, wx, top, wz);
                if (!blk_solid(tb)) {
                    tb = floor;
                }
                for (y = top + 1; y <= top + amount && y < WORLD_H - 1; ++y) {
                    edit_set(c->g, dim, wx, y, wz, tb, META_PLACED);
                    ++n;
                }
            } else if (op == 2) {
                /* lower: remove top layers (never bedrock) */
                for (y = top; y > top - amount && y > 1; --y) {
                    cl_u8 b = world_get(&c->g->world, dim, wx, y, wz);
                    if (g_blocks[b].flags & BF_NOMINE) {
                        break;
                    }
                    edit_set(c->g, dim, wx, y, wz, BLOCK_AIR, 0);
                    ++n;
                }
            } else if (op == 3) {
                /* drain liquids */
                for (y = p->y + 8; y >= p->y - 16 && y > 0; --y) {
                    if (blk_liquid(world_get(&c->g->world, dim, wx, y, wz))) {
                        edit_set(c->g, dim, wx, y, wz, BLOCK_AIR, 0);
                        ++n;
                    }
                }
            } else if (op == 4) {
                /* flood: fill air below the player's level with water */
                for (y = base; y > base - 16 && y > 0; --y) {
                    cl_u8 b = world_get(&c->g->world, dim, wx, y, wz);
                    if (b != BLOCK_AIR) {
                        break;
                    }
                    edit_set(c->g, dim, wx, y, wz, (cl_u8)g_cb.water, 15);
                    ++n;
                }
            }
        }
    }
    und = edit_end(&c->g->journal);
    report(c, n, und);
    return CMD_OK;
}

static int c_flatten(ConCtx *c) { return terrain_op(c, 0); }
static int c_raise(ConCtx *c) { return terrain_op(c, 1); }
static int c_lower(ConCtx *c) { return terrain_op(c, 2); }
static int c_drain(ConCtx *c) { return terrain_op(c, 3); }
static int c_flood(ConCtx *c) { return terrain_op(c, 4); }

static int c_smooth(ConCtx *c)
{
    long r, x, z, y, n = 0;
    int und;
    int dim = c->g->p.dim;
    Player *p = &c->g->p;
    static cl_u8 h[99 * 99];
    long size;
    if (!radius_arg(c, 1, 5, &r)) {
        return con_usage(c);
    }
    size = 2 * (r + 1) + 1;
    for (z = -(r + 1); z <= r + 1; ++z) {
        for (x = -(r + 1); x <= r + 1; ++x) {
            h[(z + r + 1) * size + (x + r + 1)] = (cl_u8)col_top(c, p->x + x, p->z + z, p->y + 24);
        }
    }
    edit_begin(&c->g->journal);
    for (z = -r; z <= r; ++z) {
        for (x = -r; x <= r; ++x) {
            long sum = 0, k, j, cur, target;
            cl_u8 top_block;
            if (x * x + z * z > r * r) {
                continue;
            }
            for (k = -1; k <= 1; ++k) {
                for (j = -1; j <= 1; ++j) {
                    sum += h[(z + k + r + 1) * size + (x + j + r + 1)];
                }
            }
            target = (sum + 4) / 9;
            cur = h[(z + r + 1) * size + (x + r + 1)];
            top_block = world_get(&c->g->world, dim, p->x + x, cur, p->z + z);
            if (!blk_solid(top_block)) {
                top_block = (cl_u8)g_cb.dirt;
            }
            if (target > cur) {
                for (y = cur + 1; y <= target; ++y) {
                    edit_set(c->g, dim, p->x + x, y, p->z + z, top_block, META_PLACED);
                    ++n;
                }
            } else if (target < cur) {
                for (y = cur; y > target; --y) {
                    edit_set(c->g, dim, p->x + x, y, p->z + z, BLOCK_AIR, 0);
                    ++n;
                }
                edit_set(c->g, dim, p->x + x, target, p->z + z, top_block, META_PLACED);
            }
        }
    }
    und = edit_end(&c->g->journal);
    report(c, n, und);
    return CMD_OK;
}

static int c_blockinfo(ConCtx *c)
{
    long x, y, z, dx, dz;
    Player *p = &c->g->p;
    cl_u8 b, m;
    char buf[160];
    char nm[48];
    if (c->argc >= 4) {
        if (!con_arg_coord(c, 1, p->x, &x) || !con_arg_coord(c, 2, p->y, &y) || !con_arg_coord(c, 3, p->z, &z)) {
            return con_usage(c);
        }
    } else {
        dir_delta(p->facing, &dx, &dz);
        x = p->x + dx;
        y = p->y;
        z = p->z + dz;
        if (world_get(&c->g->world, p->dim, x, y, z) == BLOCK_AIR) {
            --y;
        }
    }
    b = world_get(&c->g->world, p->dim, x, y, z);
    m = world_get_meta(&c->g->world, p->dim, x, y, z);
    str_copy(nm, block_name(b), sizeof(nm));
    sprintf(buf, "%ld %ld %ld: %s [Cliderness:%s] hardness=%d %s", x, y, z, nm, g_blocks[b].id,
            g_blocks[b].hardness, (m & META_PLACED) ? "(placed)" : "(natural)");
    con_out(buf);
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "setblock", "block", "<x> <y> <z> <block>", "nnnb", NULL, PERM_CHEAT, c_setblock, NULL },
    { "fill", "building", "<x1> <y1> <z1> <x2> <y2> <z2> <block>", "nnnnnnb", NULL, PERM_CHEAT, c_fill, NULL },
    { "replace", "building", "<x1> <y1> <z1> <x2> <y2> <z2> <from> <to>", "nnnnnnbb", NULL, PERM_CHEAT, c_replace, NULL },
    { "clone", "building", "<x1> <y1> <z1> <x2> <y2> <z2> <x> <y> <z>", "nnnnnnnnn", NULL, PERM_CHEAT, c_clone, NULL },
    { "undo", "building", "", "", NULL, PERM_CHEAT, c_undo, NULL },
    { "redo", "building", "", "", NULL, PERM_CHEAT, c_redo, NULL },
    { "flatten", "terrain", "[radius]", "n", NULL, PERM_CHEAT, c_flatten, NULL },
    { "raise", "terrain", "[radius] [height]", "nn", NULL, PERM_CHEAT, c_raise, NULL },
    { "lower", "terrain", "[radius] [depth]", "nn", NULL, PERM_CHEAT, c_lower, NULL },
    { "smooth", "terrain", "[radius]", "n", NULL, PERM_CHEAT, c_smooth, NULL },
    { "drain", "fluid", "[radius]", "n", NULL, PERM_CHEAT, c_drain, NULL },
    { "flood", "fluid", "[radius]", "n", NULL, PERM_CHEAT, c_flood, NULL },
    { "blockinfo", "block", "[x y z]", "nnn", NULL, PERM_ANY, c_blockinfo, "lookat" }
};

CommandTable cmd_table_build(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
