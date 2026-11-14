/* Cliderness - world/structures/structure.c */
#include <stdio.h>
#include <string.h>
#include "world/structures/structure.h"
#include "world/world.h"
#include "content/content.h"
#include "localization/locale.h"
#include "core/ini.h"
#include "core/mathx.h"
#include "core/rng.h"
#include "core/str.h"

StructDef g_structs[MAX_STRUCTS];
int g_struct_count = 0;
LootDef g_loot[MAX_LOOT];
int g_loot_count = 0;

typedef struct SCtx {
    int cur, is_loot;
    char *err;
    size_t errn;
} SCtx;

static int serr(SCtx *c, const char *what, const char *v, int line)
{
    char num[12];
    str_from_long(num, (long)line);
    str_copy(c->err, "structures.def:", c->errn);
    str_cat(c->err, num, c->errn);
    str_cat(c->err, ": bad ", c->errn);
    str_cat(c->err, what, c->errn);
    str_cat(c->err, " ", c->errn);
    str_cat(c->err, v ? v : "", c->errn);
    return 1;
}

static int blk(const char *v, cl_u8 *out)
{
    int b = block_find(v);
    if (b < 0) {
        return 0;
    }
    *out = (cl_u8)b;
    return 1;
}

static int scb(void *user, const char *type, const char *id, const char *key, const char *v, int line)
{
    SCtx *c = (SCtx *)user;
    long n;
    if (!key) {
        if (str_ieq(type, "structure")) {
            StructDef *s;
            if (g_struct_count >= MAX_STRUCTS) return serr(c, "count", id, line);
            c->cur = g_struct_count++;
            c->is_loot = 0;
            s = &g_structs[c->cur];
            memset(s, 0, sizeof(*s));
            str_copy(s->id, id, sizeof(s->id));
            str_copy(s->name, id, sizeof(s->name));
            s->rarity = 20;
            s->ymin = 20;
            s->ymax = 40;
            s->length = 32;
            s->wall = (cl_u8)g_cb.cobble;
            s->support = (cl_u8)g_cb.stone;
            return 0;
        }
        if (str_ieq(type, "loot")) {
            if (g_loot_count >= MAX_LOOT) return serr(c, "count", id, line);
            c->cur = g_loot_count++;
            c->is_loot = 1;
            memset(&g_loot[c->cur], 0, sizeof(LootDef));
            str_copy(g_loot[c->cur].id, id, sizeof(g_loot[c->cur].id));
            return 0;
        }
        return serr(c, "section", type, line);
    }
    if (c->is_loot) {
        /* item = min-max:chance */
        LootDef *l = &g_loot[c->cur];
        long lo, hi, ch;
        int it = item_find(key);
        if (it < 0 || l->n >= LOOT_ENTRIES || sscanf(v, "%ld-%ld:%ld", &lo, &hi, &ch) != 3 ||
            lo < 0 || hi < lo || hi > 64 || ch < 1 || ch > 100) {
            return serr(c, "loot entry", key, line);
        }
        l->item[l->n] = (cl_s16)it;
        l->lo[l->n] = (cl_u8)lo;
        l->hi[l->n] = (cl_u8)hi;
        l->chance[l->n] = (cl_u8)ch;
        ++l->n;
        return 0;
    }
    {
        StructDef *s = &g_structs[c->cur];
        if (str_ieq(key, "name")) {
            str_copy(s->name, v, sizeof(s->name));
        } else if (str_ieq(key, "type")) {
            static const char *const t[ST_TYPE_COUNT] = { "mine", "ruins", "city" };
            int k;
            for (k = 0; k < ST_TYPE_COUNT && !str_ieq(v, t[k]); ++k) {
            }
            if (k == ST_TYPE_COUNT) return serr(c, "type", v, line);
            s->type = (cl_u8)k;
        } else if (str_ieq(key, "dimension")) {
            s->dim = (cl_u8)(str_ieq(v, "ancient") ? DIM_ANCIENT : DIM_OVERWORLD);
        } else if (str_ieq(key, "wall")) {
            if (!blk(v, &s->wall)) return serr(c, "block", v, line);
        } else if (str_ieq(key, "support")) {
            if (!blk(v, &s->support)) return serr(c, "block", v, line);
        } else if (str_ieq(key, "light")) {
            if (!blk(v, &s->light)) return serr(c, "block", v, line);
        } else if (str_ieq(key, "crate")) {
            if (!blk(v, &s->crate)) return serr(c, "block", v, line);
        } else if (str_ieq(key, "floor")) {
            if (!blk(v, &s->floor)) return serr(c, "block", v, line);
        } else if (str_ieq(key, "y")) {
            long a, b;
            if (sscanf(v, "%ld-%ld", &a, &b) != 2 || a < 4 || b < a || b > 120) return serr(c, "y", v, line);
            s->ymin = (cl_u8)a;
            s->ymax = (cl_u8)b;
        } else if (str_to_long(v, &n) && (str_ieq(key, "rarity") || str_ieq(key, "length"))) {
            if (str_ieq(key, "rarity")) s->rarity = (cl_u8)CL_CLAMP(n, 0L, 100L);
            else s->length = (cl_u8)CL_CLAMP(n, 4L, 60L);
        } else {
            return serr(c, "key", key, line);
        }
    }
    return 0;
}

