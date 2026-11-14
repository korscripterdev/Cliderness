/* Cliderness - world/chunks/chunkstore.c */
#include <stdio.h>
#include <string.h>
#include "world/chunks/chunkstore.h"
#include "core/crc.h"
#include "core/mem.h"
#include "core/str.h"

#define REC_HDR 20
#define RAW_LEN 2048
#define RLE_CAP 4096

#ifdef CLIDE_TARGET_DOS16
#define STORE_MAX 2048
#else
#define STORE_MAX 60000
#endif

static cl_u8 g_raw[RAW_LEN];
static cl_u8 g_rle[RLE_CAP];

static void put16(cl_u8 *p, unsigned v) { p[0] = (cl_u8)(v & 255); p[1] = (cl_u8)((v >> 8) & 255); }
static void put32(cl_u8 *p, cl_u32 v)
{
    p[0] = (cl_u8)(v & 255UL); p[1] = (cl_u8)((v >> 8) & 255UL);
    p[2] = (cl_u8)((v >> 16) & 255UL); p[3] = (cl_u8)((v >> 24) & 255UL);
}
static unsigned get16(const cl_u8 *p) { return (unsigned)p[0] | ((unsigned)p[1] << 8); }
static cl_u32 get32(const cl_u8 *p)
{
    return (cl_u32)p[0] | ((cl_u32)p[1] << 8) | ((cl_u32)p[2] << 16) | ((cl_u32)p[3] << 24);
}
static long s32(cl_u32 v)
{
    /* two's complement 32-bit -> long, portable */
    if (v & 0x80000000UL) {
        return -(long)((~v & 0x7FFFFFFFUL)) - 1L;
    }
    return (long)v;
}

int store_rle_encode(const cl_u8 *src, int n, cl_u8 *dst, int cap)
{
    int i = 0, o = 0;
    while (i < n) {
        cl_u8 v = src[i];
        int run = 1;
        while (i + run < n && src[i + run] == v && run < 255) {
            ++run;
        }
        if (o + 2 > cap) {
            return -1;
        }
        dst[o++] = (cl_u8)run;
        dst[o++] = v;
        i += run;
    }
    return o;
}

int store_rle_decode(const cl_u8 *src, int len, cl_u8 *dst, int n)
{
    int i = 0, o = 0;
    while (i + 1 < len) {
        int run = src[i];
        cl_u8 v = src[i + 1];
        i += 2;
        if (run == 0 || o + run > n) {
            return -1;
        }
        while (run--) {
            dst[o++] = v;
        }
    }
    return o == n ? n : -1;
}

static int grow(ChunkStore *s)
{
    StoreEntry *ne;
    int ncap;
    if (s->count < s->cap) {
        return 1;
    }
    if (s->cap >= s->max) {
        return 0;
    }
    ncap = s->cap ? s->cap * 2 : 128;
    if (ncap > s->max) {
        ncap = s->max;
    }
    ne = (StoreEntry *)mem_alloc(sizeof(StoreEntry) * (size_t)ncap);
    if (!ne) {
        return 0;
    }
    if (s->e) {
        memcpy(ne, s->e, sizeof(StoreEntry) * (size_t)s->count);
        mem_free(s->e);
    }
    s->e = ne;
    s->cap = ncap;
    return 1;
}

int store_find(const ChunkStore *s, int dim, long cx, int y, long cz)
{
    int i;
    for (i = s->count - 1; i >= 0; --i) {
        const StoreEntry *e = &s->e[i];
        if (e->cx == cx && e->cz == cz && e->y == y && e->dim == dim) {
            return i;
        }
    }
    return -1;
}

static int set_entry(ChunkStore *s, int dim, long cx, int y, long cz, int file, long off)
{
    int i = store_find(s, dim, cx, y, cz);
    if (i < 0) {
        if (!grow(s)) {
            return 0;
        }
        i = s->count++;
        s->e[i].cx = cx;
        s->e[i].cz = cz;
        s->e[i].y = (cl_s16)y;
        s->e[i].dim = (cl_u8)dim;
    }
    s->e[i].file = (cl_u8)file;
    s->e[i].offset = off;
    return 1;
}

