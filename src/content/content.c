/* Cliderness - content/content.c */
#include <stdio.h>
#include <string.h>
#include "content/content.h"
#include "core/ini.h"
#include "core/str.h"
#include "core/mem.h"
#include "renderer/fb.h"
#include "localization/locale.h"

char g_data_root[128] = "data";
BlockDef g_blocks[MAX_BLOCKS];
ItemDef  g_items[MAX_ITEMS];
int g_block_count = 0;
int g_item_count = 0;
CoreBlocks g_cb;

typedef struct BlockExtra {
    char  drop[ID_LEN];
    char  host[ID_LEN];
    cl_s16 weight;
    cl_s16 density;
    cl_u8 make_item;
} BlockExtra;

typedef struct LoadCtx {
    BlockExtra *extra;
    int cur;
    char *err;
    size_t errn;
    const char *file;
} LoadCtx;

static const char *const g_item_types[ITEM_TYPE_COUNT] = {
    "block", "tool", "weapon", "armor", "food", "resource", "material",
    "consumable", "building", "relic"
};

static const char *const g_tools[TOOL_COUNT] = {
    "none", "pickaxe", "axe", "shovel", "hoe", "sword"
};

static const char *strip_ns(const char *id)
{
    const char *c = strchr(id, ':');
    return c ? c + 1 : id;
}

int block_find(const char *id)
{
    int i;
    id = strip_ns(id);
    for (i = 0; i < g_block_count; ++i) {
        if (str_ieq(g_blocks[i].id, id)) {
            return i;
        }
    }
    return -1;
}

int item_find(const char *id)
{
    int i;
    id = strip_ns(id);
    for (i = 0; i < g_item_count; ++i) {
        if (str_ieq(g_items[i].id, id)) {
            return i;
        }
    }
    return -1;
}

const char *block_name(int b)
{
    char key[ID_LEN + 8];
    if (b < 0 || b >= g_block_count) {
        return "?";
    }
    str_copy(key, "block.", sizeof(key));
    str_cat(key, g_blocks[b].id, sizeof(key));
    return loc_get_or(key, g_blocks[b].name);
}

const char *item_name(int i)
{
    char key[ID_LEN + 8];
    if (i < 0 || i >= g_item_count) {
        return "?";
    }
    str_copy(key, g_items[i].place_block >= 0 && g_items[i].type == ITEM_BLOCK ? "block." : "item.",
             sizeof(key));
    str_cat(key, g_items[i].id, sizeof(key));
    return loc_get_or(key, g_items[i].name);
}

const char *item_type_name(int t)
{
    return (t >= 0 && t < ITEM_TYPE_COUNT) ? g_item_types[t] : "?";
}

static int parse_glyph(const char *v, cl_u8 *out)
{
    long n;
    if (v[0] == '\'' && v[1] && v[2] == '\'') {
        *out = (cl_u8)v[1];
        return 1;
    }
    if (str_to_long(v, &n) && n >= 0 && n <= 255) {
        *out = (cl_u8)n;
        return 1;
    }
    if (v[0] && !v[1]) {
        *out = (cl_u8)v[0];
        return 1;
    }
    return 0;
}

static int parse_color(const char *v, cl_u8 *out)
{
    int c = color_from_name(v);
    if (c < 0) {
        return 0;
    }
    *out = (cl_u8)c;
    return 1;
}

static int find_name(const char *const *names, int n, const char *v)
{
    int i;
    for (i = 0; i < n; ++i) {
        if (str_ieq(names[i], v)) {
            return i;
        }
    }
    return -1;
}

static int set_err(LoadCtx *c, const char *msg, const char *detail, int line)
{
    char buf[160];
    char det[48];
    char num[16];
    const char *args[4];
    str_copy(det, detail ? detail : "", sizeof(det));
    str_from_long(num, (long)line);
    args[0] = c->file;
    args[1] = num;
    args[2] = msg;
    args[3] = det;
    str_subst(buf, sizeof(buf), "{0}:{1}: {2} {3}", args, 4);
    str_copy(c->err, buf, c->errn);
    return 1;
}

