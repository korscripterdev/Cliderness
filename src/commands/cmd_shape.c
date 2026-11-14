/*
 * Cliderness - commands/cmd_shape.c : building shapes around the player.
 * Every shape goes through the edit journal (/undo, /redo). Sizes are
 * limited so that one command never edits more than EDIT_MAX_VOLUME cells.
 */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "localization/locale.h"
#include "core/mathx.h"
#include "core/str.h"

enum { SH_SPHERE = 0, SH_DOME, SH_CYLINDER, SH_BOX, SH_WALLS, SH_PYRAMID, SH_CIRCLE, SH_PLATFORM };

static int sblock(ConCtx *c, int i, int *out)
{
    int b;
    if (i >= c->argc) return 0;
    b = block_find(c->argv[i]);
    if (b < 0) {
        con_err(loc_fmt("con.unknown_block", c->argv[i], NULL, NULL));
        return -1;
    }
    *out = b;
    return 1;
}

static int snum(ConCtx *c, int i, long def, long lo, long hi, long *out)
{
    *out = def;
    if (i < c->argc && (!con_arg_long(c, i, out) || *out < lo || *out > hi)) {
        return 0;
    }
    return 1;
}

static int hollow_flag(ConCtx *c, int i)
{
    return i < c->argc && (str_ieq(c->argv[i], "hollow") || str_ieq(c->argv[i], "h"));
}

static void put(ConCtx *c, long x, long y, long z, int b, long *n)
{
    if (y < 1 || y >= WORLD_H) return;
    edit_set(c->g, c->g->p.dim, x, y, z, (cl_u8)b, b ? META_PLACED : 0);
    ++*n;
}

static int finish(ConCtx *c, long n)
{
    char buf[32];
    int und = edit_end(&c->g->journal);
    sprintf(buf, "%ld", n);
    con_good(loc_fmt("con.changed", buf, NULL, NULL));
    if (!und && n > 0) con_out(loc_get("con.no_undo"));
    return CMD_OK;
}

/* the generic solid: membership test per cell */
static int inside(int shape, long dx, long dy, long dz, long r, long h, int hollow)
{
    long d2 = dx * dx + dy * dy + dz * dz, rr = r * r, ri = (r - 1) * (r - 1);
    switch (shape) {
    case SH_SPHERE:
        return d2 <= rr + r && (!hollow || d2 > ri + (r - 1));
    case SH_DOME:
        return dy >= 0 && d2 <= rr + r && d2 > ri + (r - 1);
    case SH_CYLINDER: {
        long q = dx * dx + dz * dz;
        if (dy < 0 || dy >= h || q > rr + r) return 0;
        return !hollow || q > ri + (r - 1) || dy == 0 || dy == h - 1;
    }
    case SH_WALLS:
        return dy >= 0 && dy < h && (mx_abs(dx) == r || mx_abs(dz) == r) && mx_abs(dx) <= r && mx_abs(dz) <= r;
    case SH_PYRAMID:
        return dy >= 0 && dy <= r && mx_abs(dx) <= r - dy && mx_abs(dz) <= r - dy &&
               (!hollow || mx_abs(dx) == r - dy || mx_abs(dz) == r - dy);
    case SH_CIRCLE: {
        long q = dx * dx + dz * dz;
        return dy == 0 && q <= rr + r && q > ri + (r - 1);
    }
    case SH_PLATFORM:
        return dy == 0 && dx * dx + dz * dz <= rr + r;
    default:
        return 0;
    }
}

static int shape_cmd(ConCtx *c, int shape, int needs_h)
{
    int b, r0;
    long r, h = 1, n = 0, dx, dy, dz, ylo, yhi;
    Player *p = &c->g->p;
    int hollow;
    r0 = sblock(c, 1, &b);
    if (r0 <= 0) return r0 < 0 ? CMD_ERR : con_usage(c);
    if (!snum(c, 2, 4, 1, 24, &r)) return con_usage(c);
    if (needs_h && !snum(c, 3, 4, 1, 48, &h)) return con_usage(c);
    hollow = hollow_flag(c, needs_h ? 4 : 3);
    ylo = shape == SH_SPHERE ? -r : 0;
    yhi = shape == SH_SPHERE || shape == SH_DOME || shape == SH_PYRAMID ? r : h - 1;
    if (shape == SH_PLATFORM) { ylo = -1; yhi = -1; }
    if (shape == SH_CIRCLE) { ylo = 0; yhi = 0; }
    edit_begin(&c->g->journal);
    for (dy = ylo; dy <= yhi; ++dy) {
        for (dz = -r; dz <= r; ++dz) {
            for (dx = -r; dx <= r; ++dx) {
                long ty = shape == SH_PLATFORM ? 0 : dy;
                if (inside(shape, dx, ty, dz, r, h, hollow)) {
                    put(c, p->x + dx, p->y + dy, p->z + dz, b, &n);
                }
            }
        }
    }
    return finish(c, n);
}

