/* Cliderness - save/save.c */
#include <stdio.h>
#include <string.h>
#include "save/save.h"
#include "content/content.h"
#include "content/effects.h"
#include "core/crc.h"
#include "core/ini.h"
#include "core/str.h"
#include "core/log.h"
#include "core/version.h"

typedef struct SaveW {
    FILE  *f;
    cl_u32 crc;
    int    err;
} SaveW;

static void sw_line(SaveW *w, const char *s)
{
    if (fputs(s, w->f) < 0 || fputc('\n', w->f) == EOF) {
        w->err = 1;
    }
    w->crc = crc32_update(w->crc, s, strlen(s));
    w->crc = crc32_update(w->crc, "\n", 1);
}

static void sw_kv(SaveW *w, const char *k, const char *v)
{
    char buf[256];
    str_copy(buf, k, sizeof(buf));
    str_cat(buf, "=", sizeof(buf));
    str_cat(buf, v, sizeof(buf));
    sw_line(w, buf);
}

static void sw_kl(SaveW *w, const char *k, long v)
{
    char num[16];
    str_from_long(num, v);
    sw_kv(w, k, num);
}

int save_valid_name(const char *name)
{
    int n = 0;
    if (!name || !*name) {
        return 0;
    }
    for (; *name; ++name, ++n) {
        char c = *name;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-')) {
            return 0;
        }
    }
    return n <= WORLD_NAME_MAX;
}

void save_world_dir(const char *name, char *out, size_t n)
{
    str_copy(out, SAVE_ROOT, n);
    str_cat(out, "/", n);
    str_cat(out, name, n);
}

static void file_in(const char *dir, const char *f, char *out, size_t n)
{
    str_copy(out, dir, n);
    str_cat(out, "/", n);
    str_cat(out, f, n);
}

int save_exists(const char *name)
{
    char dir[96], path[128];
    FILE *f;
    save_world_dir(name, dir, sizeof(dir));
    file_in(dir, "world.ini", path, sizeof(path));
    f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

int save_prepare(const ClidePlatform *p, const char *name)
{
    char dir[96];
    if (!save_valid_name(name)) {
        return 0;
    }
    p->fs.make_dir(SAVE_ROOT);
    save_world_dir(name, dir, sizeof(dir));
    return p->fs.make_dir(dir) == 0;
}

int save_verify_file(const char *path)
{
    static char line[512];
    FILE *f = fopen(path, "r");
    cl_u32 crc = CRC32_INIT;
    int ok = 0;
    if (!f) {
        return 0;
    }
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (strncmp(line, "crc=", 4) == 0) {
            char want[16];
            sprintf(want, "%08lX", (unsigned long)crc32_final(crc));
            ok = strcmp(line + 4, want) == 0;
            break;
        }
        crc = crc32_update(crc, line, len);
        crc = crc32_update(crc, "\n", 1);
    }
    fclose(f);
    return ok;
}