static cl_u16 parse_flags(const char *v)
{
    static const char *const names[] = {
        "solid", "liquid", "plant", "slow", "hurt", "replaceable", "nomine", "light", "ancient"
    };
    cl_u16 f = 0;
    char buf[160];
    char *tok;
    str_copy(buf, v, sizeof(buf));
    tok = strtok(buf, ", ");
    while (tok) {
        int i = find_name(names, (int)CL_COUNTOF(names), tok);
        if (i >= 0) {
            f |= (cl_u16)(1u << i);
        }
        tok = strtok(NULL, ", ");
    }
    return f;
}

static int block_cb(void *user, const char *type, const char *id, const char *key,
                    const char *value, int line)
{
    LoadCtx *c = (LoadCtx *)user;
    BlockDef *b;
    long n;
    if (!str_ieq(type, "block")) {
        return set_err(c, "unknown section", type, line);
    }
    if (!key) {
        if (block_find(id) >= 0) {
            return set_err(c, "duplicate block", id, line);
        }
        if (g_block_count >= MAX_BLOCKS) {
            return set_err(c, "too many blocks", id, line);
        }
        c->cur = g_block_count++;
        b = &g_blocks[c->cur];
        memset(b, 0, sizeof(*b));
        str_copy(b->id, id, sizeof(b->id));
        str_copy(b->name, id, sizeof(b->name));
        b->glyph = '?';
        b->fg = C_LIGHTGRAY;
        b->floor_glyph = 0;
        b->floor_fg = 255;
        b->floor_bg = 255;
        b->hardness = 10;
        b->drop_item = -1;
        b->drop_count = 1;
        memset(&c->extra[c->cur], 0, sizeof(BlockExtra));
        str_copy(c->extra[c->cur].drop, "self", ID_LEN);
        c->extra[c->cur].weight = 1000;
        c->extra[c->cur].density = 2000;
        c->extra[c->cur].make_item = 1;
        return 0;
    }
    b = &g_blocks[c->cur];
    if (str_ieq(key, "name")) {
        str_copy(b->name, value, sizeof(b->name));
    } else if (str_ieq(key, "glyph")) {
        if (!parse_glyph(value, &b->glyph)) return set_err(c, "bad glyph", value, line);
    } else if (str_ieq(key, "fg")) {
        if (!parse_color(value, &b->fg)) return set_err(c, "bad colour", value, line);
    } else if (str_ieq(key, "bg")) {
        if (!parse_color(value, &b->bg)) return set_err(c, "bad colour", value, line);
    } else if (str_ieq(key, "floor_glyph")) {
        if (!parse_glyph(value, &b->floor_glyph)) return set_err(c, "bad glyph", value, line);
    } else if (str_ieq(key, "floor_fg")) {
        if (!parse_color(value, &b->floor_fg)) return set_err(c, "bad colour", value, line);
    } else if (str_ieq(key, "floor_bg")) {
        if (!parse_color(value, &b->floor_bg)) return set_err(c, "bad colour", value, line);
    } else if (str_ieq(key, "flags")) {
        b->flags = parse_flags(value);
    } else if (str_ieq(key, "hardness")) {
        if (!str_to_long(value, &n) || n < 0 || n > 255) return set_err(c, "bad hardness", value, line);
        b->hardness = (cl_u8)n;
    } else if (str_ieq(key, "tool")) {
        int t = find_name(g_tools, TOOL_COUNT, value);
        if (t < 0) return set_err(c, "bad tool", value, line);
        b->tool = (cl_u8)t;
    } else if (str_ieq(key, "drop")) {
        str_copy(c->extra[c->cur].drop, value, ID_LEN);
    } else if (str_ieq(key, "drop_count")) {
        if (!str_to_long(value, &n) || n < 0 || n > 64) return set_err(c, "bad drop_count", value, line);
        b->drop_count = (cl_u8)n;
    } else if (str_ieq(key, "light")) {
        if (!str_to_long(value, &n) || n < 0 || n > 15) return set_err(c, "bad light", value, line);
        b->light = (cl_u8)n;
    } else if (str_ieq(key, "item")) {
        c->extra[c->cur].make_item = (cl_u8)(value[0] == '1');
    } else if (str_ieq(key, "weight")) {
        if (!str_to_long(value, &n) || n < 0 || n > 30000) return set_err(c, "bad weight", value, line);
        c->extra[c->cur].weight = (cl_s16)n;
    } else if (str_ieq(key, "density")) {
        if (!str_to_long(value, &n) || n < 0 || n > 30000) return set_err(c, "bad density", value, line);
        c->extra[c->cur].density = (cl_s16)n;
    } else if (str_ieq(key, "ore_chance")) {
        if (!str_to_long(value, &n) || n < 0 || n > 10000) return set_err(c, "bad ore_chance", value, line);
        b->ore_chance = (cl_u16)n;
    } else if (str_ieq(key, "ore_y")) {
        long lo, hi;
        char buf[32];
        char *dash;
        str_copy(buf, value, sizeof(buf));
        dash = strchr(buf, '-');
        if (!dash) return set_err(c, "bad ore_y", value, line);
        *dash = '\0';
        if (!str_to_long(buf, &lo) || !str_to_long(dash + 1, &hi)) return set_err(c, "bad ore_y", value, line);
        b->ore_ymin = (cl_u8)CL_CLAMP(lo, 0L, 255L);
        b->ore_ymax = (cl_u8)CL_CLAMP(hi, 0L, 255L);
    } else if (str_ieq(key, "ore_host")) {
        str_copy(c->extra[c->cur].host, value, ID_LEN);
    } else if (str_ieq(key, "category")) {
        static const char *const cats[] = {
            "terrain", "rock", "ore", "wood", "building", "plant", "liquid",
            "decoration", "functional", "ancient"
        };
        int k = find_name(cats, (int)CL_COUNTOF(cats), value);
        if (k < 0) return set_err(c, "bad category", value, line);
        b->category = (cl_u8)k;
    } else {
        return set_err(c, "unknown key", key, line);
    }
    return 0;
}

