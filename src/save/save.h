/*
 * Cliderness - save/save.h
 * World saves: saves/<name>/world.ini (text, CRC-32 protected),
 * saves/<name>/chunks.dat (modified chunks only, see chunkstore.h).
 * Every file is written to a temporary file, read back and verified, and
 * only then atomically replaces the previous save.
 */
#ifndef CLIDE_SAVE_H
#define CLIDE_SAVE_H

#include "gameplay/game.h"
#include "platform/platform.h"

#define SAVE_ROOT "saves"
#define SAVE_MAX_WORLDS 32
#define WORLD_NAME_MAX 8   /* DOS 8.3 directory names */

typedef struct SaveList {
    int  count;
    char names[SAVE_MAX_WORLDS][WORLD_NAME_MAX + 1];
} SaveList;

int  save_valid_name(const char *name);
void save_world_dir(const char *name, char *out, size_t n);
int  save_exists(const char *name);
/* create directories for a new world; returns 1 on success */
int  save_prepare(const ClidePlatform *p, const char *name);
int  save_game(Game *g, const ClidePlatform *p, char *err, size_t errn);
int  save_load(Game *g, const ClidePlatform *p, const char *name, const ClideTimeCfg *tcfg,
               char *err, size_t errn);
void save_list(SaveList *l);
int  save_register(const ClidePlatform *p, const char *name);
/* verify CRC of a text save file; 1 = valid */
int  save_verify_file(const char *path);

#endif