static void write_body(SaveW *w, Game *g)
{
    char buf[128];
    char key[32];
    int i;
    Player *p = &g->p;
    sw_line(w, "# Cliderness world save - created by korscripterdev");
    sw_line(w, "[world]");
    sw_kv(w, "game", CLIDE_NAME);
    sw_kv(w, "creator", CLIDE_CREATOR);
    sw_kv(w, "version", CLIDE_VERSION);
    sw_kl(w, "format", CLIDE_SAVE_FORMAT);
    sw_kv(w, "name", g->name);
    sprintf(buf, "%lu", (unsigned long)g->seed);
    sw_kv(w, "seed", buf);
    sw_kl(w, "mode", g->mode);
    sw_kl(w, "cheats", g->cheats);
    sw_kl(w, "cheated", g->cheated);
    sw_kl(w, "tick", g->tick);
    sw_line(w, "[time]");
    sw_kl(w, "day", g->time.day);
    sw_kl(w, "ms", g->time.ms);
    sw_kl(w, "rem1", g->time.rem1);
    sw_kl(w, "rem2", g->time.rem2);
    sw_kl(w, "real_seconds_per_day", g->time.cfg.real_seconds_per_day);
    sw_kl(w, "days_per_year", g->time.cfg.days_per_year);
    sw_kl(w, "days_per_season", g->time.cfg.days_per_season);
    sw_kl(w, "scale_pct", g->time.cfg.scale_pct);
    sw_line(w, "[weather]");
    sw_kv(w, "current", g_weather[g->weather.cur].id);
    sw_kl(w, "left", g->weather.minutes_left);
    sw_kl(w, "wind", g->weather.wind_dir);
    sprintf(buf, "%lu", (unsigned long)g->weather.rng.s);
    sw_kv(w, "rng", buf);
    sw_line(w, "[player]");
    sw_kl(w, "x", p->x);
    sw_kl(w, "y", p->y);
    sw_kl(w, "z", p->z);
    sw_kl(w, "dim", p->dim);
    sw_kl(w, "facing", p->facing);
    sw_kl(w, "hp", p->hp);
    sw_kl(w, "hp_max", p->hp_max);
    sw_kl(w, "stamina", p->stamina);
    sw_kl(w, "hunger", p->hunger);
    sw_kl(w, "thirst", p->thirst);
    sw_kl(w, "fatigue", p->fatigue);
    sw_kl(w, "body_temp", p->body_temp);
    sw_kl(w, "crouch", p->crouch);
    sw_kl(w, "god", p->god);
    sw_kl(w, "noclip", p->noclip);
    sw_kl(w, "fly", p->fly);
    sw_kl(w, "dead", p->dead);
    sw_kl(w, "spawn_x", p->spawn_x);
    sw_kl(w, "spawn_y", p->spawn_y);
    sw_kl(w, "spawn_z", p->spawn_z);
    sw_kl(w, "best_dist", p->best_dist);
    for (i = 0; i < ST_COUNT; ++i) {
        sprintf(key, "stat%d", i);
        sw_kl(w, key, p->stats[i]);
    }
    for (i = 0; i < SK_COUNT; ++i) {
        sprintf(key, "skill%d", i);
        sw_kl(w, key, p->skill_xp[i]);
    }
    sw_line(w, "[inventory]");
    sw_kl(w, "selected", g->inv.selected);
    for (i = 0; i < INV_SLOTS; ++i) {
        const ItemStack *s = &g->inv.s[i];
        if (s->item < 0) {
            continue;
        }
        sprintf(key, "slot%d", i);
        sprintf(buf, "%s %d %d", g_items[s->item].id, s->count, s->dur);
        sw_kv(w, key, buf);
    }
    for (i = 0; i < INV_EQUIP; ++i) {
        const ItemStack *s = &g->inv.equip[i];
        if (s->item < 0) {
            continue;
        }
        sprintf(key, "eq%d", i);
        sprintf(buf, "%s %d %d", g_items[s->item].id, s->count, s->dur);
        sw_kv(w, key, buf);
    }
    sw_line(w, "[effects]");
    for (i = 0; i < PLAYER_FX; ++i) {
        if (p->fx[i].id < 0 || p->fx[i].minutes <= 0) {
            continue;
        }
        sprintf(key, "fx%d", i);
        sprintf(buf, "%s %d", g_effects[p->fx[i].id].id, p->fx[i].minutes);
        sw_kv(w, key, buf);
    }
    sw_kl(w, "cold_minutes", p->cold_minutes);
    sw_line(w, "[crops]");
    for (i = 0; i < MAX_CROPS; ++i) {
        const Crop *c = &g->crops[i];
        if (!c->active) {
            continue;
        }
        sprintf(key, "c%d", i);
        sprintf(buf, "%d %ld %ld %ld %ld", c->dim, c->x, c->y, c->z, c->next_minute);
        sw_kv(w, key, buf);
    }
    sw_line(w, "[chests]");
    for (i = 0; i < MAX_CHESTS; ++i) {
        const Chest *c = &g->chests[i];
        int k;
        if (!c->active) {
            continue;
        }
        sprintf(key, "c%d", i);
        sprintf(buf, "%d %ld %ld %ld", c->dim, c->x, c->y, c->z);
        sw_kv(w, key, buf);
        for (k = 0; k < CHEST_SLOTS; ++k) {
            if (c->s[k].item < 0) {
                continue;
            }
            sprintf(key, "c%d_%d", i, k);
            sprintf(buf, "%s %d %d", g_items[c->s[k].item].id, c->s[k].count, c->s[k].dur);
            sw_kv(w, key, buf);
        }
    }
    sw_line(w, "[entities]");
    for (i = 0; i < MAX_ITEM_ENTS; ++i) {
        const ItemEntity *e = &g->ents[i];
        if (!e->active) {
            continue;
        }
        sprintf(key, "e%d", i);
        sprintf(buf, "%s %d %d %d %ld %ld %ld %ld", g_items[e->item].id, e->count, e->dur,
                e->dim, e->x, e->y, e->z, e->age);
        sw_kv(w, key, buf);
    }
    sw_line(w, "[end]");
}