static int item_cb(void *user, const char *type, const char *id, const char *key,
                   const char *value, int line)
{
    LoadCtx *c = (LoadCtx *)user;
    ItemDef *it;
    long n;
    if (!str_ieq(type, "item")) {
        return set_err(c, "unknown section", type, line);
    }
    if (!key) {
        if (item_find(id) >= 0) {
            return set_err(c, "duplicate item", id, line);
        }
        if (g_item_count >= MAX_ITEMS) {
            return set_err(c, "too many items", id, line);
        }
        c->cur = g_item_count++;
        it = &g_items[c->cur];
        memset(it, 0, sizeof(*it));
        str_copy(it->id, id, sizeof(it->id));
        str_copy(it->name, id, sizeof(it->name));
        it->glyph = '?';
        it->fg = C_WHITE;
        it->type = ITEM_RESOURCE;
        it->tool_power = 100;
        it->max_stack = 64;
        it->weight = 100;
        it->volume = 10;
        it->density = 1500;
        it->place_block = -1;
        return 0;
    }
    it = &g_items[c->cur];
    if (str_ieq(key, "name")) {
        str_copy(it->name, value, sizeof(it->name));
    } else if (str_ieq(key, "glyph")) {
        if (!parse_glyph(value, &it->glyph)) return set_err(c, "bad glyph", value, line);
    } else if (str_ieq(key, "fg")) {
        if (!parse_color(value, &it->fg)) return set_err(c, "bad colour", value, line);
    } else if (str_ieq(key, "type")) {
        int t = find_name(g_item_types, ITEM_TYPE_COUNT, value);
        if (t < 0) return set_err(c, "bad type", value, line);
        it->type = (cl_u8)t;
    } else if (str_ieq(key, "tool")) {
        int t = find_name(g_tools, TOOL_COUNT, value);
        if (t < 0) return set_err(c, "bad tool", value, line);
        it->tool_type = (cl_u8)t;
    } else {
        cl_s16 *dst = NULL;
        long lo = 0, hi = 30000;
        if (str_ieq(key, "power")) { dst = &it->tool_power; lo = 1; }
        else if (str_ieq(key, "stack")) { dst = &it->max_stack; lo = 1; hi = 999; }
        else if (str_ieq(key, "weight")) dst = &it->weight;
        else if (str_ieq(key, "volume")) dst = &it->volume;
        else if (str_ieq(key, "density")) dst = &it->density;
        else if (str_ieq(key, "durability")) dst = &it->durability;
        else if (str_ieq(key, "food")) { dst = &it->food; hi = 1000; }
        else if (str_ieq(key, "water")) { dst = &it->water; hi = 1000; }
        if (!dst) {
            return set_err(c, "unknown key", key, line);
        }
        if (!str_to_long(value, &n) || n < lo || n > hi) {
            return set_err(c, "bad number", value, line);
        }
        *dst = (cl_s16)n;
    }
    return 0;
}

