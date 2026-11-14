/*
 * Cliderness - world/world.h
 * 3D block world, rendered top-down. Axes: x = east, z = south, y = up.
 * Commands use the (x y z) order with y as height.
 *
 * Storage: a chunk is one 32x32 horizontal slice at a single y level of one
 * dimension. Chunks live in a bounded LRU cache; generated chunks are never
 * saved, only chunks that differ from the generator (CH_MODIFIED) are written
 * to the chunk store, which keeps "world seed" and "player changes" apart.
 */
#ifndef CLIDE_WORLD_H
#define CLIDE_WORLD_H

#include "core/types.h"
#include "world/chunks/chunkstore.h"

#define WORLD_H      128
#define SEA_LEVEL    62
#define CHUNK_SHIFT  5
#define CHUNK_SIZE   32
#define CHUNK_CELLS  1024

#define DIM_OVERWORLD 0
#define DIM_ANCIENT   1

#define META_FLUID  0x0F
#define META_PLACED 0x80

#define CH_USED     1
#define CH_MODIFIED 2
#define CH_DIRTY    4

#ifdef CLIDE_TARGET_DOS16
#define WORLD_CHUNK_CACHE  40
#define WORLD_COLUMN_CACHE 6
#else
#define WORLD_CHUNK_CACHE  768
#define WORLD_COLUMN_CACHE 48
#endif
#define WORLD_BUCKETS 256

typedef struct Chunk {
    long   cx, cz;
    int    y;
    cl_u8  dim;
    cl_u8  flags;
    cl_u32 stamp;
    int    next;
    cl_u8  blocks[CHUNK_CELLS];
    cl_u8  meta[CHUNK_CELLS];
} Chunk;

typedef struct ColumnData {
    long   cx, cz;
    cl_u8  dim;
    cl_u8  valid;
    cl_u32 stamp;
    cl_u8  height[CHUNK_CELLS];
    cl_u8  biome[CHUNK_CELLS];
    cl_u8  temp[CHUNK_CELLS];
    cl_u8  humid[CHUNK_CELLS];
} ColumnData;

typedef struct WorldStats {
    cl_u32 generated, loaded, evicted, written;
} WorldStats;

typedef struct World {
    cl_u32      seed;
    char        dir[128];
    /* one allocation per chunk: keeps every malloc far below 64 KB,
     * which 16-bit DOS (size_t = 16 bits) requires */
    Chunk      *chunks[WORLD_CHUNK_CACHE];
    int         nchunks;
    int         buckets[WORLD_BUCKETS];
    ColumnData *cols[WORLD_COLUMN_CACHE];
    int         ncols;
    cl_u32      stamp;
    int         last;
    ChunkStore  store;
    int         store_open;
    WorldStats  stats;
    /* atomic file replace from the platform layer (NULL = portable fallback) */
    int       (*replace)(const char *tmp, const char *dst);
} World;

/* dir may be NULL for a world that is never saved (tests). */
int   world_open(World *w, cl_u32 seed, const char *dir);
void  world_close(World *w);
/* Write all modified chunks; returns 1 on success. */
int   world_save(World *w);

cl_u8 world_get(World *w, int dim, long x, long y, long z);
cl_u8 world_get_meta(World *w, int dim, long x, long y, long z);
/* Sets a block (and meta). Marks the chunk modified. Returns 0 if y is out of range. */
int   world_set(World *w, int dim, long x, long y, long z, cl_u8 block, cl_u8 meta);

const ColumnData *world_column(World *w, int dim, long x, long z, int *index);
int   world_gen_height(World *w, int dim, long x, long z);
int   world_biome(World *w, int dim, long x, long z);
int   world_climate_temp(World *w, int dim, long x, long z);   /* 0..100 */
/* highest non-air block at (x,z) scanning down from y_from */
long  world_top(World *w, int dim, long x, long z, long y_from);
void  world_find_spawn(World *w, long *x, long *y, long *z);
int   world_cached_chunks(const World *w);
int   world_modified_chunks(World *w);
cl_u32 world_memory(const World *w);

#endif
