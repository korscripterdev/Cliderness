/*
 * Cliderness - localization/locale.h  (ClideLocale AutoDetect)
 * String tables in data/locale/<code>.lng (UTF-8, key=value).
 * English is always loaded as the fallback table.
 */
#ifndef CLIDE_LOCALE_H
#define CLIDE_LOCALE_H

#include "core/types.h"

#define LOC_MAX_LANGS 16

/* display status of a language on the current terminal */
#define LANG_OK          1   /* verified displayable */
#define LANG_UNVERIFIED  0   /* probably displayable, needs user confirmation */
#define LANG_UNAVAILABLE -1  /* cannot be displayed (e.g. Hangul on CP437 DOS) */

typedef struct LangInfo {
    char code[8];
    char name[32];
    char native[48];
    char script[12];
    char sample[64];
    int  status;
    int  confirmed;
} LangInfo;

/* Scan <root>/locale/index.lst, read headers and rate each language for the
 * terminal capabilities (TERMCAP_* flags). Loads English. */
int  loc_init(const char *root, cl_u32 termcaps);
void loc_shutdown(void);
int  loc_set(const char *code);
const char *loc_current(void);
const char *loc_get(const char *key);
const char *loc_get_or(const char *key, const char *fallback);
/* key lookup with {0}.. substitution into a static rotating buffer */
const char *loc_fmt(const char *key, const char *a0, const char *a1, const char *a2);
int  loc_has(const char *key);

int  loc_lang_count(void);
LangInfo *loc_lang(int i);
int  loc_find(const char *code);
/* Pick the best language for a detected OS language code. */
const char *loc_choose(const char *detected);

/* Translation coverage: number of keys of the English table missing in
 * <code> (prefix "x." = only those keys, "!x." = all but those).
 * If out is not NULL, the first missing keys are written there,
 * one per line. Returns -1 if the language cannot be loaded. */
int  loc_missing(const char *code, const char *prefix, char *out, size_t n);
int  loc_key_count(void);

#endif