int save_game(Game *g, const ClidePlatform *p, char *err, size_t errn)
{
    char tmp[128], dst[128];
    SaveW w;
    err[0] = '\0';
    if (!save_prepare(p, g->name)) {
        str_copy(err, "cannot create save directory", errn);
        return 0;
    }
    g->world.replace = p->fs.replace;
    if (!world_save(&g->world)) {
        str_copy(err, "chunk store commit failed", errn);
        return 0;
    }
    file_in(g->dir, "world.tmp", tmp, sizeof(tmp));
    file_in(g->dir, "world.ini", dst, sizeof(dst));
    w.f = fopen(tmp, "w");
    w.crc = CRC32_INIT;
    w.err = 0;
    if (!w.f) {
        str_copy(err, "cannot write world.tmp", errn);
        return 0;
    }
    write_body(&w, g);
    fprintf(w.f, "crc=%08lX\n", (unsigned long)crc32_final(w.crc));
    if (fclose(w.f) != 0 || w.err) {
        remove(tmp);
        str_copy(err, "write error (disk full?)", errn);
        return 0;
    }
    if (!save_verify_file(tmp)) {
        remove(tmp);
        str_copy(err, "verification failed", errn);
        return 0;
    }
    if (p->fs.replace(tmp, dst) != 0) {
        str_copy(err, "cannot replace world.ini", errn);
        return 0;
    }
    save_register(p, g->name);
    g->unsaved = 0;
    return 1;
}

/* ---------------------------------------------------------------- loading */

typedef struct LoadCtx {
    Game *g;
    int   phase;      /* 0 = header only, 1 = full */
    cl_u32 seed;
    int   mode, cheats;
    int   format;
    int   unknown_items;
    char  game[24];
} LoadCtx;

static long num(const char *v)
{
    long n = 0;
    str_to_long(v, &n);
    return n;
}

static cl_u32 unum(const char *s)
{
    unsigned long u = 0;
    while (*s >= '0' && *s <= '9') {
        u = u * 10UL + (unsigned long)(*s - '0');
        ++s;
    }
    return CL_U32(u);
}

static int *player_int(Player *p, const char *key)
{
    if (str_ieq(key, "dim")) return &p->dim;
    if (str_ieq(key, "facing")) return &p->facing;
    if (str_ieq(key, "hp")) return &p->hp;
    if (str_ieq(key, "hp_max")) return &p->hp_max;
    if (str_ieq(key, "stamina")) return &p->stamina;
    if (str_ieq(key, "hunger")) return &p->hunger;
    if (str_ieq(key, "thirst")) return &p->thirst;
    if (str_ieq(key, "fatigue")) return &p->fatigue;
    if (str_ieq(key, "body_temp")) return &p->body_temp;
    if (str_ieq(key, "crouch")) return &p->crouch;
    if (str_ieq(key, "god")) return &p->god;
    if (str_ieq(key, "noclip")) return &p->noclip;
    if (str_ieq(key, "fly")) return &p->fly;
    if (str_ieq(key, "dead")) return &p->dead;
    return NULL;
}

