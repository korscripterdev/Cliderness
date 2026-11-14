/*
 * Cliderness - core/ini.h
 * Streaming parser for the data/definition format used by every content file:
 *
 *   # comment
 *   [type id]      -> section_type="type", section_id="id"
 *   key = value
 *
 * The file is read line by line (no whole-file buffering, DOS friendly).
 * The callback receives key == NULL once at the start of each section.
 */
#ifndef CLIDE_CORE_INI_H
#define CLIDE_CORE_INI_H

typedef int (*IniCallback)(void *user, const char *type, const char *id,
                           const char *key, const char *value, int line);

/* Returns 0 on success, -1 if the file cannot be opened,
 * or the (positive) line number where the callback reported an error. */
int ini_parse_file(const char *path, IniCallback cb, void *user);

#endif
