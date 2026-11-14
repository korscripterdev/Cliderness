/*
 * Cliderness - core/str.h
 * Bounded string helpers and UTF-8 utilities (C90 has no snprintf).
 * Code points are long because int may be 16 bits.
 */
#ifndef CLIDE_CORE_STR_H
#define CLIDE_CORE_STR_H

#include "core/types.h"

void  str_copy(char *dst, const char *src, size_t n);
void  str_cat(char *dst, const char *src, size_t n);
int   str_ieq(const char *a, const char *b);
int   str_iprefix(const char *s, const char *prefix);
int   str_icontains(const char *hay, const char *needle);
char *str_trim(char *s);
void  str_lower(char *s);
int   str_to_long(const char *s, long *out);
void  str_from_long(char *dst, long v);
/* Replace {0}..{9} in tmpl with args. Always terminates dst. */
void  str_subst(char *dst, size_t n, const char *tmpl, const char *const *args, int nargs);

/* UTF-8. utf8_next returns the code point and advances *p; -1 at end. */
long  utf8_next(const char **p);
int   utf8_encode(long cp, char *out);
int   uni_width(long cp);
int   utf8_width(const char *s);

#endif