int structures_load(const char *root, char *err, size_t errn)
{
    char path[160];
    SCtx c;
    int r;
    g_struct_count = 0;
    g_loot_count = 0;
    memset(&c, 0, sizeof(c));
    c.err = err;
    c.errn = errn;
    str_copy(path, root, sizeof(path));
    str_cat(path, "/structs/structs.def", sizeof(path));
    r = ini_parse_file(path, scb, &c);
    if (r < 0) {
        str_copy(err, "cannot open ", errn);
        str_cat(err, path, errn);
        return 0;
    }
    return r == 0;
}

int struct_find(const char *id)
{
    int i;
    for (i = 0; i < g_struct_count; ++i) {
        if (str_ieq(g_structs[i].id, id)) return i;
    }
    return -1;
}

int loot_find(const char *id)
{
    int i;
    for (i = 0; i < g_loot_count; ++i) {
        if (str_ieq(g_loot[i].id, id)) return i;
    }
    return -1;
}

const char *struct_name(int s)
{
    char key[40];
    if (s < 0 || s >= g_struct_count) return "?";
    str_copy(key, "struct.", sizeof(key));
    str_cat(key, g_structs[s].id, sizeof(key));
    return loc_get_or(key, g_structs[s].name);
}

static long region_size(const StructDef *s)
{
    return s->type == ST_MINE ? 128L : 192L;
}

/* does region (rx, rz) contain structure s? centre returned */
static int region_has(cl_u32 seed, int si, long rx, long rz, long *cx, long *cy, long *cz)
{
    const StructDef *s = &g_structs[si];
    cl_u32 h = rng_hash3(CL_U32(seed + 0x5757UL + (cl_u32)si * 977UL), rx, 31, rz);
    long size = region_size(s);
    if ((long)(h % 100UL) >= (long)s->rarity) {
        return 0;
    }
    *cx = rx * size + size / 4 + (long)((h >> 8) % (cl_u32)(size / 2));
    *cz = rz * size + size / 4 + (long)((h >> 16) % (cl_u32)(size / 2));
    *cy = (long)s->ymin + (long)((h >> 24) % (cl_u32)(s->ymax - s->ymin + 1));
    return 1;
}

/* mine corridor geometry; returns block or -1 */
static int mine_cell(const StructDef *s, long cx, long cy, long cz, long x, int y, long z, int surface_h)
{
    long L = s->length, along = -1;
    int axis = -1;
    if (y < cy || y > cy + 2 || y >= surface_h - 4) {
        return -1;
    }
    if (z == cz && x >= cx - L && x <= cx + L) { axis = 0; along = x - cx; }
    else if (x == cx && z >= cz - L && z <= cz + L) { axis = 1; along = z - cz; }
    else if ((z == cz - 12 || z == cz + 12) && x >= cx - L / 2 && x <= cx + L / 2) { axis = 2; along = x - cx; }
    if (axis < 0) {
        return -1;
    }
    if (y == cy + 2) {
        return (mx_floor_mod(along, 6L) == 0) ? s->support : -1;
    }
    if (y == cy) {
        long a = mx_abs(along);
        if (s->crate && ((axis < 2 && a == L) || (axis == 2 && a == L / 2))) {
            return s->crate;
        }
        if (s->light && axis < 2 && mx_floor_mod(along, 12L) == 6) {
            return s->light;
        }
    }
    return BLOCK_AIR;
}

static int ruins_cell(cl_u32 seed, const StructDef *s, long cx, long cz, long x, int y, long z, int surface_h)
{
    long dx = mx_abs(x - cx), dz = mx_abs(z - cz);
    long r = s->length > 3 ? 3 : (long)s->length;
    if (dx > r || dz > r || y <= surface_h || y > surface_h + 2 || surface_h < SEA_LEVEL) {
        return -1;
    }
    if (dx == 0 && dz == 0 && y == surface_h + 1) {
        return s->crate ? s->crate : -1;
    }
    if (dx == r || dz == r) {
        /* broken walls: some stones are missing, more at the top */
        cl_u32 h = rng_hash3(seed, x, y, z);
        if ((long)(h % 100UL) < (y == surface_h + 2 ? 55L : 15L)) {
            return BLOCK_AIR;
        }
        return s->wall;
    }
    return BLOCK_AIR;
}

