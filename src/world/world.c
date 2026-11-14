/* Cliderness - world/world.c : chunk cache, column cache, block access */
#include <string.h>
#include "world/world.h"
#include "world/terrain/terrain.h"
#include "world/biomes/biome.h"
#include "content/content.h"
#include "core/mathx.h"
#include "core/mem.h"
#include "core/str.h"

static int bucket_of(int dim, long cx, int y, long cz)
{
    cl_u32 h = CL_U32((cl_u32)cx * 73856093UL) ^ CL_U32((cl_u32)cz * 19349663UL) ^
               CL_U32((cl_u32)y * 83492791UL) ^ (cl_u32)dim;
    return (int)(h % WORLD_BUCKETS);
}

int world_open(World *w, cl_u32 seed, const char *dir)
{
    int i;
    memset(w, 0, sizeof(*w));
    w->seed = seed;
    w->nchunks = WORLD_CHUNK_CACHE;
    w->ncols = WORLD_COLUMN_CACHE;
    for (i = 0; i < w->nchunks; ++i) {
        w->chunks[i] = (Chunk *)mem_calloc(sizeof(Chunk));
        if (!w->chunks[i]) {
            world_close(w);
            return 0;
        }
    }
    for (i = 0; i < w->ncols; ++i) {
        w->cols[i] = (ColumnData *)mem_calloc(sizeof(ColumnData));
        if (!w->cols[i]) {
            world_close(w);
            return 0;
        }
    }
    for (i = 0; i < WORLD_BUCKETS; ++i) {
        w->buckets[i] = -1;
    }
    w->last = -1;
    if (dir) {
        str_copy(w->dir, dir, sizeof(w->dir));
        if (!store_open(&w->store, dir)) {
            world_close(w);
            return 0;
        }
        w->store_open = 1;
    }
    terrain_init_ores();
    if (!fluid_init(&w->fluid)) {
        world_close(w);
        return 0;
    }
    return 1;
}

void world_close(World *w)
{
    fluid_free(&w->fluid);
    if (w->store_open) {
        store_close(&w->store);
        w->store_open = 0;
    }
    {
        int i;
        for (i = 0; i < WORLD_CHUNK_CACHE; ++i) {
            mem_free(w->chunks[i]);
            w->chunks[i] = NULL;
        }
        for (i = 0; i < WORLD_COLUMN_CACHE; ++i) {
            mem_free(w->cols[i]);
            w->cols[i] = NULL;
        }
    }
}

static ColumnData *column_fetch(World *w, int dim, long cx, long cz)
{
    int i, victim = 0;
    cl_u32 oldest = 0xFFFFFFFFUL;
    for (i = 0; i < w->ncols; ++i) {
        ColumnData *c = w->cols[i];
        if (c->valid && c->cx == cx && c->cz == cz && c->dim == dim) {
            c->stamp = ++w->stamp;
            return c;
        }
        if (!c->valid) {
            victim = i;
            oldest = 0;
        } else if (c->stamp < oldest) {
            oldest = c->stamp;
            victim = i;
        }
    }
    {
        ColumnData *c = w->cols[victim];
        c->cx = cx;
        c->cz = cz;
        c->dim = (cl_u8)dim;
        terrain_gen_column(w->seed, dim, c);
        c->stamp = ++w->stamp;
        return c;
    }
}

static void unlink_chunk(World *w, int idx)
{
    Chunk *c = w->chunks[idx];
    int b = bucket_of(c->dim, c->cx, c->y, c->cz);
    int *p = &w->buckets[b];
    while (*p >= 0) {
        if (*p == idx) {
            *p = c->next;
            return;
        }
        p = &w->chunks[*p]->next;
    }
}

static void flush_chunk(World *w, Chunk *c)
{
    if ((c->flags & CH_MODIFIED) && (c->flags & CH_DIRTY) && w->store_open) {
        if (store_write(&w->store, c->dim, c->cx, c->y, c->cz, c->blocks, c->meta)) {
            c->flags &= (cl_u8)~CH_DIRTY;
            ++w->stats.written;
        }
    }
}

