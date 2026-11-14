/* Cliderness - world/biomes/biome.c */
#include <stdio.h>
#include <string.h>
#include "world/biomes/biome.h"
#include "content/content.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/ini.h"
#include "core/str.h"

BiomeDef g_biomes[MAX_BIOMES];
int g_biome_count = 0;

typedef struct BCtx {
    int cur;
    char *err;
    size_t errn;
    const char *file;
} BCtx;

static int berr(BCtx *c, const char *msg, const char *what, int line)
{
    char num[12];
    str_from_long(num, (long)line);
    str_copy(c->err, c->file, c->errn);
    str_cat(c->err, ":", c->errn);
    str_cat(c->err, num, c->errn);
    str_cat(c->err, ": ", c->errn);
    str_cat(c->err, msg, c->errn);
    str_cat(c->err, " ", c->errn);
    str_cat(c->err, what, c->errn);
    return 1;
}

static int range(const char *v, cl_s16 *lo, cl_s16 *hi)
{
    char buf[32];
    char *sep;
    long a, b;
    str_copy(buf, v, sizeof(buf));
    sep = strstr(buf, "..");
    if (!sep) {
        return 0;
    }
    *sep = '\0';
    if (!str_to_long(buf, &a) || !str_to_long(sep + 2, &b)) {
        return 0;
    }
    *lo = (cl_s16)a;
    *hi = (cl_s16)b;
    return 1;
}

static int blk(const char *v, cl_u8 *out)
{
    int b;
    if (str_ieq(v, "none")) {
        *out = 0;
        return 1;
    }
    b = block_find(v);
    if (b < 0) {
        return 0;
    }
    *out = (cl_u8)b;
    return 1;
}

static int biome_cb(void *user, const char *type, const char *id, const char *key,
                    const char *value, int line)
{
    BCtx *c = (BCtx *)user;
    BiomeDef *b;
    long n;
    if (!str_ieq(type, "biome")) {
        return berr(c, "unknown section", type, line);
    }
    if (!key) {
        if (g_biome_count >= MAX_BIOMES) {
            return berr(c, "too many biomes", id, line);
        }
        c->cur = g_biome_count++;
        b = &g_biomes[c->cur];
        memset(b, 0, sizeof(*b));
        str_copy(b->id, id, sizeof(b->id));
        str_copy(b->name, id, sizeof(b->name));
        b->h_lo = 0; b->h_hi = 255;
        b->t_lo = 0; b->t_hi = 100;
        b->m_lo = 0; b->m_hi = 100;
        b->s_lo = 0; b->s_hi = 100;
        b->base_temp = 18;
        b->humidity = 50;
        b->surface = (cl_u8)g_cb.grass;
        b->subsurface = (cl_u8)g_cb.dirt;
        b->seabed = (cl_u8)g_cb.sand;
        b->stone = (cl_u8)g_cb.stone;
        b->map_color = C_GREEN;
        return 0;
    }
    b = &g_biomes[c->cur];
    if (str_ieq(key, "name")) {
        str_copy(b->name, value, sizeof(b->name));
    } else if (str_ieq(key, "kind")) {
        static const char *const kinds[BK_COUNT] = { "land", "coast", "ocean", "cave", "ancient" };
        int k;
        for (k = 0; k < BK_COUNT; ++k) {
            if (str_ieq(value, kinds[k])) {
                break;
            }
        }
        if (k == BK_COUNT) return berr(c, "bad kind", value, line);
        b->kind = (cl_u8)k;
    } else if (str_ieq(key, "height")) {
        if (!range(value, &b->h_lo, &b->h_hi)) return berr(c, "bad range", value, line);
    } else if (str_ieq(key, "temp")) {
        if (!range(value, &b->t_lo, &b->t_hi)) return berr(c, "bad range", value, line);
    } else if (str_ieq(key, "humid")) {
        if (!range(value, &b->m_lo, &b->m_hi)) return berr(c, "bad range", value, line);
    } else if (str_ieq(key, "special")) {
        if (!range(value, &b->s_lo, &b->s_hi)) return berr(c, "bad range", value, line);
    } else if (str_ieq(key, "base_temp") || str_ieq(key, "humidity") ||
               str_ieq(key, "tree_density") || str_ieq(key, "plant_density")) {
        if (!str_to_long(value, &n) || n < -100 || n > 1000) return berr(c, "bad number", value, line);
        if (str_ieq(key, "base_temp")) b->base_temp = (cl_s16)n;
        else if (str_ieq(key, "humidity")) b->humidity = (cl_s16)n;
        else if (str_ieq(key, "tree_density")) b->tree_density = (cl_u16)n;
        else b->plant_density = (cl_u16)n;
    } else if (str_ieq(key, "freeze")) {
        b->freeze = (cl_u8)(value[0] == '1');
    } else if (str_ieq(key, "map_color")) {
        int col = color_from_name(value);
        if (col < 0) return berr(c, "bad colour", value, line);
        b->map_color = (cl_u8)col;
    } else {
        cl_u8 *dst = NULL;
        if (str_ieq(key, "surface")) dst = &b->surface;
        else if (str_ieq(key, "subsurface")) dst = &b->subsurface;
        else if (str_ieq(key, "seabed")) dst = &b->seabed;
        else if (str_ieq(key, "stone")) dst = &b->stone;
        else if (str_ieq(key, "tree")) dst = &b->tree;
        else if (str_ieq(key, "tree_top")) dst = &b->tree_top;
        else if (str_ieq(key, "tree2")) dst = &b->tree2;
        else if (str_ieq(key, "plant")) dst = &b->plant;
        else if (str_ieq(key, "plant2")) dst = &b->plant2;
        else if (str_ieq(key, "plant3")) dst = &b->plant3;
        if (!dst) return berr(c, "unknown key", key, line);
        if (!blk(value, dst)) return berr(c, "unknown block", value, line);
    }
    return 0;
}

