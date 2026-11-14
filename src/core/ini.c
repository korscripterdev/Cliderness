/* Cliderness - core/ini.c */
#include <stdio.h>
#include <string.h>
#include "core/ini.h"
#include "core/str.h"

#define INI_LINE 512

int ini_parse_file(const char *path, IniCallback cb, void *user)
{
    static char line[INI_LINE];
    char type[48];
    char id[64];
    FILE *f;
    int lineno = 0;
    int result = 0;

    f = fopen(path, "r");
    if (!f) {
        return -1;
    }
    type[0] = '\0';
    id[0] = '\0';
    while (fgets(line, sizeof(line), f)) {
        char *s = line;
        char *eq;
        ++lineno;
        if (lineno == 1 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB &&
            (unsigned char)s[2] == 0xBF) {
            s += 3;
        }
        s = str_trim(s);
        if (!*s || *s == '#' || *s == ';') {
            continue;
        }
        if (*s == '[') {
            char *close = strchr(s, ']');
            char *sp;
            if (!close) {
                result = lineno;
                break;
            }
            *close = '\0';
            ++s;
            sp = strchr(s, ' ');
            if (sp) {
                *sp = '\0';
                str_copy(type, str_trim(s), sizeof(type));
                str_copy(id, str_trim(sp + 1), sizeof(id));
            } else {
                str_copy(type, str_trim(s), sizeof(type));
                id[0] = '\0';
            }
            if (cb(user, type, id, NULL, NULL, lineno) != 0) {
                result = lineno;
                break;
            }
            continue;
        }
        eq = strchr(s, '=');
        if (!eq) {
            result = lineno;
            break;
        }
        *eq = '\0';
        if (cb(user, type, id, str_trim(s), str_trim(eq + 1), lineno) != 0) {
            result = lineno;
            break;
        }
    }
    fclose(f);
    return result;
}