static Chunk *chunk_fetch(World *w, int dim, long cx, int y, long cz)
{
    int b, i, victim = -1;
    cl_u32 oldest = 0xFFFFFFFFUL;
    Chunk *c;
    if (w->last >= 0) {
        c = w->chunks[w->last];
        if (c->cx == cx && c->cz == cz && c->y == y && c->dim == dim) {
            return c;
        }
    }
    b = bucket_of(dim, cx, y, cz);
    for (i = w->buckets[b]; i >= 0; i = w->chunks[i]->next) {
        c = w->chunks[i];
        if (c->cx == cx && c->cz == cz && c->y == y && c->dim == dim) {
            c->stamp = ++w->stamp;
            w->last = i;
            return c;
        }
    }
    for (i = 0; i < w->nchunks; ++i) {
        c = w->chunks[i];
        if (!(c->flags & CH_USED)) {
            victim = i;
            break;
        }
        /* never evict a modified chunk that could not be written */
        if ((c->flags & CH_DIRTY) && !w->store_open) {
            continue;
        }
        if (c->stamp < oldest) {
            oldest = c->stamp;
            victim = i;
        }
    }
    if (victim < 0) {
        victim = 0;
    }
    c = w->chunks[victim];
    if (c->flags & CH_USED) {
        flush_chunk(w, c);
        unlink_chunk(w, victim);
        ++w->stats.evicted;
    }
    c->cx = cx;
    c->cz = cz;
    c->y = y;
    c->dim = (cl_u8)dim;
    c->flags = CH_USED;
    if (w->store_open && store_read(&w->store, dim, cx, y, cz, c->blocks, c->meta) == 1) {
        c->flags |= CH_MODIFIED;
        ++w->stats.loaded;
    } else {
        const ColumnData *col = column_fetch(w, dim, cx, cz);
        terrain_gen_chunk(w->seed, col, c);
        ++w->stats.generated;
    }
    c->stamp = ++w->stamp;
    c->next = w->buckets[b];
    w->buckets[b] = victim;
    w->last = victim;
    return c;
}

static void split(long x, long z, long *cx, long *cz, int *idx)
{
    long lx, lz;
    if (x >= 0) {
        *cx = x >> CHUNK_SHIFT;
        lx = x & (CHUNK_SIZE - 1);
    } else {
        *cx = mx_floor_div(x, CHUNK_SIZE);
        lx = x - *cx * CHUNK_SIZE;
    }
    if (z >= 0) {
        *cz = z >> CHUNK_SHIFT;
        lz = z & (CHUNK_SIZE - 1);
    } else {
        *cz = mx_floor_div(z, CHUNK_SIZE);
        lz = z - *cz * CHUNK_SIZE;
    }
    *idx = (int)(lz * CHUNK_SIZE + lx);
}

cl_u8 world_get(World *w, int dim, long x, long y, long z)
{
    long cx, cz;
    int idx;
    if (y < 0) {
        return (cl_u8)g_cb.bedrock;
    }
    if (y >= WORLD_H) {
        return BLOCK_AIR;
    }
    split(x, z, &cx, &cz, &idx);
    return chunk_fetch(w, dim, cx, (int)y, cz)->blocks[idx];
}

cl_u8 world_get_meta(World *w, int dim, long x, long y, long z)
{
    long cx, cz;
    int idx;
    if (y < 0 || y >= WORLD_H) {
        return 0;
    }
    split(x, z, &cx, &cz, &idx);
    return chunk_fetch(w, dim, cx, (int)y, cz)->meta[idx];
}

int world_set(World *w, int dim, long x, long y, long z, cl_u8 block, cl_u8 meta)
{
    int r = world_set_raw(w, dim, x, y, z, block, meta);
    if (r) {
        fluid_notify(w, dim, x, y, z);
    }
    return r;
}

