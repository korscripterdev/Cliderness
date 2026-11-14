/* Cliderness - world/fluids/fluid.c (ClideFluid) */
#include <string.h>
#include "world/fluids/fluid.h"
#include "world/world.h"
#include "content/content.h"
#include "core/mem.h"

int fluid_init(FluidSim *f)
{
    memset(f, 0, sizeof(*f));
    f->q = (FluidCell *)mem_alloc(sizeof(FluidCell) * (size_t)FLUID_QUEUE);
    f->budget = FLUID_BUDGET;
    f->enabled = 1;
    return f->q != NULL;
}

void fluid_free(FluidSim *f)
{
    mem_free(f->q);
    f->q = NULL;
    f->count = 0;
}

static int is_fluid(cl_u8 b)
{
    return (b == (cl_u8)g_cb.water || b == (cl_u8)g_cb.lava) && b != 0;
}

int fluid_level(World *w, int dim, long x, long y, long z)
{
    cl_u8 b = world_get(w, dim, x, y, z);
    int l;
    if (!is_fluid(b)) {
        return 0;
    }
    l = world_get_meta(w, dim, x, y, z) & META_FLUID;
    return l ? l : 15;
}

void fluid_enqueue(World *w, int dim, long x, long y, long z)
{
    FluidSim *f = &w->fluid;
    FluidCell *c;
    if (!f->q || y < 0 || y >= WORLD_H) {
        return;
    }
    if (f->count >= FLUID_QUEUE) {
        ++f->dropped;
        return;
    }
    c = &f->q[(f->head + f->count) % FLUID_QUEUE];
    c->x = x;
    c->y = (cl_s16)y;
    c->z = z;
    c->dim = (cl_u8)dim;
    ++f->count;
}

