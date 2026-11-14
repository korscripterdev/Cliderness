/*
 * Cliderness - world/terrain/terrain.h
 * Deterministic, seed based terrain generation. terrain_block() is a pure
 * function of (seed, column data, x, y, z), so chunk borders are seamless and
 * any chunk can be regenerated identically at any time.
 */
#ifndef CLIDE_WORLD_TERRAIN_H
#define CLIDE_WORLD_TERRAIN_H

#include "world/world.h"

void  terrain_init_ores(void);
void  terrain_gen_column(cl_u32 seed, int dim, ColumnData *col);
cl_u8 terrain_block(cl_u32 seed, int dim, const ColumnData *col, int index, long x, int y, long z);
void  terrain_gen_chunk(cl_u32 seed, const ColumnData *col, Chunk *ch);
/* raw climate samples, used by column generation and tests */
int   terrain_height(cl_u32 seed, long x, long z);

#endif