int biome_load(const char *root, char *err, size_t errn)
{
    char path[160];
    char line[64];
    FILE *f;
    BCtx c;
    g_biome_count = 0;
    c.cur = 0;
    c.err = err;
    c.errn = errn;
    c.file = "";
    str_copy(path, root, sizeof(path));
    str_cat(path, "/biomes/index.lst", sizeof(path));
    f = fopen(path, "r");
    if (!f) {
        str_copy(err, "cannot open ", errn);
        str_cat(err, path, errn);
        return 0;
    }
    while (fgets(line, sizeof(line), f)) {
        char *s = str_trim(line);
        char fp[160];
        int r;
        if (!*s || *s == '#') {
            continue;
        }
        str_copy(fp, root, sizeof(fp));
        str_cat(fp, "/biomes/", sizeof(fp));
        str_cat(fp, s, sizeof(fp));
        c.file = s;
        r = ini_parse_file(fp, biome_cb, &c);
        if (r != 0) {
            if (r < 0) {
                str_copy(err, "cannot open ", errn);
                str_cat(err, fp, errn);
            }
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    if (g_biome_count == 0) {
        str_copy(err, "no biomes defined", errn);
        return 0;
    }
    return 1;
}

int biome_find(const char *id)
{
    int i;
    for (i = 0; i < g_biome_count; ++i) {
        if (str_ieq(g_biomes[i].id, id)) {
            return i;
        }
    }
    return -1;
}

int biome_select(int kind_hint, int height, int temp, int humid, int special)
{
    int i;
    int fallback = -1;
    for (i = 0; i < g_biome_count; ++i) {
        const BiomeDef *b = &g_biomes[i];
        if (b->kind != kind_hint) {
            continue;
        }
        if (fallback < 0) {
            fallback = i;
        }
        if (height < b->h_lo || height > b->h_hi) continue;
        if (temp < b->t_lo || temp > b->t_hi) continue;
        if (humid < b->m_lo || humid > b->m_hi) continue;
        if (special < b->s_lo || special > b->s_hi) continue;
        return i;
    }
    return fallback >= 0 ? fallback : 0;
}

const char *biome_name(int b)
{
    char key[40];
    if (b < 0 || b >= g_biome_count) {
        return "?";
    }
    str_copy(key, "biome.", sizeof(key));
    str_cat(key, g_biomes[b].id, sizeof(key));
    return loc_get_or(key, g_biomes[b].name);
}
