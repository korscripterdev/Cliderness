/* Cliderness - tests/test_world.c : generation, chunks, caves, biomes, chunk store */
#include <stdio.h>
#include <string.h>
#include "tf.h"
#include "world/world.h"
#include "world/terrain/terrain.h"
#include "world/biomes/biome.h"
#include "world/chunks/chunkstore.h"
#include "platform/platform.h"
#include "content/content.h"
#include "core/mem.h"
#include "core/rng.h"

static World g_w1, g_w2;

static void t_determinism(void)
{
    int i, same = 1, diff = 0;
    CHECK(world_open(&g_w1, 777UL, NULL));
    CHECK(world_open(&g_w2, 777UL, NULL));
    for (i = 0; i < 3000; ++i) {
        cl_u32 h = rng_hash32((cl_u32)i);
        long x = (long)(h % 4000UL) - 2000L;
        long z = (long)((h >> 12) % 4000UL) - 2000L;
        long y = (long)((h >> 4) % WORLD_H);
        if (world_get(&g_w1, 0, x, y, z) != world_get(&g_w2, 0, x, y, z)) {
            same = 0;
        }
    }
    CHECK(same);
    world_close(&g_w2);
    CHECK(world_open(&g_w2, 778UL, NULL));
    for (i = 0; i < 400; ++i) {
        long x = (long)i * 37L - 7000L;
        if (world_gen_height(&g_w1, 0, x, 50) != world_gen_height(&g_w2, 0, x, 50)) {
            ++diff;
        }
    }
    CHECK(diff > 100);   /* another seed gives another world */
    world_close(&g_w2);
}

static void t_chunk_boundary(void)
{
    /* Blocks read through the cache across chunk borders must equal the
     * pure generator, including negative coordinates. */
    static ColumnData col;
    long xs[6], z;
    int i, ok = 1;
    xs[0] = -33; xs[1] = -32; xs[2] = -1; xs[3] = 0; xs[4] = 31; xs[5] = 32;
    for (i = 0; i < 6; ++i) {
        for (z = -33; z <= 32; z += 13) {
            int y;
            long cx = xs[i] >= 0 ? xs[i] / 32 : -((-xs[i] + 31) / 32);
            long cz = z >= 0 ? z / 32 : -((-z + 31) / 32);
            int idx = (int)((z - cz * 32) * 32 + (xs[i] - cx * 32));
            memset(&col, 0, sizeof(col));
            col.cx = cx;
            col.cz = cz;
            terrain_gen_column(777UL, 0, &col);
            for (y = 0; y < WORLD_H; y += 3) {
                if (terrain_block(777UL, 0, &col, idx, xs[i], y, z) != world_get(&g_w1, 0, xs[i], y, z)) {
                    ok = 0;
                }
            }
        }
    }
    CHECK(ok);
    /* heights of neighbouring columns across a border differ by a sane amount */
    {
        int h1 = world_gen_height(&g_w1, 0, 31, 5);
        int h2 = world_gen_height(&g_w1, 0, 32, 5);
        CHECK(h1 - h2 < 6 && h2 - h1 < 6);
    }
}

static void t_set_get(void)
{
    cl_u8 stone = (cl_u8)g_cb.stone;
    CHECK(world_set(&g_w1, 0, -5, 100, -7, stone, META_PLACED));
    CHECK(world_get(&g_w1, 0, -5, 100, -7) == stone);
    CHECK(world_get_meta(&g_w1, 0, -5, 100, -7) == META_PLACED);
    CHECK(!world_set(&g_w1, 0, 0, WORLD_H, 0, stone, 0));
    CHECK(world_get(&g_w1, 0, 0, -1, 0) == g_cb.bedrock);
    CHECK(world_get(&g_w1, 0, 0, 0, 0) == g_cb.bedrock);
    CHECK(world_get(&g_w1, 0, 0, WORLD_H + 5, 0) == BLOCK_AIR);
}