static void join(char *out, const char *dir, const char *name)
{
    str_copy(out, dir, 160);
    str_cat(out, "/", 160);
    str_cat(out, name, 160);
}

int store_open(ChunkStore *s, const char *dir)
{
    cl_u8 hdr[REC_HDR];
    memset(s, 0, sizeof(*s));
    s->max = STORE_MAX;
    join(s->main_path, dir, "chunks.dat");
    join(s->jnl_path, dir, "chunks.jnl");
    join(s->new_path, dir, "chunks.new");
    remove(s->jnl_path);
    s->mainf = fopen(s->main_path, "rb");
    if (s->mainf) {
        if (fread(hdr, 1, 12, s->mainf) != 12 || memcmp(hdr, "CLCHUNK1", 8) != 0) {
            fclose(s->mainf);
            s->mainf = NULL;
            ++s->errors;
            return 0;
        }
        for (;;) {
            long off = ftell(s->mainf);
            unsigned len;
            if (fread(hdr, 1, REC_HDR, s->mainf) != REC_HDR) {
                break;
            }
            if (hdr[0] != 'C' || hdr[1] != 'K') {
                ++s->errors;
                break;
            }
            len = get16(hdr + 14);
            if (!set_entry(s, hdr[2], s32(get32(hdr + 6)), (int)(cl_s16)get16(hdr + 4),
                           s32(get32(hdr + 10)), 0, off)) {
                ++s->errors;
                break;
            }
            if (fseek(s->mainf, (long)len, SEEK_CUR) != 0) {
                break;
            }
        }
    }
    return 1;
}

void store_close(ChunkStore *s)
{
    if (s->mainf) {
        fclose(s->mainf);
    }
    if (s->jnl) {
        fclose(s->jnl);
        remove(s->jnl_path);
    }
    mem_free(s->e);
    memset(s, 0, sizeof(*s));
}

static int read_record(FILE *f, long off, int *dim, long *cx, int *y, long *cz, cl_u8 *raw)
{
    cl_u8 hdr[REC_HDR];
    unsigned len;
    cl_u32 crc;
    if (!f || fseek(f, off, SEEK_SET) != 0 || fread(hdr, 1, REC_HDR, f) != REC_HDR) {
        return -1;
    }
    if (hdr[0] != 'C' || hdr[1] != 'K') {
        return -1;
    }
    len = get16(hdr + 14);
    if (len > RLE_CAP || fread(g_rle, 1, len, f) != len) {
        return -1;
    }
    crc = crc32_final(crc32_update(CRC32_INIT, g_rle, len));
    if (crc != get32(hdr + 16)) {
        return -1;
    }
    *dim = hdr[2];
    *y = (int)(cl_s16)get16(hdr + 4);
    *cx = s32(get32(hdr + 6));
    *cz = s32(get32(hdr + 10));
    if (store_rle_decode(g_rle, (int)len, raw, RAW_LEN) != RAW_LEN) {
        return -1;
    }
    return 1;
}

int store_read(ChunkStore *s, int dim, long cx, int y, long cz, cl_u8 *blocks, cl_u8 *meta)
{
    int i = store_find(s, dim, cx, y, cz);
    int d, yy;
    long x, z;
    if (i < 0) {
        return 0;
    }
    if (s->e[i].file == 1 && s->jnl) {
        fflush(s->jnl);
    }
    if (read_record(s->e[i].file ? s->jnl : s->mainf, s->e[i].offset, &d, &x, &yy, &z, g_raw) != 1 ||
        d != dim || x != cx || yy != y || z != cz) {
        ++s->errors;
        return -1;
    }
    memcpy(blocks, g_raw, 1024);
    memcpy(meta, g_raw + 1024, 1024);
    return 1;
}

