/* Cliderness - gameplay/entities/creature.c : creature definitions loader */
#include <stdio.h>
#include <string.h>
#include "gameplay/entities/creature.h"
#include "world/structures/structure.h"
#include "world/world.h"
#include "content/content.h"
#include "content/effects.h"
#include "localization/locale.h"
#include "renderer/fb.h"
#include "core/ini.h"
#include "core/str.h"

CreatureDef g_creatures[MAX_CREATURE_DEFS];
int g_creature_count = 0;

typedef struct CCtx {
    int cur, is_loot;
    char *err;
    size_t errn;
} CCtx;

static int cerr(CCtx *c, const char *what, const char *v, int line)
{
    char num[12];
    str_from_long(num, (long)line);
    str_copy(c->err, "creatures.def:", c->errn);
    str_cat(c->err, num, c->errn);
    str_cat(c->err, ": bad ", c->errn);
    str_cat(c->err, what, c->errn);
    str_cat(c->err, " ", c->errn);
    str_cat(c->err, v ? v : "", c->errn);
    return 1;
}

static int pick(const char *const *names, int n, const char *v)
{
    int i;
    for (i = 0; i < n; ++i) {
        if (str_ieq(names[i], v)) return i;
    }
    return -1;
}

/* "coal:4>bread:1" */
static int parse_trade(CreatureDef *d, const char *v)
{
    char buf[64], *gt, *c1, *c2;
    int a, b;
    long na = 1, nb = 1;
    if (d->ntrades >= TRADE_MAX) return 0;
    str_copy(buf, v, sizeof(buf));
    gt = strchr(buf, '>');
    if (!gt) return 0;
    *gt = '\0';
    c1 = strrchr(buf, ':');
    c2 = strrchr(gt + 1, ':');
    if (c1) { *c1 = '\0'; if (!str_to_long(c1 + 1, &na)) return 0; }
    if (c2) { *c2 = '\0'; if (!str_to_long(c2 + 1, &nb)) return 0; }
    a = item_find(str_trim(buf));
    b = item_find(str_trim(gt + 1));
    if (a < 0 || b < 0 || na < 1 || nb < 1) return 0;
    d->trade_in[d->ntrades] = (cl_s16)a;
    d->trade_in_n[d->ntrades] = (cl_s16)na;
    d->trade_out[d->ntrades] = (cl_s16)b;
    d->trade_out_n[d->ntrades] = (cl_s16)nb;
    ++d->ntrades;
    return 1;
}

