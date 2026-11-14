/* Cliderness - localization/locale.c */
#include <stdio.h>
#include <string.h>
#include "localization/locale.h"
#include "platform/platform.h"
#include "renderer/cp437.h"
#include "core/ini.h"
#include "core/str.h"
#include "core/mem.h"
#include "core/rng.h"

#ifdef CLIDE_TARGET_DOS16
#define LOC_POOL    24000U
#define LOC_ENTRIES 640
#else
#define LOC_POOL    60000U
#define LOC_ENTRIES 1400
#endif

typedef struct LocEntry {
    cl_u16 hash;
    cl_u16 key;
    cl_u16 val;
} LocEntry;

typedef struct LocTable {
    LocEntry *e;
    char     *pool;
    unsigned  used;
    int       count;
    int       overflow;
} LocTable;

static LocTable g_base, g_cur;
static int g_has_cur = 0;
static char g_code[8] = "en";
static char g_root[128] = "data";
static LangInfo g_langs[LOC_MAX_LANGS];
static int g_nlangs = 0;

static cl_u16 h16(const char *s)
{
    return (cl_u16)(rng_hash_str(s) & 0xFFFFUL);
}

static int table_alloc(LocTable *t)
{
    t->e = (LocEntry *)mem_alloc(sizeof(LocEntry) * LOC_ENTRIES);
    t->pool = (char *)mem_alloc(LOC_POOL);
    t->used = 0;
    t->count = 0;
    t->overflow = 0;
    return t->e && t->pool;
}

static void table_free(LocTable *t)
{
    mem_free(t->e);
    mem_free(t->pool);
    t->e = NULL;
    t->pool = NULL;
    t->count = 0;
}

static const char *table_get(const LocTable *t, const char *key)
{
    cl_u16 h = h16(key);
    int i;
    for (i = 0; i < t->count; ++i) {
        if (t->e[i].hash == h && strcmp(t->pool + t->e[i].key, key) == 0) {
            return t->pool + t->e[i].val;
        }
    }
    return NULL;
}

static unsigned pool_add(LocTable *t, const char *s)
{
    unsigned len = (unsigned)strlen(s) + 1;
    unsigned off;
    if (t->used + len > LOC_POOL) {
        t->overflow = 1;
        return 0xFFFFU;
    }
    off = t->used;
    memcpy(t->pool + off, s, len);
    t->used += len;
    return off;
}

/* unescape \n */
static void unescape(char *s)
{
    char *o = s;
    while (*s) {
        if (s[0] == '\\' && s[1] == 'n') {
            *o++ = '\n';
            s += 2;
        } else {
            *o++ = *s++;
        }
    }
    *o = '\0';
}

typedef struct LoadCtx {
    LocTable *t;
    LangInfo *info;
} LoadCtx;

static int load_cb(void *user, const char *type, const char *id, const char *key,
                   const char *value, int line)
{
    LoadCtx *c = (LoadCtx *)user;
    static char val[512];
    unsigned k, v;
    (void)type;
    (void)id;
    (void)line;
    if (!key) {
        return 0;
    }
    if (key[0] == '@') {
        if (c->info) {
            if (str_ieq(key, "@name")) str_copy(c->info->name, value, sizeof(c->info->name));
            else if (str_ieq(key, "@native")) str_copy(c->info->native, value, sizeof(c->info->native));
            else if (str_ieq(key, "@script")) str_copy(c->info->script, value, sizeof(c->info->script));
            else if (str_ieq(key, "@sample")) str_copy(c->info->sample, value, sizeof(c->info->sample));
        }
        return 0;
    }
    if (!c->t) {
        return 0;
    }
    if (c->t->count >= LOC_ENTRIES) {
        c->t->overflow = 1;
        return 0;
    }
    str_copy(val, value, sizeof(val));
    unescape(val);
    k = pool_add(c->t, key);
    v = pool_add(c->t, val);
    if (k == 0xFFFFU || v == 0xFFFFU) {
        return 0;
    }
    c->t->e[c->t->count].hash = h16(key);
    c->t->e[c->t->count].key = (cl_u16)k;
    c->t->e[c->t->count].val = (cl_u16)v;
    ++c->t->count;
    return 0;
}

static void lang_path(char *out, size_t n, const char *code)
{
    str_copy(out, g_root, n);
    str_cat(out, "/locale/", n);
    str_cat(out, code, n);
    str_cat(out, ".lng", n);
}

static int load_table(LocTable *t, const char *code, LangInfo *info)
{
    char path[160];
    LoadCtx c;
    lang_path(path, sizeof(path), code);
    c.t = t;
    c.info = info;
    if (t) {
        t->used = 0;
        t->count = 0;
        t->overflow = 0;
    }
    return ini_parse_file(path, load_cb, &c) == 0;
}

/* CP437-only terminals: language usable only if every string fits CP437 */
static int check_cp437(const char *code)
{
    LocTable t;
    int i, ok = 1;
    if (!table_alloc(&t)) {
        table_free(&t);
        return 0;
    }
    if (!load_table(&t, code, NULL)) {
        ok = 0;
    }
    for (i = 0; ok && i < t.count; ++i) {
        if (!cp437_can_show(t.pool + t.e[i].val)) {
            ok = 0;
        }
    }
    table_free(&t);
    return ok;
}