static int load_list(const char *root, const char *sub, IniCallback cb, LoadCtx *c)
{
    char path[160];
    char line[64];
    FILE *f;
    int any = 0;
    str_copy(path, root, sizeof(path));
    str_cat(path, "/", sizeof(path));
    str_cat(path, sub, sizeof(path));
    str_cat(path, "/index.lst", sizeof(path));
    f = fopen(path, "r");
    if (!f) {
        str_copy(c->err, "cannot open ", c->errn);
        str_cat(c->err, path, c->errn);
        return 0;
    }
    while (fgets(line, sizeof(line), f)) {
        char *s = str_trim(line);
        char fpath[160];
        int r;
        if (!*s || *s == '#') {
            continue;
        }
        str_copy(fpath, root, sizeof(fpath));
        str_cat(fpath, "/", sizeof(fpath));
        str_cat(fpath, sub, sizeof(fpath));
        str_cat(fpath, "/", sizeof(fpath));
        str_cat(fpath, s, sizeof(fpath));
        c->file = s;
        r = ini_parse_file(fpath, cb, c);
        if (r != 0) {
            if (r < 0) {
                str_copy(c->err, "cannot open ", c->errn);
                str_cat(c->err, fpath, c->errn);
            }
            fclose(f);
            return 0;
        }
        any = 1;
    }
    fclose(f);
    if (!any) {
        str_copy(c->err, "empty index ", c->errn);
        str_cat(c->err, path, c->errn);
    }
    return any;
}

void content_reset(void)
{
    g_block_count = 0;
    g_item_count = 0;
}

static int req(const char *id)
{
    return block_find(id);
}

