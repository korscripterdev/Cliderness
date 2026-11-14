/* Cliderness - content/effects.c */
#include <string.h>
#include "content/effects.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/ini.h"
#include "core/str.h"

EffectDef g_effects[MAX_EFFECTS];
int g_effect_count = 0;

typedef struct ECtx {
    int cur;
    char *err;
    size_t errn;
} ECtx;

static int eerr(ECtx *c, const char *what, int line)
{
    char num[12];
    str_from_long(num, (long)line);
    str_copy(c->err, "effects.def:", c->errn);
    str_cat(c->err, num, c->errn);
    str_cat(c->err, ": bad ", c->errn);
    str_cat(c->err, what, c->errn);
    return 1;
}

static int effect_cb(void *user, const char *type, const char *id, const char *key,
                     const char *value, int line)
{
    ECtx *c = (ECtx *)user;
    EffectDef *e;
    long n;
    if (!str_ieq(type, "effect")) {
        return eerr(c, "section", line);
    }
    if (!key) {
        if (g_effect_count >= MAX_EFFECTS || effect_find(id) >= 0) {
            return eerr(c, "effect", line);
        }
        c->cur = g_effect_count++;
        e = &g_effects[c->cur];
        memset(e, 0, sizeof(*e));
        str_copy(e->id, id, sizeof(e->id));
        str_copy(e->name, id, sizeof(e->name));
        e->regen = 100;
        e->duration = 60;
        e->color = C_LIGHTGRAY;
        return 0;
    }
    e = &g_effects[c->cur];
    if (str_ieq(key, "name")) {
        str_copy(e->name, value, sizeof(e->name));
        return 0;
    }
    if (str_ieq(key, "color")) {
        int col = color_from_name(value);
        if (col < 0) return eerr(c, "colour", line);
        e->color = (cl_u8)col;
        return 0;
    }
    if (!str_to_long(value, &n) || n < -1000 || n > 30000) {
        return eerr(c, key, line);
    }
    if (str_ieq(key, "hp")) e->hp = (cl_s16)n;
    else if (str_ieq(key, "hunger")) e->hunger = (cl_s16)n;
    else if (str_ieq(key, "thirst")) e->thirst = (cl_s16)n;
    else if (str_ieq(key, "stamina")) e->stamina = (cl_s16)n;
    else if (str_ieq(key, "temp")) e->temp = (cl_s16)n;
    else if (str_ieq(key, "move")) e->move = (cl_s16)n;
    else if (str_ieq(key, "regen")) e->regen = (cl_s16)n;
    else if (str_ieq(key, "duration")) e->duration = (cl_s16)n;
    else if (str_ieq(key, "harmful")) e->harmful = (cl_u8)(n != 0);
    else return eerr(c, key, line);
    return 0;
}

int effects_load(const char *root, char *err, size_t errn)
{
    char path[160];
    ECtx c;
    int r;
    g_effect_count = 0;
    c.cur = 0;
    c.err = err;
    c.errn = errn;
    str_copy(path, root, sizeof(path));
    str_cat(path, "/effects/effects.def", sizeof(path));
    r = ini_parse_file(path, effect_cb, &c);
    if (r < 0) {
        str_copy(err, "cannot open ", errn);
        str_cat(err, path, errn);
        return 0;
    }
    return r == 0;
}

int effect_find(const char *id)
{
    int i;
    for (i = 0; i < g_effect_count; ++i) {
        if (str_ieq(g_effects[i].id, id)) {
            return i;
        }
    }
    return -1;
}

const char *effect_name(int e)
{
    char key[32];
    if (e < 0 || e >= g_effect_count) {
        return "?";
    }
    str_copy(key, "effect.", sizeof(key));
    str_cat(key, g_effects[e].id, sizeof(key));
    return loc_get_or(key, g_effects[e].name);
}