static void t_cache_bounded(void)
{
    long i;
    for (i = 0; i < WORLD_CHUNK_CACHE + 200; ++i) {
        world_get(&g_w1, 0, i * 32L, 64, 0);
    }
    CHECK(world_cached_chunks(&g_w1) <= WORLD_CHUNK_CACHE);
    CHECK(g_w1.stats.evicted > 0);
    /* DOS configuration must fit comfortably in conventional memory */
    CHECK((long)sizeof(Chunk) * 40L + (long)sizeof(ColumnData) * 6L < 128000L);
}

static void t_terrain_rules(void)
{
    long x;
    int land = 0, sea = 0, ok = 1, trees = 0;
    for (x = -4000; x < 4000; x += 17) {
        int h = world_gen_height(&g_w1, 0, x, x / 3);
        cl_u8 top = world_get(&g_w1, 0, x, h, x / 3);
        cl_u8 above = world_get(&g_w1, 0, x, h + 1, x / 3);
        if (h >= SEA_LEVEL) {
            ++land;
            if (blk_liquid(top)) ok = 0;
            if (blk_solid(above)) ++trees;
        } else if (h < SEA_LEVEL - 1) {
            ++sea;
            if (!blk_liquid(world_get(&g_w1, 0, x, SEA_LEVEL - 1, x / 3)) &&
                world_get(&g_w1, 0, x, SEA_LEVEL - 1, x / 3) != g_cb.ice) ok = 0;
        }
        if (world_get(&g_w1, 0, x, 0, x / 3) != g_cb.bedrock) ok = 0;
    }
    CHECK(ok);
    CHECK(land > 50 && sea > 20);
    CHECK(trees > 0);
}

static void t_biomes(void)
{
    int seen[MAX_BIOMES];
    int distinct = 0, i;
    long x, z;
    memset(seen, 0, sizeof(seen));
    for (z = -6000; z <= 6000; z += 150) {
        for (x = -6000; x <= 6000; x += 150) {
            seen[world_biome(&g_w1, 0, x, z)] = 1;
        }
    }
    for (i = 0; i < g_biome_count; ++i) {
        distinct += seen[i];
    }
    CHECK(distinct >= 10);
    CHECK(g_biome_count >= 16 + 5 + 5 + 11);
    CHECK(biome_find("plains") >= 0 && biome_find("coral_reef") >= 0 && biome_find("crystal_cave") >= 0);
}

static void t_ores(void)
{
    long x, z;
    int y, ores = 0;
    int coal = block_find("coal_ore");
    for (z = 0; z < 32; ++z) {
        for (x = 0; x < 32; ++x) {
            for (y = 20; y < 60; ++y) {
                if (g_blocks[world_get(&g_w1, 0, x, y, z)].ore_chance) ++ores;
            }
        }
    }
    CHECK(coal > 0);
    CHECK(ores > 10);
}

/* caves: underground air must form connected tunnels, not isolated cells */
static void t_cave_connectivity(void)
{
    enum { S = 48, H = 40, Y0 = 10 };
    static cl_u8 air[S][H][S];
    static short qx[S * H * S], qy[S * H * S], qz[S * H * S];
    int x, y, z, total = 0, comps = 0, biggest = 0;
    for (x = 0; x < S; ++x) for (y = 0; y < H; ++y) for (z = 0; z < S; ++z) {
        cl_u8 b = world_get(&g_w1, 0, x + 200, y + Y0, z - 300);
        int h = world_gen_height(&g_w1, 0, x + 200, z - 300);
        air[x][y][z] = (cl_u8)(b == BLOCK_AIR && y + Y0 < h - 5);
        total += air[x][y][z];
    }
    for (x = 0; x < S; ++x) for (y = 0; y < H; ++y) for (z = 0; z < S; ++z) {
        int head = 0, tail = 0, size = 0;
        if (!air[x][y][z]) continue;
        air[x][y][z] = 0;
        qx[tail] = (short)x; qy[tail] = (short)y; qz[tail] = (short)z; ++tail;
        while (head < tail) {
            int cx = qx[head], cy = qy[head], cz = qz[head], k;
            static const int d[6][3] = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
            ++head;
            ++size;
            for (k = 0; k < 6; ++k) {
                int nx = cx + d[k][0], ny = cy + d[k][1], nz = cz + d[k][2];
                if (nx < 0 || ny < 0 || nz < 0 || nx >= S || ny >= H || nz >= S) continue;
                if (!air[nx][ny][nz]) continue;
                air[nx][ny][nz] = 0;
                qx[tail] = (short)nx; qy[tail] = (short)ny; qz[tail] = (short)nz; ++tail;
            }
        }
        ++comps;
        if (size > biggest) biggest = size;
    }
    printf("    caves: %d air cells, %d components, largest %d\n", total, comps, biggest);
    CHECK(total > 100);
    CHECK(comps > 0 && total / comps >= 8);   /* average cave pocket is not a single cell */
    CHECK(biggest >= 60);
}

