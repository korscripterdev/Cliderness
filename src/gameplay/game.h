/*
 * Cliderness - gameplay/game.h
 * The simulation of one open world session. UI code calls the game_* action
 * functions; the main loop calls game_tick() at a fixed 20 ticks per second
 * only while no pausing screen is open.
 */
#ifndef CLIDE_GAMEPLAY_GAME_H
#define CLIDE_GAMEPLAY_GAME_H

#include "core/types.h"
#include "core/rng.h"
#include "world/world.h"
#include "world/time/clidetime.h"
#include "world/weather/weather.h"
#include "gameplay/player/player.h"
#include "gameplay/inventory/inventory.h"
#include "gameplay/entities/entity.h"
#include "gameplay/building/edit.h"

#define TICK_MS 50L
#define TICKS_PER_SECOND 20

enum { GM_SURVIVAL = 0, GM_CREATIVE, GM_ADVENTURE, GM_SPECTATOR, GM_COUNT };

typedef struct Game {
    int         active;
    char        name[16];
    char        dir[128];
    cl_u32      seed;
    int         mode;
    int         cheats;        /* cheat commands allowed */
    int         cheated;       /* a cheat was ever used */
    World       world;
    Player      p;
    Inventory   inv;
    ItemEntity  ents[MAX_ITEM_ENTS];
    ClideTime   time;
    WeatherState weather;
    EditJournal journal;
    ClideRng    rng;
    long        tick;
    long        last_minute;
    int         blocked_msg;   /* rate limit for repeated messages */
    int         unsaved;
} Game;

extern Game g_game;

/* lifecycle; dir NULL = memory only world (tests) */
int  game_create(Game *g, const char *name, const char *dir, cl_u32 seed, int mode, int cheats,
                 const ClideTimeCfg *tcfg);
void game_destroy(Game *g);
void game_tick(Game *g);

/* player actions */
void game_request_move(Game *g, int dir, int sprint);
void game_toggle_crouch(Game *g);
void game_mine(Game *g);
void game_dig_down(Game *g);
void game_place(Game *g);
void game_build_up(Game *g);
void game_interact(Game *g);
void game_drop(Game *g, int slot, int whole_stack);
void game_use(Game *g);
void game_select(Game *g, int slot);
void game_set_mode(Game *g, int mode);
void game_teleport(Game *g, long x, long y, long z);
void game_respawn(Game *g);

/* helpers shared by UI / commands */
int  game_light_radius(Game *g);
int  game_is_covered(Game *g, long x, long y, long z);
int  game_mode_find(const char *name);
const char *game_mode_key(int mode);
int  game_can_edit_world(const Game *g);

/* entities */
int  ent_spawn(Game *g, int dim, long x, long y, long z, int item, int count, int dur);
void ent_tick(Game *g);
int  ent_pickup(Game *g, long x, long y, long z);
int  ent_count(const Game *g);
int  ent_kill_all(Game *g);

/* survival (once per game minute) */
void surv_minute(Game *g);
void surv_tick(Game *g);
int  surv_env_temp(Game *g);
void surv_damage(Game *g, int amount, const char *cause_key);
void skill_gain(Game *g, int skill, int xp);

/* edit journal */
int  edit_init(EditJournal *j);
void edit_free(EditJournal *j);
void edit_begin(EditJournal *j);
void edit_set(Game *g, int dim, long x, long y, long z, cl_u8 b, cl_u8 m);
int  edit_end(EditJournal *j);   /* 1 = undoable op recorded */
int  edit_undo(Game *g);         /* returns records restored, -1 nothing */
int  edit_redo(Game *g);

/* mining / placing internals */
int  mine_time(Game *g, int block);
void mine_finish(Game *g);

#endif