static int c_sphere(ConCtx *c) { return shape_cmd(c, SH_SPHERE, 0); }
static int c_dome(ConCtx *c) { return shape_cmd(c, SH_DOME, 0); }
static int c_cylinder(ConCtx *c) { return shape_cmd(c, SH_CYLINDER, 1); }
static int c_walls(ConCtx *c) { return shape_cmd(c, SH_WALLS, 1); }
static int c_pyramid(ConCtx *c) { return shape_cmd(c, SH_PYRAMID, 0); }
static int c_circle(ConCtx *c) { return shape_cmd(c, SH_CIRCLE, 0); }
static int c_platform(ConCtx *c) { return shape_cmd(c, SH_PLATFORM, 0); }

/* hollow box with one corner at the player, extending east/up/south */
static int c_box(ConCtx *c)
{
    int b, r0;
    long sx, sy, sz, x, y, z, n = 0;
    Player *p = &c->g->p;
    r0 = sblock(c, 1, &b);
    if (r0 <= 0) return r0 < 0 ? CMD_ERR : con_usage(c);
    if (!snum(c, 2, 5, 2, 48, &sx) || !snum(c, 3, sx, 2, 48, &sy) || !snum(c, 4, sx, 2, 48, &sz)) {
        return con_usage(c);
    }
    edit_begin(&c->g->journal);
    for (y = 0; y < sy; ++y) {
        for (z = 0; z < sz; ++z) {
            for (x = 0; x < sx; ++x) {
                if (x == 0 || y == 0 || z == 0 || x == sx - 1 || y == sy - 1 || z == sz - 1) {
                    put(c, p->x + 1 + x, p->y - 1 + y, p->z + 1 + z, b, &n);
                }
            }
        }
    }
    return finish(c, n);
}

/* straight builds along the facing direction */
static int line_cmd(ConCtx *c, int kind)
{
    int b = BLOCK_AIR, r0, first = 1;
    long len, h = 1, w = 1, k, i, j, n = 0, dx, dz;
    Player *p = &c->g->p;
    if (kind != 3) {   /* tunnel carves air */
        r0 = sblock(c, 1, &b);
        if (r0 <= 0) return r0 < 0 ? CMD_ERR : con_usage(c);
        first = 2;
    }
    if (!snum(c, first, 8, 1, 64, &len)) return con_usage(c);
    if (kind == 1 && !snum(c, first + 1, 3, 1, 32, &h)) return con_usage(c);
    if (kind == 3) {
        if (!snum(c, first + 1, 1, 1, 5, &w) || !snum(c, first + 2, 2, 1, 6, &h)) return con_usage(c);
    }
    dir_delta(p->facing, &dx, &dz);
    edit_begin(&c->g->journal);
    for (k = 1; k <= len; ++k) {
        long x = p->x + dx * k, z = p->z + dz * k;
        switch (kind) {
        case 0:   /* line at foot level */
            put(c, x, p->y, z, b, &n);
            break;
        case 1:   /* wall */
            for (j = 0; j < h; ++j) put(c, x, p->y + j, z, b, &n);
            break;
        case 2:   /* bridge under the feet, 3 wide */
            for (i = -1; i <= 1; ++i) put(c, x - dz * i, p->y - 1, z + dx * i, b, &n);
            break;
        case 3:   /* tunnel */
            for (i = -(w / 2); i <= (w - 1) / 2; ++i) {
                for (j = 0; j < h; ++j) put(c, x - dz * i, p->y + j, z + dx * i, BLOCK_AIR, &n);
            }
            break;
        case 4:   /* stairs going up */
            put(c, x, p->y - 1 + k, z, b, &n);
            for (j = 0; j < 3; ++j) put(c, x, p->y + k + j, z, BLOCK_AIR, &n);
            break;
        default:
            break;
        }
    }
    return finish(c, n);
}