static int ccb(void *user, const char *type, const char *id, const char *key, const char *v, int line)
{
    CCtx *c = (CCtx *)user;
    CreatureDef *d;
    long n;
    if (!key) {
        if (str_ieq(type, "loot")) {
            if (g_loot_count >= MAX_LOOT) return cerr(c, "loot count", id, line);
            c->cur = g_loot_count++;
            c->is_loot = 1;
            memset(&g_loot[c->cur], 0, sizeof(LootDef));
            str_copy(g_loot[c->cur].id, id, sizeof(g_loot[c->cur].id));
            return 0;
        }
        if (!str_ieq(type, "creature")) return cerr(c, "section", type, line);
        if (g_creature_count >= MAX_CREATURE_DEFS) return cerr(c, "count", id, line);
        c->cur = g_creature_count++;
        c->is_loot = 0;
        d = &g_creatures[c->cur];
        memset(d, 0, sizeof(*d));
        str_copy(d->id, id, sizeof(d->id));
        str_copy(d->name, id, sizeof(d->name));
        d->glyph = '?';
        d->color = C_WHITE;
        d->hp = 10;
        d->damage = 0;
        d->speed = 6;
        d->attack_cd = 20;
        d->sight = 8;
        d->hearing = 10;
        d->flee_pct = 0;
        d->weight = 10;
        d->group_min = d->group_max = 1;
        d->effect = -1;
        d->activity = ACTV_ANY;
        return 0;
    }
    if (c->is_loot) {
        LootDef *l = &g_loot[c->cur];
        long lo, hi, ch;
        int it = item_find(key);
        if (it < 0 || l->n >= LOOT_ENTRIES || sscanf(v, "%ld-%ld:%ld", &lo, &hi, &ch) != 3) {
            return cerr(c, "loot entry", key, line);
        }
        l->item[l->n] = (cl_s16)it;
        l->lo[l->n] = (cl_u8)lo;
        l->hi[l->n] = (cl_u8)hi;
        l->chance[l->n] = (cl_u8)ch;
        ++l->n;
        return 0;
    }
    d = &g_creatures[c->cur];
    if (str_ieq(key, "name")) {
        str_copy(d->name, v, sizeof(d->name));
    } else if (str_ieq(key, "glyph")) {
        if (v[0] == '\'' && v[1] && v[2] == '\'') d->glyph = (cl_u8)v[1];
        else if (str_to_long(v, &n) && n >= 0 && n < 256) d->glyph = (cl_u8)n;
        else if (v[0] && !v[1]) d->glyph = (cl_u8)v[0];
        else return cerr(c, "glyph", v, line);
    } else if (str_ieq(key, "color")) {
        int col = color_from_name(v);
        if (col < 0) return cerr(c, "colour", v, line);
        d->color = (cl_u8)col;
    } else if (str_ieq(key, "kind")) {
        static const char *const k[] = { "animal", "monster", "npc" };
        int i = pick(k, 3, v);
        if (i < 0) return cerr(c, "kind", v, line);
        d->kind = (cl_u8)i;
    } else if (str_ieq(key, "aggression")) {
        static const char *const k[] = { "never", "defensive", "night", "always" };
        int i = pick(k, 4, v);
        if (i < 0) return cerr(c, "aggression", v, line);
        d->aggression = (cl_u8)i;
    } else if (str_ieq(key, "activity")) {
        static const char *const k[] = { "day", "night", "any" };
        int i = pick(k, 3, v);
        if (i < 0) return cerr(c, "activity", v, line);
        d->activity = (cl_u8)i;
    } else if (str_ieq(key, "habitat")) {
        static const char *const k[] = { "surface", "cave", "water" };
        int i = pick(k, 3, v);
        if (i < 0) return cerr(c, "habitat", v, line);
        d->habitat = (cl_u8)i;
        d->aquatic = (cl_u8)(i == SPAWN_WATER);
    } else if (str_ieq(key, "dimension")) {
        d->dim = (cl_u8)(str_ieq(v, "ancient") ? DIM_ANCIENT : DIM_OVERWORLD);
    } else if (str_ieq(key, "effect")) {
        d->effect = (cl_s16)effect_find(v);
        if (d->effect < 0) return cerr(c, "effect", v, line);
    } else if (str_ieq(key, "loot")) {
        str_copy(d->loot, v, sizeof(d->loot));
    } else if (str_ieq(key, "biomes")) {
        str_copy(d->biomes, v, sizeof(d->biomes));
    } else if (str_ieq(key, "trade")) {
        if (!parse_trade(d, v)) return cerr(c, "trade", v, line);
    } else if (str_ieq(key, "group")) {
        long lo, hi;
        if (sscanf(v, "%ld-%ld", &lo, &hi) != 2 || lo < 1 || hi < lo || hi > 8) return cerr(c, "group", v, line);
        d->group_min = (cl_u8)lo;
        d->group_max = (cl_u8)hi;
    } else {
        cl_s16 *dst = NULL;
        if (!str_to_long(v, &n) || n < 0 || n > 30000) return cerr(c, key, v, line);
        if (str_ieq(key, "hp")) dst = &d->hp;
        else if (str_ieq(key, "damage")) dst = &d->damage;
        else if (str_ieq(key, "defense")) dst = &d->defense;
        else if (str_ieq(key, "speed")) dst = &d->speed;
        else if (str_ieq(key, "attack_cd")) dst = &d->attack_cd;
        else if (str_ieq(key, "sight")) dst = &d->sight;
        else if (str_ieq(key, "hearing")) dst = &d->hearing;
        else if (str_ieq(key, "flee_pct")) dst = &d->flee_pct;
        else if (str_ieq(key, "weight")) dst = &d->weight;
        else if (str_ieq(key, "xp")) dst = &d->xp;
        else if (str_ieq(key, "burns")) { d->burns = (cl_u8)(n != 0); return 0; }
        else if (str_ieq(key, "effect_chance")) { d->effect_chance = (cl_u8)CL_MIN(n, 100L); return 0; }
        if (!dst) return cerr(c, "key", key, line);
        *dst = (cl_s16)n;
    }
    return 0;
}

int creatures_load(const char *root, char *err, size_t errn)
{
    char path[160];
    CCtx c;
    int r, i;
    g_creature_count = 0;
    memset(&c, 0, sizeof(c));
    c.err = err;
    c.errn = errn;
    str_copy(path, root, sizeof(path));
    str_cat(path, "/entities/creature.def", sizeof(path));
    r = ini_parse_file(path, ccb, &c);
    if (r < 0) {
        str_copy(err, "cannot open ", errn);
        str_cat(err, path, errn);
        return 0;
    }
    if (r > 0) {
        return 0;
    }
    for (i = 0; i < g_creature_count; ++i) {
        if (g_creatures[i].loot[0] && loot_find(g_creatures[i].loot) < 0) {
            str_copy(err, "unknown loot table: ", errn);
            str_cat(err, g_creatures[i].loot, errn);
            return 0;
        }
        if (g_creatures[i].speed < 1) g_creatures[i].speed = 1;
    }
    return 1;
}

int creature_find(const char *id)
{
    int i;
    const char *colon = strchr(id, ':');
    if (colon) id = colon + 1;
    for (i = 0; i < g_creature_count; ++i) {
        if (str_ieq(g_creatures[i].id, id)) return i;
    }
    return -1;
}

const char *creature_name(int c)
{
    char key[40];
    if (c < 0 || c >= g_creature_count) return "?";
    str_copy(key, "mob.", sizeof(key));
    str_cat(key, g_creatures[c].id, sizeof(key));
    return loc_get_or(key, g_creatures[c].name);
}

const char *mob_state_key(int st)
{
    static const char *const k[MS_COUNT] = {
        "ai.idle", "ai.wander", "ai.chase", "ai.attack", "ai.flee", "ai.sleep", "ai.home", "ai.shelter"
    };
    return k[(st >= 0 && st < MS_COUNT) ? st : 0];
}