static int write_record(FILE *f, int dim, long cx, int y, long cz, const cl_u8 *raw)
{
    cl_u8 hdr[REC_HDR];
    int len = store_rle_encode(raw, RAW_LEN, g_rle, RLE_CAP);
    if (len < 0) {
        return 0;
    }
    hdr[0] = 'C';
    hdr[1] = 'K';
    hdr[2] = (cl_u8)dim;
    hdr[3] = 0;
    put16(hdr + 4, (unsigned)(y & 0xFFFF));
    put32(hdr + 6, CL_U32((cl_u32)cx));
    put32(hdr + 10, CL_U32((cl_u32)cz));
    put16(hdr + 14, (unsigned)len);
    put32(hdr + 16, crc32_final(crc32_update(CRC32_INIT, g_rle, (size_t)len)));
    if (fwrite(hdr, 1, REC_HDR, f) != REC_HDR || fwrite(g_rle, 1, (size_t)len, f) != (size_t)len) {
        return 0;
    }
    return 1;
}

int store_write(ChunkStore *s, int dim, long cx, int y, long cz, const cl_u8 *blocks, const cl_u8 *meta)
{
    static cl_u8 raw[RAW_LEN];
    long off;
    if (!s->jnl) {
        s->jnl = fopen(s->jnl_path, "w+b");
        if (!s->jnl) {
            ++s->errors;
            return 0;
        }
    }
    memcpy(raw, blocks, 1024);
    memcpy(raw + 1024, meta, 1024);
    fseek(s->jnl, 0L, SEEK_END);
    off = ftell(s->jnl);
    if (!write_record(s->jnl, dim, cx, y, cz, raw)) {
        ++s->errors;
        return 0;
    }
    if (!set_entry(s, dim, cx, y, cz, 1, off)) {
        ++s->errors;
        return 0;
    }
    return 1;
}

static int fallback_replace(const char *tmp, const char *dst)
{
    remove(dst);
    return rename(tmp, dst) == 0 ? 0 : -1;
}

int store_commit(ChunkStore *s, int (*replace)(const char *, const char *))
{
    static cl_u8 raw[RAW_LEN];
    cl_u8 hdr[12];
    FILE *nf;
    int i, ok = 1;
    long *offs;
    if (!replace) {
        replace = fallback_replace;
    }
    if (s->jnl) {
        fflush(s->jnl);
    }
    offs = (long *)mem_alloc(sizeof(long) * (size_t)(s->count ? s->count : 1));
    if (!offs) {
        return 0;
    }
    nf = fopen(s->new_path, "wb");
    if (!nf) {
        mem_free(offs);
        return 0;
    }
    memcpy(hdr, "CLCHUNK1", 8);
    put32(hdr + 8, (cl_u32)s->count);
    fwrite(hdr, 1, 12, nf);
    for (i = 0; ok && i < s->count; ++i) {
        StoreEntry *e = &s->e[i];
        int d, y;
        long cx, cz;
        if (read_record(e->file ? s->jnl : s->mainf, e->offset, &d, &cx, &y, &cz, raw) != 1) {
            ok = 0;
            break;
        }
        offs[i] = ftell(nf);
        if (!write_record(nf, d, cx, y, cz, raw)) {
            ok = 0;
        }
    }
    if (fclose(nf) != 0) {
        ok = 0;
    }
    /* verify the new file completely before replacing the old one */
    if (ok) {
        FILE *vf = fopen(s->new_path, "rb");
        if (!vf) {
            ok = 0;
        }
        for (i = 0; ok && i < s->count; ++i) {
            int d, y;
            long cx, cz;
            if (read_record(vf, offs[i], &d, &cx, &y, &cz, raw) != 1 ||
                cx != s->e[i].cx || cz != s->e[i].cz || y != s->e[i].y) {
                ok = 0;
            }
        }
        if (vf) {
            fclose(vf);
        }
    }
    if (!ok) {
        remove(s->new_path);
        mem_free(offs);
        ++s->errors;
        return 0;
    }
    if (s->mainf) {
        fclose(s->mainf);
        s->mainf = NULL;
    }
    if (replace(s->new_path, s->main_path) != 0) {
        mem_free(offs);
        s->mainf = fopen(s->main_path, "rb");
        ++s->errors;
        return 0;
    }
    s->mainf = fopen(s->main_path, "rb");
    for (i = 0; i < s->count; ++i) {
        s->e[i].file = 0;
        s->e[i].offset = offs[i];
    }
    mem_free(offs);
    if (s->jnl) {
        fclose(s->jnl);
        s->jnl = NULL;
        remove(s->jnl_path);
    }
    return 1;
}