int loc_init(const char *root, cl_u32 termcaps)
{
    char path[160];
    char line[32];
    FILE *f;
    str_copy(g_root, root, sizeof(g_root));
    g_nlangs = 0;
    str_copy(path, root, sizeof(path));
    str_cat(path, "/locale/index.lst", sizeof(path));
    f = fopen(path, "r");
    if (f) {
        while (g_nlangs < LOC_MAX_LANGS && fgets(line, sizeof(line), f)) {
            char *s = str_trim(line);
            LangInfo *li;
            if (!*s || *s == '#') {
                continue;
            }
            li = &g_langs[g_nlangs];
            memset(li, 0, sizeof(*li));
            str_copy(li->code, s, sizeof(li->code));
            str_copy(li->name, s, sizeof(li->name));
            if (!load_table(NULL, li->code, li)) {
                continue;
            }
            if (termcaps & TERMCAP_UNICODE) {
                li->status = (str_ieq(li->script, "latin") || str_ieq(li->script, "cyrillic") ||
                              str_ieq(li->script, "greek")) ? LANG_OK : LANG_UNVERIFIED;
            } else {
                li->status = check_cp437(li->code) ? LANG_OK : LANG_UNAVAILABLE;
            }
            ++g_nlangs;
        }
        fclose(f);
    }
    if (!g_base.e && !table_alloc(&g_base)) {
        return 0;
    }
    if (!load_table(&g_base, "en", NULL)) {
        return 0;
    }
    str_copy(g_code, "en", sizeof(g_code));
    return 1;
}

void loc_shutdown(void)
{
    table_free(&g_base);
    if (g_has_cur) {
        table_free(&g_cur);
        g_has_cur = 0;
    }
}

int loc_set(const char *code)
{
    if (str_ieq(code, "en")) {
        if (g_has_cur) {
            table_free(&g_cur);
            g_has_cur = 0;
        }
        str_copy(g_code, "en", sizeof(g_code));
        return 1;
    }
    if (loc_find(code) < 0) {
        return 0;
    }
    if (!g_has_cur) {
        if (!table_alloc(&g_cur)) {
            table_free(&g_cur);
            return 0;
        }
        g_has_cur = 1;
    }
    if (!load_table(&g_cur, code, NULL)) {
        table_free(&g_cur);
        g_has_cur = 0;
        return 0;
    }
    str_copy(g_code, g_langs[loc_find(code)].code, sizeof(g_code));
    return 1;
}

const char *loc_current(void)
{
    return g_code;
}

const char *loc_get_or(const char *key, const char *fallback)
{
    const char *v = NULL;
    if (g_has_cur) {
        v = table_get(&g_cur, key);
    }
    if (!v && g_base.e) {
        v = table_get(&g_base, key);
    }
    return v ? v : fallback;
}

const char *loc_get(const char *key)
{
    return loc_get_or(key, key);
}

int loc_has(const char *key)
{
    return loc_get_or(key, NULL) != NULL;
}

const char *loc_fmt(const char *key, const char *a0, const char *a1, const char *a2)
{
    static char bufs[4][256];
    static int which = 0;
    const char *args[3];
    char *b = bufs[which];
    which = (which + 1) & 3;
    args[0] = a0;
    args[1] = a1;
    args[2] = a2;
    str_subst(b, sizeof(bufs[0]), loc_get(key), args, 3);
    return b;
}

int loc_lang_count(void)
{
    return g_nlangs;
}

LangInfo *loc_lang(int i)
{
    return (i >= 0 && i < g_nlangs) ? &g_langs[i] : NULL;
}

int loc_find(const char *code)
{
    int i;
    for (i = 0; i < g_nlangs; ++i) {
        if (str_ieq(g_langs[i].code, code)) {
            return i;
        }
    }
    return -1;
}

const char *loc_choose(const char *detected)
{
    int i = loc_find(detected);
    if (i < 0 && strlen(detected) > 2) {
        char two[3];
        two[0] = detected[0];
        two[1] = detected[1];
        two[2] = '\0';
        i = loc_find(two);
    }
    /* An OS configured for a language very likely has fonts for it, so an
     * unverified language is accepted here; unavailable ones are not. */
    if (i >= 0 && g_langs[i].status != LANG_UNAVAILABLE) {
        return g_langs[i].code;
    }
    return "en";
}

int loc_key_count(void)
{
    return g_base.count;
}

int loc_missing(const char *code, const char *prefix, char *out, size_t n)
{
    LocTable t;
    int i, missing = 0;
    if (out && n) {
        out[0] = '\0';
    }
    if (!table_alloc(&t)) {
        table_free(&t);
        return -1;
    }
    if (!load_table(&t, code, NULL)) {
        table_free(&t);
        return -1;
    }
    for (i = 0; i < g_base.count; ++i) {
        const char *key = g_base.pool + g_base.e[i].key;
        /* "abc." = only keys with that prefix, "!abc." = all keys except those */
        if (prefix && prefix[0] == '!') {
            if (str_iprefix(key, prefix + 1)) {
                continue;
            }
        } else if (prefix && *prefix && !str_iprefix(key, prefix)) {
            continue;
        }
        if (!table_get(&t, key)) {
            ++missing;
            if (out && strlen(out) + strlen(key) + 2 < n) {
                str_cat(out, key, n);
                str_cat(out, "\n", n);
            }
        }
    }
    table_free(&t);
    return missing;
}