static int load_cb(void *user, const char *type, const char *id, const char *key,
                   const char *v, int line)
{
    LoadCtx *c = (LoadCtx *)user;
    Game *g = c->g;
    Player *p = &g->p;
    (void)id;
    (void)line;
    if (!key) {
        return 0;
    }
    if (str_ieq(type, "world")) {
        if (str_ieq(key, "seed")) {
            c->seed = unum(v);
        } else if (str_ieq(key, "mode")) c->mode = (int)num(v);
        else if (str_ieq(key, "cheats")) c->cheats = (int)num(v);
        else if (str_ieq(key, "format")) c->format = (int)num(v);
        else if (str_ieq(key, "game")) str_copy(c->game, v, sizeof(c->game));
        else if (c->phase && str_ieq(key, "cheated")) g->cheated = (int)num(v);
        else if (c->phase && str_ieq(key, "tick")) g->tick = num(v);
        return 0;
    }
    if (!c->phase) {
        return 0;
    }
    if (str_ieq(type, "time")) {
        if (str_ieq(key, "day")) g->time.day = num(v);
        else if (str_ieq(key, "ms")) g->time.ms = num(v);
        else if (str_ieq(key, "rem1")) g->time.rem1 = num(v);
        else if (str_ieq(key, "rem2")) g->time.rem2 = num(v);
        else if (str_ieq(key, "real_seconds_per_day")) g->time.cfg.real_seconds_per_day = num(v);
        else if (str_ieq(key, "days_per_year")) g->time.cfg.days_per_year = num(v);
        else if (str_ieq(key, "days_per_season")) g->time.cfg.days_per_season = num(v);
        else if (str_ieq(key, "scale_pct")) g->time.cfg.scale_pct = num(v);
    } else if (str_ieq(type, "weather")) {
        if (str_ieq(key, "current")) {
            int wi = weather_find(v);
            g->weather.cur = wi >= 0 ? wi : 0;
        } else if (str_ieq(key, "left")) g->weather.minutes_left = num(v);
        else if (str_ieq(key, "wind")) g->weather.wind_dir = (int)num(v) & 255;
        else if (str_ieq(key, "rng")) {
            g->weather.rng.s = unum(v);
            if (!g->weather.rng.s) g->weather.rng.s = 1;
        }
    } else if (str_ieq(type, "player")) {
        if (str_iprefix(key, "stat") && !str_iprefix(key, "stamina")) {
            long k;
            if (str_to_long(key + 4, &k) && k >= 0 && k < ST_COUNT) {
                p->stats[k] = (int)CL_CLAMP(num(v), 1L, 100L);
            }
            return 0;
        }
        if (str_iprefix(key, "skill")) {
            long k;
            if (str_to_long(key + 5, &k) && k >= 0 && k < SK_COUNT) {
                p->skill_xp[k] = num(v);
            }
            return 0;
        }
        {
            int *ip = player_int(p, key);
            if (ip) {
                *ip = (int)num(v);
                return 0;
            }
        }
        if (str_ieq(key, "x")) p->x = num(v);
        else if (str_ieq(key, "y")) p->y = num(v);
        else if (str_ieq(key, "z")) p->z = num(v);
        else if (str_ieq(key, "spawn_x")) p->spawn_x = num(v);
        else if (str_ieq(key, "spawn_y")) p->spawn_y = num(v);
        else if (str_ieq(key, "spawn_z")) p->spawn_z = num(v);
        else if (str_ieq(key, "best_dist")) p->best_dist = num(v);
    } else if (str_ieq(type, "inventory")) {
        if (str_ieq(key, "selected")) {
            g->inv.selected = (int)CL_CLAMP(num(v), 0L, (long)(INV_HOTBAR - 1));
        } else if (str_iprefix(key, "eq")) {
            long eq;
            char iid[ID_LEN + 8];
            int cnt = 0, dur = 0, it;
            if (!str_to_long(key + 2, &eq) || eq < 0 || eq >= INV_EQUIP ||
                sscanf(v, "%35s %d %d", iid, &cnt, &dur) < 2) {
                return 0;
            }
            it = item_find(iid);
            if (it < 0) {
                ++c->unknown_items;
                return 0;
            }
            g->inv.equip[eq].item = (cl_s16)it;
            g->inv.equip[eq].count = (cl_s16)cnt;
            g->inv.equip[eq].dur = (cl_s16)dur;
        } else if (str_iprefix(key, "slot")) {
            long slot;
            char iid[ID_LEN + 8];
            int cnt = 0, dur = 0, it;
            if (!str_to_long(key + 4, &slot) || slot < 0 || slot >= INV_SLOTS) {
                return 0;
            }
            if (sscanf(v, "%35s %d %d", iid, &cnt, &dur) < 2) {
                return 0;
            }
            it = item_find(iid);
            if (it < 0) {
                ++c->unknown_items;
                return 0;
            }
            g->inv.s[slot].item = (cl_s16)it;
            g->inv.s[slot].count = (cl_s16)cnt;
            g->inv.s[slot].dur = (cl_s16)dur;
        }
    } else if (str_ieq(type, "effects")) {
        char eid[32];
        int mins = 0;
        if (str_ieq(key, "cold_minutes")) {
            p->cold_minutes = (int)num(v);
        } else if (sscanf(v, "%31s %d", eid, &mins) == 2) {
            int e = effect_find(eid), k;
            for (k = 0; e >= 0 && k < PLAYER_FX; ++k) {
                if (p->fx[k].id < 0) {
                    p->fx[k].id = (cl_s16)e;
                    p->fx[k].minutes = (cl_s16)mins;
                    break;
                }
            }
        }
    } else if (str_ieq(type, "crops")) {
        int dim, k;
        long x, y, z, next;
        if (sscanf(v, "%d %ld %ld %ld %ld", &dim, &x, &y, &z, &next) == 5) {
            for (k = 0; k < MAX_CROPS; ++k) {
                Crop *cr = &g->crops[k];
                if (!cr->active) {
                    cr->active = 1;
                    cr->dim = (cl_u8)dim;
                    cr->x = x;
                    cr->y = y;
                    cr->z = z;
                    cr->next_minute = next;
                    break;
                }
            }
        }
    } else if (str_ieq(type, "chests")) {
        const char *us = strchr(key, '_');
        long idx;
        char ib[16];
        size_t len = us ? (size_t)(us - key - 1) : strlen(key) - 1;
        if (key[0] != 'c' || len == 0 || len >= sizeof(ib)) {
            return 0;
        }
        memcpy(ib, key + 1, len);
        ib[len] = '\0';
        if (!str_to_long(ib, &idx) || idx < 0 || idx >= MAX_CHESTS) {
            return 0;
        }
        if (!us) {
            int dim, k;
            long x, y, z;
            if (sscanf(v, "%d %ld %ld %ld", &dim, &x, &y, &z) == 4) {
                Chest *ch = &g->chests[idx];
                memset(ch, 0, sizeof(*ch));
                ch->active = 1;
                ch->dim = (cl_u8)dim;
                ch->x = x;
                ch->y = y;
                ch->z = z;
                for (k = 0; k < CHEST_SLOTS; ++k) {
                    ch->s[k].item = -1;
                }
            }
        } else {
            long slot;
            char iid[ID_LEN + 8];
            int cnt = 0, dur = 0, it;
            if (!str_to_long(us + 1, &slot) || slot < 0 || slot >= CHEST_SLOTS ||
                sscanf(v, "%35s %d %d", iid, &cnt, &dur) < 2) {
                return 0;
            }
            it = item_find(iid);
            if (it < 0) {
                ++c->unknown_items;
                return 0;
            }
            g->chests[idx].s[slot].item = (cl_s16)it;
            g->chests[idx].s[slot].count = (cl_s16)cnt;
            g->chests[idx].s[slot].dur = (cl_s16)dur;
        }
    } else if (str_ieq(type, "entities")) {
        char iid[ID_LEN + 8];
        int cnt, dur, dim;
        long x, y, z, age;
        int it;
        if (sscanf(v, "%35s %d %d %d %ld %ld %ld %ld", iid, &cnt, &dur, &dim, &x, &y, &z, &age) != 8) {
            return 0;
        }
        it = item_find(iid);
        if (it < 0) {
            ++c->unknown_items;
            return 0;
        }
        {
            int e = ent_spawn(g, dim, x, y, z, it, cnt, dur);
            if (e >= 0) {
                g->ents[e].age = age;
            }
        }
    }
    return 0;
}