static int city_cell(cl_u32 seed, const StructDef *s, long cx, long cz, long x, int y, long z, int surface_h)
{
    long dx = x - cx, dz = z - cz;
    long R = s->length;
    long gx, gz, lx, lz;
    if (mx_abs(dx) > R || mx_abs(dz) > R || y <= surface_h || y > surface_h + 4) {
        return -1;
    }
    /* outer wall */
    if (mx_abs(dx) == R || mx_abs(dz) == R) {
        if ((dx == 0 || dz == 0) && y <= surface_h + 2) {
            return BLOCK_AIR;   /* gates */
        }
        return s->wall;
    }
    /* 9x9 blocks: 7x7 houses with a 2-wide street */
    gx = mx_floor_mod(dx + R, 9L);
    gz = mx_floor_mod(dz + R, 9L);
    lx = gx - 1;
    lz = gz - 1;
    if (gx == 0 || gz == 0 || gx == 8 || gz == 8) {
        return y == surface_h + 1 ? BLOCK_AIR : -1;   /* streets */
    }
    {
        cl_u32 h = rng_hash3(seed, mx_floor_div(dx + R, 9L), 7, mx_floor_div(dz + R, 9L));
        int ruined = (h % 3UL) == 0UL;
        int edge = lx == 0 || lz == 0 || lx == 6 || lz == 6;
        if (lx == 3 && lz == 3 && y == surface_h + 1 && s->crate && (h % 4UL) == 1UL) {
            return s->crate;
        }
        if (edge) {
            if (lx == 3 && lz == 0 && y <= surface_h + 2) {
                return BLOCK_AIR;   /* door */
            }
            if (y == surface_h + 4) {
                return ruined ? BLOCK_AIR : s->support;
            }
            if (ruined && (rng_hash3(seed, x, y, z) % 100UL) < 35UL) {
                return BLOCK_AIR;
            }
            return s->wall;
        }
        if (y == surface_h + 4 && !ruined) {
            return s->support;   /* roof */
        }
        return BLOCK_AIR;
    }
}

int structure_block(cl_u32 seed, int dim, int surface_h, long x, int y, long z)
{
    int si;
    for (si = 0; si < g_struct_count; ++si) {
        const StructDef *s = &g_structs[si];
        long size = region_size(s);
        long rx0 = mx_floor_div(x, size), rz0 = mx_floor_div(z, size);
        long rx, rz;
        if (s->dim != dim || s->rarity == 0) {
            continue;
        }
        if (s->type == ST_MINE && (y < s->ymin || y > s->ymax + 2)) {
            continue;
        }
        if (s->type != ST_MINE && (y <= surface_h || y > surface_h + 4)) {
            continue;
        }
        for (rz = rz0 - 1; rz <= rz0 + 1; ++rz) {
            for (rx = rx0 - 1; rx <= rx0 + 1; ++rx) {
                long cx, cy, cz;
                int b = -1;
                if (!region_has(seed, si, rx, rz, &cx, &cy, &cz)) {
                    continue;
                }
                if (s->type == ST_MINE) {
                    b = mine_cell(s, cx, cy, cz, x, y, z, surface_h);
                } else if (s->type == ST_RUINS) {
                    b = ruins_cell(seed, s, cx, cz, x, y, z, surface_h);
                } else {
                    b = city_cell(seed, s, cx, cz, x, y, z, surface_h);
                }
                if (b >= 0) {
                    return b;
                }
            }
        }
    }
    return -1;
}

int structure_locate(cl_u32 seed, int si, long x, long z, long *ox, long *oy, long *oz)
{
    long size, rx0, rz0, r, rx, rz, best = -1;
    if (si < 0 || si >= g_struct_count) {
        return 0;
    }
    size = region_size(&g_structs[si]);
    rx0 = mx_floor_div(x, size);
    rz0 = mx_floor_div(z, size);
    for (r = 0; r <= 24 && best < 0; ++r) {
        for (rz = rz0 - r; rz <= rz0 + r; ++rz) {
            for (rx = rx0 - r; rx <= rx0 + r; ++rx) {
                long cx, cy, cz, d;
                if (mx_abs(rx - rx0) != r && mx_abs(rz - rz0) != r) {
                    continue;
                }
                if (!region_has(seed, si, rx, rz, &cx, &cy, &cz)) {
                    continue;
                }
                d = (cx - x) * (cx - x) + (cz - z) * (cz - z);
                if (best < 0 || d < best) {
                    best = d;
                    *ox = cx;
                    *oy = cy;
                    *oz = cz;
                }
            }
        }
    }
    return best >= 0;
}