static void t_spawn(void)
{
    long x, y, z;
    world_find_spawn(&g_w1, &x, &y, &z);
    CHECK(y > SEA_LEVEL);
    CHECK(!blk_solid(world_get(&g_w1, 0, x, y, z)));
    CHECK(blk_solid(world_get(&g_w1, 0, x, y - 1, z)));
}

static void t_rle(void)
{
    static cl_u8 src[2048], enc[4096], dec[2048];
    int i, n;
    for (i = 0; i < 2048; ++i) {
        src[i] = (cl_u8)(i < 1000 ? 3 : ((i / 8) % 7));
    }
    n = store_rle_encode(src, 2048, enc, 4096);
    CHECK(n > 0 && n < 2048);
    CHECK(store_rle_decode(enc, n, dec, 2048) == 2048);
    CHECK(memcmp(src, dec, 2048) == 0);
    CHECK(store_rle_decode(enc, n - 2, dec, 2048) < 0);  /* truncated data is rejected */
}

static void t_store_persistence(void)
{
    /* modified chunks survive cache eviction and a full save + reopen */
    static World w;
    char dir[64];
    long i;
    cl_u8 glass = (cl_u8)block_find("glass");
    sprintf(dir, "tw_store");
    plat_native()->fs.make_dir(dir);
    remove("tw_store/chunks.dat");
    CHECK(world_open(&w, 99UL, dir));
    w.replace = plat_native()->fs.replace;
    world_set(&w, 0, 5, 80, 5, glass, META_PLACED);
    world_set(&w, 0, -40, 3, -40, BLOCK_AIR, 0);
    for (i = 0; i < WORLD_CHUNK_CACHE + 50; ++i) {
        world_get(&w, 0, 1000 + i * 32L, 70, 0);   /* force eviction */
    }
    CHECK(w.stats.written >= 1);
    CHECK(world_get(&w, 0, 5, 80, 5) == glass);    /* read back from the journal */
    CHECK(world_save(&w));
    world_close(&w);
    CHECK(world_open(&w, 99UL, dir));
    CHECK(world_get(&w, 0, 5, 80, 5) == glass);
    CHECK(world_get_meta(&w, 0, 5, 80, 5) == META_PLACED);
    CHECK(world_get(&w, 0, -40, 3, -40) == BLOCK_AIR);
    CHECK(world_get(&w, 0, 6, 80, 5) != glass);
    CHECK(w.store.count == 2);                     /* only changed chunks are stored */
    world_close(&w);
}

void suite_world(void)
{
    tf_run("seeded generation is deterministic", t_determinism);
    tf_run("chunk borders are seamless", t_chunk_boundary);
    tf_run("block set / get", t_set_get);
    tf_run("chunk cache stays bounded", t_cache_bounded);
    tf_run("terrain layers, sea, bedrock, trees", t_terrain_rules);
    tf_run("biome distribution", t_biomes);
    tf_run("ore generation", t_ores);
    tf_run("cave connectivity", t_cave_connectivity);
    tf_run("spawn point on dry land", t_spawn);
    tf_run("chunk RLE", t_rle);
    tf_run("modified chunks persist", t_store_persistence);
    world_close(&g_w1);
}