int save_load(Game *g, const ClidePlatform *p, const char *name, const ClideTimeCfg *tcfg,
              char *err, size_t errn)
{
    char dir[96], path[128];
    LoadCtx c;
    err[0] = '\0';
    if (!save_valid_name(name)) {
        str_copy(err, "invalid world name", errn);
        return 0;
    }
    save_world_dir(name, dir, sizeof(dir));
    file_in(dir, "world.ini", path, sizeof(path));
    if (!save_verify_file(path)) {
        str_copy(err, "world.ini missing or corrupt (CRC mismatch)", errn);
        return 0;
    }
    memset(&c, 0, sizeof(c));
    c.g = g;
    c.phase = 0;
    memset(g, 0, sizeof(*g));
    if (ini_parse_file(path, load_cb, &c) != 0) {
        str_copy(err, "cannot parse world.ini", errn);
        return 0;
    }
    if (!str_ieq(c.game, CLIDE_NAME)) {
        str_copy(err, "not a Cliderness save", errn);
        return 0;
    }
    if (c.format > CLIDE_SAVE_FORMAT) {
        str_copy(err, "save is from a newer version", errn);
        return 0;
    }
    if (!game_create(g, name, dir, c.seed, c.mode, c.cheats, tcfg)) {
        str_copy(err, "cannot open world data", errn);
        return 0;
    }
    g->world.replace = p->fs.replace;
    inv_clear(&g->inv);
    c.phase = 1;
    ini_parse_file(path, load_cb, &c);
    if (c.unknown_items) {
        log_addf(ATTR(C_YELLOW, C_BLACK), "%d unknown items skipped", c.unknown_items);
    }
    g->last_minute = ctime_total_minutes(&g->time);
    g->unsaved = 0;
    return 1;
}

void save_list(SaveList *l)
{
    char path[64];
    char line[32];
    FILE *f;
    l->count = 0;
    str_copy(path, SAVE_ROOT, sizeof(path));
    str_cat(path, "/worlds.lst", sizeof(path));
    f = fopen(path, "r");
    if (!f) {
        return;
    }
    while (l->count < SAVE_MAX_WORLDS && fgets(line, sizeof(line), f)) {
        char *s = str_trim(line);
        if (save_valid_name(s) && save_exists(s)) {
            str_copy(l->names[l->count++], s, WORLD_NAME_MAX + 1);
        }
    }
    fclose(f);
}

int save_register(const ClidePlatform *p, const char *name)
{
    SaveList l;
    char path[64];
    FILE *f;
    int i;
    save_list(&l);
    for (i = 0; i < l.count; ++i) {
        if (str_ieq(l.names[i], name)) {
            return 1;
        }
    }
    p->fs.make_dir(SAVE_ROOT);
    str_copy(path, SAVE_ROOT, sizeof(path));
    str_cat(path, "/worlds.lst", sizeof(path));
    f = fopen(path, "a");
    if (!f) {
        return 0;
    }
    fprintf(f, "%s\n", name);
    fclose(f);
    return 1;
}
