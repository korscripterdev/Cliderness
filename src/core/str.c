/* Cliderness - core/str.c */
#include <string.h>
#include <ctype.h>
#include "core/str.h"

void str_copy(char *dst, const char *src, size_t n)
{
    size_t i = 0;
    if (n == 0) {
        return;
    }
    if (src) {
        while (i + 1 < n && src[i]) {
            dst[i] = src[i];
            ++i;
        }
    }
    dst[i] = '\0';
}

void str_cat(char *dst, const char *src, size_t n)
{
    size_t len = strlen(dst);
    if (len >= n) {
        return;
    }
    str_copy(dst + len, src, n - len);
}

static int lower(int c)
{
    return tolower((unsigned char)c);
}

int str_ieq(const char *a, const char *b)
{
    while (*a && *b) {
        if (lower(*a) != lower(*b)) {
            return 0;
        }
        ++a;
        ++b;
    }
    return *a == *b;
}

int str_iprefix(const char *s, const char *prefix)
{
    while (*prefix) {
        if (!*s || lower(*s) != lower(*prefix)) {
            return 0;
        }
        ++s;
        ++prefix;
    }
    return 1;
}

int str_icontains(const char *hay, const char *needle)
{
    if (!*needle) {
        return 1;
    }
    for (; *hay; ++hay) {
        if (str_iprefix(hay, needle)) {
            return 1;
        }
    }
    return 0;
}

char *str_trim(char *s)
{
    char *e;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
        ++s;
    }
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) {
        --e;
    }
    *e = '\0';
    return s;
}

void str_lower(char *s)
{
    for (; *s; ++s) {
        *s = (char)lower(*s);
    }
}

int str_to_long(const char *s, long *out)
{
    long v = 0;
    int neg = 0;
    int digits = 0;
    while (*s == ' ') {
        ++s;
    }
    if (*s == '-') {
        neg = 1;
        ++s;
    } else if (*s == '+') {
        ++s;
    }
    while (*s >= '0' && *s <= '9') {
        if (v > 200000000L) {
            return 0;
        }
        v = v * 10 + (*s - '0');
        ++s;
        ++digits;
    }
    while (*s == ' ') {
        ++s;
    }
    if (!digits || *s) {
        return 0;
    }
    *out = neg ? -v : v;
    return 1;
}

void str_from_long(char *dst, long v)
{
    char tmp[16];
    int i = 0;
    int j = 0;
    unsigned long u;
    if (v < 0) {
        dst[j++] = '-';
        u = (unsigned long)(-(v + 1)) + 1UL;
    } else {
        u = (unsigned long)v;
    }
    do {
        tmp[i++] = (char)('0' + (int)(u % 10UL));
        u /= 10UL;
    } while (u && i < 15);
    while (i > 0) {
        dst[j++] = tmp[--i];
    }
    dst[j] = '\0';
}

void str_subst(char *dst, size_t n, const char *tmpl, const char *const *args, int nargs)
{
    size_t o = 0;
    if (n == 0) {
        return;
    }
    while (*tmpl && o + 1 < n) {
        if (tmpl[0] == '{' && tmpl[1] >= '0' && tmpl[1] <= '9' && tmpl[2] == '}') {
            int idx = tmpl[1] - '0';
            const char *a = (idx < nargs && args[idx]) ? args[idx] : "";
            while (*a && o + 1 < n) {
                dst[o++] = *a++;
            }
            tmpl += 3;
        } else {
            dst[o++] = *tmpl++;
        }
    }
    dst[o] = '\0';
}

long utf8_next(const char **p)
{
    const unsigned char *s = (const unsigned char *)*p;
    long cp;
    int extra;
    int i;
    if (!*s) {
        return -1;
    }
    if (s[0] < 0x80) {
        *p += 1;
        return (long)s[0];
    }
    if ((s[0] & 0xE0) == 0xC0) {
        cp = s[0] & 0x1F;
        extra = 1;
    } else if ((s[0] & 0xF0) == 0xE0) {
        cp = s[0] & 0x0F;
        extra = 2;
    } else if ((s[0] & 0xF8) == 0xF0) {
        cp = s[0] & 0x07;
        extra = 3;
    } else {
        *p += 1;
        return 0xFFFDL;
    }
    for (i = 1; i <= extra; ++i) {
        if ((s[i] & 0xC0) != 0x80) {
            *p += i;
            return 0xFFFDL;
        }
        cp = (cp << 6) | (long)(s[i] & 0x3F);
    }
    *p += extra + 1;
    return cp;
}

int utf8_encode(long cp, char *out)
{
    if (cp < 0x80L) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800L) {
        out[0] = (char)(0xC0 | (int)(cp >> 6));
        out[1] = (char)(0x80 | (int)(cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000L) {
        out[0] = (char)(0xE0 | (int)(cp >> 12));
        out[1] = (char)(0x80 | (int)((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (int)(cp & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (int)(cp >> 18));
    out[1] = (char)(0x80 | (int)((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | (int)((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (int)(cp & 0x3F));
    return 4;
}

/* East Asian Wide / Fullwidth ranges relevant to the supported languages. */
int uni_width(long cp)
{
    if (cp < 0x1100L) {
        return 1;
    }
    if ((cp >= 0x1100L && cp <= 0x115FL) ||
        (cp >= 0x2E80L && cp <= 0x303EL) ||
        (cp >= 0x3041L && cp <= 0x33FFL) ||
        (cp >= 0x3400L && cp <= 0x4DBFL) ||
        (cp >= 0x4E00L && cp <= 0x9FFFL) ||
        (cp >= 0xA000L && cp <= 0xA4CFL) ||
        (cp >= 0xAC00L && cp <= 0xD7A3L) ||
        (cp >= 0xF900L && cp <= 0xFAFFL) ||
        (cp >= 0xFE30L && cp <= 0xFE4FL) ||
        (cp >= 0xFF00L && cp <= 0xFF60L) ||
        (cp >= 0xFFE0L && cp <= 0xFFE6L)) {
        return 2;
    }
    return 1;
}

int utf8_width(const char *s)
{
    int w = 0;
    long cp;
    while ((cp = utf8_next(&s)) >= 0) {
        w += uni_width(cp);
    }
    return w;
}