static int c_line(ConCtx *c) { return line_cmd(c, 0); }
static int c_wall(ConCtx *c) { return line_cmd(c, 1); }
static int c_bridge(ConCtx *c) { return line_cmd(c, 2); }
static int c_tunnel(ConCtx *c) { return line_cmd(c, 3); }
static int c_stairs(ConCtx *c) { return line_cmd(c, 4); }

static int c_pillar(ConCtx *c)
{
    int b, r0;
    long h, j, n = 0, dx, dz;
    Player *p = &c->g->p;
    r0 = sblock(c, 1, &b);
    if (r0 <= 0) return r0 < 0 ? CMD_ERR : con_usage(c);
    if (!snum(c, 2, 5, 1, 64, &h)) return con_usage(c);
    dir_delta(p->facing, &dx, &dz);
    edit_begin(&c->g->journal);
    for (j = 0; j < h; ++j) put(c, p->x + dx, p->y + j, p->z + dz, b, &n);
    return finish(c, n);
}

/* hollow out a sphere of air (a cave) */
static int c_carve(ConCtx *c)
{
    long r, dx, dy, dz, n = 0;
    Player *p = &c->g->p;
    if (!snum(c, 1, 3, 1, 12, &r)) return con_usage(c);
    edit_begin(&c->g->journal);
    for (dy = -r; dy <= r; ++dy)
        for (dz = -r; dz <= r; ++dz)
            for (dx = -r; dx <= r; ++dx)
                if (dx * dx + dy * dy + dz * dz <= r * r + r &&
                    world_get(&c->g->world, p->dim, p->x + dx, p->y + dy, p->z + dz) != BLOCK_AIR &&
                    world_get(&c->g->world, p->dim, p->x + dx, p->y + dy, p->z + dz) != (cl_u8)g_cb.bedrock) {
                    put(c, p->x + dx, p->y + dy, p->z + dz, BLOCK_AIR, &n);
                }
    return finish(c, n);
}

static const Command g_cmds[] = {
    { "sphere", "building", "<block> <radius 1-24> [hollow]", "bnx", "hollow", PERM_CHEAT, c_sphere, NULL },
    { "dome", "building", "<block> <radius 1-24>", "bn", NULL, PERM_CHEAT, c_dome, NULL },
    { "cylinder", "building", "<block> <radius> <height> [hollow]", "bnnx", "hollow", PERM_CHEAT, c_cylinder, NULL },
    { "walls", "building", "<block> <radius> <height>", "bnn", NULL, PERM_CHEAT, c_walls, NULL },
    { "pyramid", "building", "<block> <radius> [hollow]", "bnx", "hollow", PERM_CHEAT, c_pyramid, NULL },
    { "circle", "building", "<block> <radius>", "bn", NULL, PERM_CHEAT, c_circle, "ring" },
    { "platform", "building", "<block> <radius>", "bn", NULL, PERM_CHEAT, c_platform, NULL },
    { "box", "building", "<block> <x> [y] [z]", "bnnn", NULL, PERM_CHEAT, c_box, NULL },
    { "line", "building", "<block> <length>", "bn", NULL, PERM_CHEAT, c_line, NULL },
    { "wall", "building", "<block> <length> [height]", "bnn", NULL, PERM_CHEAT, c_wall, NULL },
    { "bridge", "building", "<block> <length>", "bn", NULL, PERM_CHEAT, c_bridge, NULL },
    { "tunnel", "building", "<length> [width] [height]", "nnn", NULL, PERM_CHEAT, c_tunnel, NULL },
    { "stairs", "building", "<block> <length>", "bn", NULL, PERM_CHEAT, c_stairs, NULL },
    { "pillar", "building", "<block> <height>", "bn", NULL, PERM_CHEAT, c_pillar, NULL },
    { "carve", "building", "[radius 1-12]", "n", NULL, PERM_CHEAT, c_carve, NULL }
};

CommandTable cmd_table_shape(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