int world_set_raw(World *w, int dim, long x, long y, long z, cl_u8 block, cl_u8 meta)
{
    long cx, cz;
    int idx;
    Chunk *c;
    if (y < 0 || y >= WORLD_H) {
        return 0;
    }
    split(x, z, &cx, &cz, &idx);
    c = chunk_fetch(w, dim, cx, (int)y, cz);
    if (c->blocks[idx] == block && c->meta[idx] == meta) {
        return 1;
    }
    c->blocks[idx] = block;
    c->meta[idx] = meta;
    c->flags |= CH_MODIFIED | CH_DIRTY;
    return 1;
}

const ColumnData *world_column(World *w, int dim, long x, long z, int *index)
{
    long cx, cz;
    int idx;
    split(x, z, &cx, &cz, &idx);
    if (index) {
        *index = idx;
    }
    return column_fetch(w, dim, cx, cz);
}

int world_gen_height(World *w, int dim, long x, long z)
{
    int i;
    const ColumnData *c = world_column(w, dim, x, z, &i);
    return c->height[i];
}

int world_biome(World *w, int dim, long x, long z)
{
    int i;
    const ColumnData *c = world_column(w, dim, x, z, &i);
    return c->biome[i];
}

int world_climate_temp(World *w, int dim, long x, long z)
{
    int i;
    const ColumnData *c = world_column(w, dim, x, z, &i);
    return c->temp[i];
}

long world_top(World *w, int dim, long x, long z, long y_from)
{
    long y;
    if (y_from >= WORLD_H) {
        y_from = WORLD_H - 1;
    }
    for (y = y_from; y > 0; --y) {
        if (world_get(w, dim, x, y, z) != BLOCK_AIR) {
            return y;
        }
    }
    return 0;
}

void world_find_spawn(World *w, long *sx, long *sy, long *sz)
{
    long r, x, z;
    for (r = 0; r < 2000; r += 7) {
        for (x = -r; x <= r; x += 7) {
            for (z = -r; z <= r; z += (r ? 2 * r : 1)) {
                int h = world_gen_height(w, DIM_OVERWORLD, x, z);
                cl_u8 above = world_get(w, DIM_OVERWORLD, x, h + 1, z);
                if (h >= SEA_LEVEL + 1 && h < 90 && !blk_solid(above) &&
                    !blk_liquid(world_get(w, DIM_OVERWORLD, x, h, z))) {
                    *sx = x;
                    *sy = h + 1;
                    *sz = z;
                    return;
                }
                if (r == 0) {
                    break;
                }
            }
        }
    }
    *sx = 0;
    *sz = 0;
    *sy = world_top(w, DIM_OVERWORLD, 0, 0, WORLD_H - 1) + 1;
}

int world_cached_chunks(const World *w)
{
    int i, n = 0;
    for (i = 0; i < w->nchunks; ++i) {
        if (w->chunks[i]->flags & CH_USED) {
            ++n;
        }
    }
    return n;
}

int world_modified_chunks(World *w)
{
    int i, n = w->store_open ? w->store.count : 0;
    for (i = 0; i < w->nchunks; ++i) {
        Chunk *c = w->chunks[i];
        if ((c->flags & CH_USED) && (c->flags & CH_MODIFIED) &&
            (!w->store_open || store_find(&w->store, c->dim, c->cx, c->y, c->cz) < 0)) {
            ++n;
        }
    }
    return n;
}

cl_u32 world_memory(const World *w)
{
    return (cl_u32)sizeof(Chunk) * (cl_u32)w->nchunks +
           (cl_u32)sizeof(ColumnData) * (cl_u32)w->ncols +
           (cl_u32)sizeof(StoreEntry) * (cl_u32)w->store.cap;
}

int world_save(World *w)
{
    int i;
    if (!w->store_open) {
        return 0;
    }
    for (i = 0; i < w->nchunks; ++i) {
        Chunk *c = w->chunks[i];
        if ((c->flags & CH_USED) && (c->flags & CH_MODIFIED) &&
            ((c->flags & CH_DIRTY) || store_find(&w->store, c->dim, c->cx, c->y, c->cz) < 0)) {
            c->flags |= CH_DIRTY;
            flush_chunk(w, c);
            if (c->flags & CH_DIRTY) {
                return 0;
            }
        }
    }
    return store_commit(&w->store, w->replace);
}