int content_load(const char *root, char *err, size_t errn)
{
    LoadCtx c;
    int i, ok = 1;
    int first_items;
    content_reset();
    c.extra = (BlockExtra *)mem_calloc(sizeof(BlockExtra) * MAX_BLOCKS);
    c.err = err;
    c.errn = errn;
    c.cur = 0;
    c.file = "";
    err[0] = '\0';
    if (!c.extra) {
        str_copy(err, "out of memory", errn);
        return 0;
    }
    /* block 0 is always air */
    memset(&g_blocks[0], 0, sizeof(BlockDef));
    str_copy(g_blocks[0].id, "air", ID_LEN);
    str_copy(g_blocks[0].name, "Air", NAME_LEN);
    g_blocks[0].glyph = ' ';
    g_blocks[0].flags = BF_REPLACEABLE | BF_NOMINE;
    g_blocks[0].drop_item = -1;
    g_block_count = 1;
    c.extra[0].make_item = 0;

    if (!load_list(root, "blocks", block_cb, &c)) {
        ok = 0;
    }
    /* automatic block items */
    for (i = 1; ok && i < g_block_count; ++i) {
        ItemDef *it;
        BlockDef *b = &g_blocks[i];
        if (!c.extra[i].make_item || (b->flags & BF_LIQUID)) {
            continue;
        }
        if (g_item_count >= MAX_ITEMS) {
            str_copy(err, "too many items", errn);
            ok = 0;
            break;
        }
        it = &g_items[g_item_count++];
        memset(it, 0, sizeof(*it));
        str_copy(it->id, b->id, ID_LEN);
        str_copy(it->name, b->name, NAME_LEN);
        it->glyph = b->glyph;
        it->fg = b->fg == C_BLACK ? C_DARKGRAY : b->fg;
        it->type = ITEM_BLOCK;
        it->tool_power = 100;
        it->max_stack = 64;
        it->weight = c.extra[i].weight;
        it->volume = 100;
        it->density = c.extra[i].density;
        it->place_block = (cl_s16)i;
    }
    first_items = g_item_count;
    if (ok && !load_list(root, "items", item_cb, &c)) {
        ok = 0;
    }
    (void)first_items;
    /* resolve drops and ore hosts */
    for (i = 1; ok && i < g_block_count; ++i) {
        BlockExtra *e = &c.extra[i];
        if (str_ieq(e->drop, "none")) {
            g_blocks[i].drop_item = -1;
        } else if (str_ieq(e->drop, "self")) {
            g_blocks[i].drop_item = (cl_s16)item_find(g_blocks[i].id);
        } else {
            int d = item_find(e->drop);
            if (d < 0) {
                str_copy(err, "unknown drop item: ", errn);
                str_cat(err, e->drop, errn);
                ok = 0;
                break;
            }
            g_blocks[i].drop_item = (cl_s16)d;
        }
        if (g_blocks[i].ore_chance) {
            int h = block_find(e->host[0] ? e->host : "stone");
            if (h < 0) {
                str_copy(err, "unknown ore host: ", errn);
                str_cat(err, e->host, errn);
                ok = 0;
                break;
            }
            g_blocks[i].ore_host = (cl_u8)h;
        }
        if (g_blocks[i].floor_glyph == 0) {
            g_blocks[i].floor_glyph = g_blocks[i].glyph;
        }
        if (g_blocks[i].floor_fg == 255) {
            g_blocks[i].floor_fg = g_blocks[i].fg;
        }
        if (g_blocks[i].floor_bg == 255) {
            g_blocks[i].floor_bg = g_blocks[i].bg;
        }
    }
    mem_free(c.extra);
    if (!ok) {
        return 0;
    }
    g_cb.air = 0;
    g_cb.bedrock = req("bedrock");
    g_cb.stone = req("stone");
    g_cb.deep_rock = req("deep_rock");
    g_cb.dirt = req("dirt");
    g_cb.grass = req("grass");
    g_cb.sand = req("sand");
    g_cb.gravel = req("gravel");
    g_cb.water = req("water");
    g_cb.lava = req("lava");
    g_cb.ice = req("ice");
    g_cb.snow = req("snow");
    g_cb.log = req("oak_log");
    g_cb.leaves = req("oak_leaves");
    g_cb.tall_grass = req("tall_grass");
    g_cb.cobble = req("cobblestone");
    if (g_cb.bedrock < 0 || g_cb.stone < 0 || g_cb.dirt < 0 || g_cb.water < 0 ||
        g_cb.sand < 0 || g_cb.grass < 0) {
        str_copy(err, "core blocks missing (bedrock/stone/dirt/grass/sand/water)", errn);
        return 0;
    }
    if (g_cb.deep_rock < 0) g_cb.deep_rock = g_cb.stone;
    if (g_cb.gravel < 0) g_cb.gravel = g_cb.sand;
    if (g_cb.lava < 0) g_cb.lava = g_cb.stone;
    if (g_cb.ice < 0) g_cb.ice = g_cb.water;
    if (g_cb.snow < 0) g_cb.snow = g_cb.grass;
    if (g_cb.cobble < 0) g_cb.cobble = g_cb.stone;
    return 1;
}
