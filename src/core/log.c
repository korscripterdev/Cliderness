/* Cliderness - core/log.c */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "core/log.h"
#include "core/str.h"

static LogEntry g_log[LOG_LINES];
static int g_head = 0;
static int g_count = 0;

void log_clear(void)
{
    g_head = 0;
    g_count = 0;
}

void log_add(cl_u8 attr, const char *text)
{
    LogEntry *e;
    if (g_count > 0) {
        e = &g_log[(g_head + LOG_LINES - 1) % LOG_LINES];
        if (e->attr == attr && strcmp(e->text, text) == 0) {
            ++e->repeat;
            return;
        }
    }
    e = &g_log[g_head];
    str_copy(e->text, text, sizeof(e->text));
    e->attr = attr;
    e->repeat = 1;
    g_head = (g_head + 1) % LOG_LINES;
    if (g_count < LOG_LINES) {
        ++g_count;
    }
}

void log_addf(cl_u8 attr, const char *fmt, ...)
{
    /* Callers only pass short formats with bounded arguments. Long %s
     * arguments must be pre-truncated by the caller (C90 has no vsnprintf). */
    static char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    log_add(attr, buf);
}

int log_count(void)
{
    return g_count;
}

const LogEntry *log_get(int age)
{
    if (age < 0 || age >= g_count) {
        return NULL;
    }
    return &g_log[(g_head + LOG_LINES - 1 - age) % LOG_LINES];
}
