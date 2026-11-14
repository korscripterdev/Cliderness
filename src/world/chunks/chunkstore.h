/*
 * Cliderness - world/chunks/chunkstore.h
 * Disk store for player-modified chunks.
 *
 *   chunks.dat  last verified save (header "CLCHUNK1", records)
 *   chunks.jnl  session journal: chunks evicted from the cache since the last
 *               save. Discarded if the game exits without saving, so the
 *               verified save is never touched until a full commit succeeds.
 *
 * Record: "CK" dim(1) pad(1) y(2) cx(4) cz(4) len(2) crc(4) data(len), all
 * little-endian, data = RLE (count,value) pairs of 1024 blocks + 1024 meta.
 * Only a small index (16 bytes per modified chunk) is kept in memory.
 */
#ifndef CLIDE_WORLD_CHUNKSTORE_H
#define CLIDE_WORLD_CHUNKSTORE_H

#include <stdio.h>
#include "core/types.h"

typedef struct StoreEntry {
    long  cx, cz;
    cl_s16 y;
    cl_u8 dim;
    cl_u8 file;      /* 0 = chunks.dat, 1 = journal */
    long  offset;
} StoreEntry;

typedef struct ChunkStore {
    StoreEntry *e;
    int   count, cap, max;
    char  main_path[160];
    char  jnl_path[160];
    char  new_path[160];
    FILE *mainf;
    FILE *jnl;
    int   errors;
} ChunkStore;

int  store_open(ChunkStore *s, const char *dir);
void store_close(ChunkStore *s);
int  store_find(const ChunkStore *s, int dim, long cx, int y, long cz);
/* 1 = found and valid, 0 = not stored, -1 = corrupt */
int  store_read(ChunkStore *s, int dim, long cx, int y, long cz, cl_u8 *blocks, cl_u8 *meta);
int  store_write(ChunkStore *s, int dim, long cx, int y, long cz, const cl_u8 *blocks, const cl_u8 *meta);
/* Rewrite chunks.dat from all entries, verify, atomically replace. */
int  store_commit(ChunkStore *s, int (*replace)(const char *, const char *));
int  store_rle_encode(const cl_u8 *src, int n, cl_u8 *dst, int cap);
int  store_rle_decode(const cl_u8 *src, int len, cl_u8 *dst, int n);

#endif
