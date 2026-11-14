/*
 * Cliderness - core/log.h
 * In-game event log (bottom of the screen). Fixed ring buffer, no allocation.
 * Identical consecutive messages are merged with a repeat counter.
 */
#ifndef CLIDE_CORE_LOG_H
#define CLIDE_CORE_LOG_H

#include "core/types.h"

#define LOG_LINES 48
#define LOG_TEXT  112

typedef struct LogEntry {
    char  text[LOG_TEXT];
    cl_u8 attr;
    int   repeat;
} LogEntry;

void log_clear(void);
void log_add(cl_u8 attr, const char *text);
void log_addf(cl_u8 attr, const char *fmt, ...);
int  log_count(void);
/* 0 = newest */
const LogEntry *log_get(int age);

#endif
