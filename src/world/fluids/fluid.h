/*
 * Cliderness - world/fluids/fluid.h  (ClideFluid)
 *
 * Water and lava store a level 1..15 in the low nibble of the block meta
 * (15 = full cell). Generated water is static; a cell only becomes active
 * when something changes next to it (mining, placing, commands, rain).
 *
 * Each fluid step processes up to `budget` cells from an active-cell ring
 * buffer (DOS: small queue and budget). A cell first pours as much as fits
 * into the cell below, then gives one unit to each lower neighbour - total
 * volume is conserved exactly. Exceptions, by design:
 *   - sea / lake sources: water below sea level where the generator placed
 *     water refills to 15 (oceans cannot be drained by a ditch);
 *   - rain adds water to depressions; water meeting lava turns the lava
 *     into obsidian (full) or cobblestone (partial) and is consumed.
 * Lava moves on every third step and needs a level difference of 3.
 */
#ifndef CLIDE_WORLD_FLUID_H
#define CLIDE_WORLD_FLUID_H

#include "core/types.h"

struct World;

#ifdef CLIDE_TARGET_DOS16
#define FLUID_QUEUE  512
#define FLUID_BUDGET 96
#else
#define FLUID_QUEUE  16384
#define FLUID_BUDGET 2048
#endif

typedef struct FluidCell {
    long   x, z;
    cl_s16 y;
    cl_u8  dim;
    cl_u8  pad;
} FluidCell;

typedef struct FluidSim {
    FluidCell *q;
    long head, count;
    long budget;
    long step;
    int  enabled;
    long processed;       /* statistics */
    long dropped;
} FluidSim;

int  fluid_init(FluidSim *f);
void fluid_free(FluidSim *f);
void fluid_enqueue(struct World *w, int dim, long x, long y, long z);
/* enqueue the cell and its 6 neighbours if any of them is a fluid */
void fluid_notify(struct World *w, int dim, long x, long y, long z);
/* run one simulation step; returns number of cells processed */
long fluid_step(struct World *w);
int  fluid_level(struct World *w, int dim, long x, long y, long z);
/* place water/lava with a level (no undo, notifies neighbours) */
void fluid_put(struct World *w, int dim, long x, long y, long z, int block, int level);
int  fluid_is_source(struct World *w, int dim, long x, long y, long z);
/* total fluid units of `block` in a box (for tests and /volume) */
long fluid_volume(struct World *w, int dim, int block, long x1, long y1, long z1, long x2, long y2, long z2);

#endif
