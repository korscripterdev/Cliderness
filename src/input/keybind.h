/*
 * Cliderness - input/keybind.h
 * Rebindable actions. Every action has up to two key specs, stored in
 * keys.ini. Letters match case-insensitively; Shift on a movement key means
 * sprint. "RShift" / "Ctrl" are bare modifier taps and only work where the
 * platform reports them (Windows, DOS); every such action also has a normal
 * fallback key so terminals without modifier reporting stay fully playable.
 */
#ifndef CLIDE_INPUT_KEYBIND_H
#define CLIDE_INPUT_KEYBIND_H

#include "input/keys.h"

enum {
    ACT_NONE = 0,
    ACT_MOVE_N, ACT_MOVE_W, ACT_MOVE_S, ACT_MOVE_E,
    ACT_CROUCH, ACT_SKY, ACT_INVENTORY, ACT_DROP, ACT_DROP_STACK, ACT_STATS, ACT_INTERACT,
    ACT_HOT1, ACT_HOT2, ACT_HOT3, ACT_HOT4, ACT_HOT5, ACT_HOT6, ACT_HOT7, ACT_HOT8, ACT_HOT9,
    ACT_HELP, ACT_PAUSE, ACT_CONSOLE, ACT_MINE, ACT_PLACE, ACT_DIG_DOWN, ACT_BUILD_UP,
    ACT_USE, ACT_PANEL,
    ACT_COUNT
};

typedef struct KeySpec {
    long key;
    int  mods;   /* KM_CTRL / KM_ALT requirements */
} KeySpec;

extern KeySpec g_binds[ACT_COUNT][2];

void kb_defaults(void);
int  kb_load(const char *path);
int  kb_save(const char *path);
/* returns ACT_* for an event; *sprint = 1 when Shift was held on movement */
int  kb_action(const ClideKeyEvent *ev, int *sprint);
int  kb_parse(const char *s, KeySpec *out);
void kb_format(const KeySpec *k, char *out, int n);
void kb_describe(int action, char *out, int n);   /* "W / Up" */
const char *kb_action_id(int action);
const char *kb_action_key(int action);            /* localisation key */
int  kb_spec_matches(const KeySpec *k, const ClideKeyEvent *ev);

#endif