void fluid_notify(World *w, int dim, long x, long y, long z)
{
    static const int d[7][3] = { {0,0,0}, {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
    int k;
    if (!w->fluid.q || !w->fluid.enabled) {
        return;
    }
    for (k = 0; k < 7; ++k) {
        long nx = x + d[k][0], ny = y + d[k][1], nz = z + d[k][2];
        if (is_fluid(world_get(w, dim, nx, ny, nz))) {
            fluid_enqueue(w, dim, nx, ny, nz);
        }
    }
}

int fluid_is_source(World *w, int dim, long x, long y, long z)
{
    int h;
    if (y >= SEA_LEVEL) {
        return 0;
    }
    h = world_gen_height(w, dim, x, z);
    return y > h && h < SEA_LEVEL;
}

/* raw set: no fluid notification (the simulation enqueues by itself) */
static void put_raw(World *w, int dim, long x, long y, long z, int block, int level)
{
    if (level <= 0) {
        world_set_raw(w, dim, x, y, z, BLOCK_AIR, 0);
    } else {
        world_set_raw(w, dim, x, y, z, (cl_u8)block, (cl_u8)(level & META_FLUID));
    }
}

void fluid_put(World *w, int dim, long x, long y, long z, int block, int level)
{
    put_raw(w, dim, x, y, z, block, CL_CLAMP(level, 0, 15));
    fluid_enqueue(w, dim, x, y, z);
    fluid_notify(w, dim, x, y, z);
}

/* can a fluid flow into this cell? air or a replaceable plant */
static int open_cell(cl_u8 b)
{
    if (b == BLOCK_AIR) {
        return 1;
    }
    return (g_blocks[b].flags & BF_REPLACEABLE) && !(g_blocks[b].flags & BF_LIQUID);
}

static void react(World *w, int dim, long x, long y, long z, int lava_level)
{
    /* water touched this lava cell */
    int block = lava_level >= 15 ? block_find("obsidian") : g_cb.cobble;
    if (block < 0) {
        block = g_cb.stone;
    }
    world_set_raw(w, dim, x, y, z, (cl_u8)block, 0);
}

static void process(World *w, const FluidCell *c)
{
    static const int dx[4] = { 1, 0, -1, 0 }, dz[4] = { 0, 1, 0, -1 };
    int dim = c->dim;
    long x = c->x, y = c->y, z = c->z;
    cl_u8 b = world_get(w, dim, x, y, z);
    int level, start, k, min_diff, is_water;
    if (!is_fluid(b)) {
        return;
    }
    is_water = b == (cl_u8)g_cb.water;
    level = fluid_level(w, dim, x, y, z);
    start = level;
    if (!is_water && (w->fluid.step % 3L) != 0) {
        fluid_enqueue(w, dim, x, y, z);   /* lava is slow */
        return;
    }
    if (is_water && fluid_is_source(w, dim, x, y, z)) {
        level = 15;
    }
    /* 1. pour down */
    if (y > 0) {
        cl_u8 below = world_get(w, dim, x, y - 1, z);
        if (open_cell(below) || below == b) {
            int lb = below == b ? fluid_level(w, dim, x, y - 1, z) : 0;
            int t = CL_MIN(level, 15 - lb);
            if (t > 0) {
                put_raw(w, dim, x, y - 1, z, b, lb + t);
                fluid_enqueue(w, dim, x, y - 1, z);
                level -= t;
            }
        } else if (is_water && below == (cl_u8)g_cb.lava) {
            react(w, dim, x, y - 1, z, fluid_level(w, dim, x, y - 1, z));
            level -= 1;
        } else if (!is_water && below == (cl_u8)g_cb.water) {
            react(w, dim, x, y, z, level);
            fluid_notify(w, dim, x, y, z);
            return;
        }
    }
    /* 2. spread sideways.
     * a) edge flow: a unit moves to an open neighbour that has a drop below
     *    it (water runs over edges and into holes even as a thin film);
     * b) levelling: the cell and its open / same-fluid neighbours share
     *    their total equally when they differ by at least min_diff
     *    (2 for water, 3 for lava). Integer shares keep the volume exact;
     *    the remainder rotates so no direction is preferred. */
    min_diff = is_water ? 2 : 3;
    for (k = 0; k < 4 && level > 0; ++k) {
        int dir = (int)((k + w->fluid.step) & 3L);
        long nx = x + dx[dir], nz = z + dz[dir];
        cl_u8 n = world_get(w, dim, nx, y, nz);
        if (is_water && n == (cl_u8)g_cb.lava) {
            react(w, dim, nx, y, nz, fluid_level(w, dim, nx, y, nz));
            --level;
            continue;
        }
        if (open_cell(n) && y > 0) {
            cl_u8 nb = world_get(w, dim, nx, y - 1, nz);
            if (open_cell(nb) || (nb == b && fluid_level(w, dim, nx, y - 1, nz) < 15)) {
                put_raw(w, dim, nx, y, nz, b, 1);
                fluid_enqueue(w, dim, nx, y, nz);
                --level;
            }
        }
    }
    if (level > 0) {
        long gx[5], gz[5];
        int gl[5], n = 0, lo, hi, total, share, rest, j;
        gx[0] = x;
        gz[0] = z;
        gl[0] = level;
        n = 1;
        for (k = 0; k < 4; ++k) {
            long nx = x + dx[k], nz = z + dz[k];
            cl_u8 nbk = world_get(w, dim, nx, y, nz);
            if (open_cell(nbk) || nbk == b) {
                gx[n] = nx;
                gz[n] = nz;
                gl[n] = nbk == b ? fluid_level(w, dim, nx, y, nz) : 0;
                ++n;
            }
        }
        lo = hi = gl[0];
        total = 0;
        for (j = 0; j < n; ++j) {
            if (gl[j] < lo) lo = gl[j];
            if (gl[j] > hi) hi = gl[j];
            total += gl[j];
        }
        if (n > 1 && hi - lo >= min_diff) {
            share = total / n;
            rest = total % n;
            for (j = 0; j < n; ++j) {
                int idx = (int)((j + w->fluid.step) % n);
                int nl = share + (j < rest ? 1 : 0);
                if (idx == 0) {
                    level = nl;
                } else if (nl != gl[idx]) {
                    put_raw(w, dim, gx[idx], y, gz[idx], b, nl);
                    fluid_enqueue(w, dim, gx[idx], y, gz[idx]);
                }
            }
        }
    }
    if (is_water && fluid_is_source(w, dim, x, y, z)) {
        level = 15;
    }
    if (level != start || level != fluid_level(w, dim, x, y, z)) {
        put_raw(w, dim, x, y, z, b, level);
        if (level > 0) {
            fluid_enqueue(w, dim, x, y, z);
        }
        /* fluid above may now flow into us; neighbours may flow back */
        fluid_notify(w, dim, x, y, z);
    }
}

long fluid_step(World *w)
{
    FluidSim *f = &w->fluid;
    long n = 0, todo;
    if (!f->q || !f->enabled) {
        return 0;
    }
    ++f->step;
    todo = CL_MIN(f->count, f->budget);
    while (n < todo && f->count > 0) {
        FluidCell c = f->q[f->head];
        f->head = (f->head + 1) % FLUID_QUEUE;
        --f->count;
        process(w, &c);
        ++n;
    }
    f->processed += n;
    return n;
}

long fluid_volume(World *w, int dim, int block, long x1, long y1, long z1, long x2, long y2, long z2)
{
    long x, y, z, v = 0;
    for (y = y1; y <= y2; ++y) {
        for (z = z1; z <= z2; ++z) {
            for (x = x1; x <= x2; ++x) {
                if (world_get(w, dim, x, y, z) == (cl_u8)block) {
                    v += fluid_level(w, dim, x, y, z);
                }
            }
        }
    }
    return v;
}
