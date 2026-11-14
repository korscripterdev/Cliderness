/* Cliderness - content/recipes.c */
#include <stdio.h>
#include <string.h>
#include "content/recipes.h"
#include "content/content.h"
#include "core/ini.h"
#include "core/str.h"

RecipeDef g_recipes[MAX_RECIPES];
int g_recipe_count = 0;

static const char *const g_stations[STATION_COUNT] = {
    "none", "workbench", "furnace", "campfire", "anvil", "altar"
};
static const char *const g_rcats[RCAT_COUNT] = {
    "tools", "building", "materials", "food", "equipment", "medical", "vehicles", "ancient"
};
/* same order as SK_* in gameplay/player/player.h */
static const char *const g_skills[] = {
    "mining", "woodcutting", "building", "crafting", "melee", "ranged",
    "swimming", "sailing", "fishing", "cooking", "stealth", "exploration"
};

int station_find(const char *name)
{
    int i;
    for (i = 0; i < STATION_COUNT; ++i) {
        if (str_ieq(name, g_stations[i])) {
            return i;
        }
    }
    return -1;
}

const char *station_key(int st)
{
    static const char *const k[STATION_COUNT] = {
        "station.none", "station.workbench", "station.furnace", "station.campfire",
        "station.anvil", "station.altar"
    };
    return k[(st >= 0 && st < STATION_COUNT) ? st : 0];
}

const char *recipe_category_key(int cat)
{
    static const char *const k[RCAT_COUNT] = {
        "rcat.tools", "rcat.building", "rcat.materials", "rcat.food", "rcat.equipment",
        "rcat.medical", "rcat.vehicles", "rcat.ancient"
    };
    return k[(cat >= 0 && cat < RCAT_COUNT) ? cat : 0];
}

typedef struct RCtx {
    int cur;
    char *err;
    size_t errn;
    const char *file;
} RCtx;

static int rerr(RCtx *c, const char *msg, const char *what, int line)
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

/* "item:count" */
static int parse_stack(const char *v, int *item, long *count)
{
    char buf[48];
    char *colon;
    str_copy(buf, v, sizeof(buf));
    colon = strrchr(buf, ':');
    *count = 1;
    /* the last ":<digits>" part is the count; "ns:item" stays an id */
    if (colon && str_to_long(colon + 1, count)) {
        *colon = '\0';
    } else {
        *count = 1;
    }
    *item = item_find(buf);
    return *item >= 0 && *count >= 1 && *count <= 999;
}

static int recipe_cb(void *user, const char *type, const char *id, const char *key,
                     const char *value, int line)
{
    RCtx *c = (RCtx *)user;
    RecipeDef *r;
    long n;
    int k;
    if (!str_ieq(type, "recipe")) {
        return rerr(c, "unknown section", type, line);
    }
    if (!key) {
        if (g_recipe_count >= MAX_RECIPES || recipe_find(id) >= 0) {
            return rerr(c, "duplicate or too many recipes", id, line);
        }
        c->cur = g_recipe_count++;
        r = &g_recipes[c->cur];
        memset(r, 0, sizeof(*r));
        str_copy(r->id, id, sizeof(r->id));
        r->result = -1;
        r->count = 1;
        r->skill = 3; /* crafting */
        return 0;
    }
    r = &g_recipes[c->cur];
    if (str_ieq(key, "result")) {
        int it = item_find(value);
        if (it < 0) return rerr(c, "unknown item", value, line);
        r->result = (cl_s16)it;
    } else if (str_ieq(key, "count")) {
        if (!str_to_long(value, &n) || n < 1 || n > 99) return rerr(c, "bad count", value, line);
        r->count = (cl_s16)n;
    } else if (str_ieq(key, "in")) {
        int it;
        if (r->nin >= RECIPE_IN) return rerr(c, "too many ingredients", value, line);
        if (!parse_stack(value, &it, &n)) return rerr(c, "bad ingredient", value, line);
        r->in_item[r->nin] = (cl_s16)it;
        r->in_count[r->nin] = (cl_s16)n;
        ++r->nin;
    } else if (str_ieq(key, "station")) {
        k = station_find(value);
        if (k < 0) return rerr(c, "bad station", value, line);
        r->station = (cl_u8)k;
    } else if (str_ieq(key, "fuel")) {
        if (!str_to_long(value, &n) || n < 0 || n > 50) return rerr(c, "bad fuel", value, line);
        r->fuel = (cl_u8)n;
    } else if (str_ieq(key, "skill")) {
        for (k = 0; k < (int)CL_COUNTOF(g_skills); ++k) {
            if (str_ieq(value, g_skills[k])) break;
        }
        if (k == (int)CL_COUNTOF(g_skills)) return rerr(c, "bad skill", value, line);
        r->skill = (cl_u8)k;
    } else if (str_ieq(key, "min_skill")) {
        if (!str_to_long(value, &n) || n < 0 || n > 100) return rerr(c, "bad min_skill", value, line);
        r->min_skill = (cl_u8)n;
    } else if (str_ieq(key, "category")) {
        for (k = 0; k < RCAT_COUNT; ++k) {
            if (str_ieq(value, g_rcats[k])) break;
        }
        if (k == RCAT_COUNT) return rerr(c, "bad category", value, line);
        r->category = (cl_u8)k;
    } else {
        return rerr(c, "unknown key", key, line);
    }
    return 0;
}

int recipes_load(const char *root, char *err, size_t errn)
{
    char path[160];
    char line[64];
    FILE *f;
    RCtx c;
    int i;
    g_recipe_count = 0;
    c.cur = 0;
    c.err = err;
    c.errn = errn;
    c.file = "";
    str_copy(path, root, sizeof(path));
    str_cat(path, "/recipes/index.lst", sizeof(path));
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
        str_cat(fp, "/recipes/", sizeof(fp));
        str_cat(fp, s, sizeof(fp));
        c.file = s;
        r = ini_parse_file(fp, recipe_cb, &c);
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
    for (i = 0; i < g_recipe_count; ++i) {
        if (g_recipes[i].result < 0 || g_recipes[i].nin == 0) {
            str_copy(err, "recipe without result or ingredients: ", errn);
            str_cat(err, g_recipes[i].id, errn);
            return 0;
        }
    }
    return 1;
}

int recipe_find(const char *id)
{
    int i;
    for (i = 0; i < g_recipe_count; ++i) {
        if (str_ieq(g_recipes[i].id, id)) {
            return i;
        }
    }
    return -1;
}
