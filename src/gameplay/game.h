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
#include "audio/audio.h"

#define TICK_MS 50L
#define TICKS_PER_SECOND 20

enum { GM_SURVIVAL = 0, GM_CREATIVE, GM_ADVENTURE, GM_SPECTATOR, GM_COUNT };

#ifdef CLIDE_TARGET_DOS16
#define MAX_CROPS  64
#define MAX_CHESTS 16
#else
#define MAX_CROPS  512
#define MAX_CHESTS 128
#endif
#define CHEST_SLOTS 27
#ifdef CLIDE_TARGET_DOS16
#define MAX_MOBS 16
#else
#define MAX_MOBS 48
#endif

#ifdef CLIDE_TARGET_DOS16
#define MAX_SHIPS 4
#else
#define MAX_SHIPS 16
#endif

/* a ship on the water (definition in gameplay/vehicles/ship.h) */
typedef struct Ship {
    long   x, y, z;            /* centre cell (air above water) */
    long   moved;              /* cells travelled */
    cl_s16 def;
    cl_s16 hull;
    cl_s16 water;              /* litres in the hull */
    cl_s16 acc;                /* movement accumulator (100 = one cell) */
    cl_s16 chest;              /* cargo hold (virtual chest) or -1 */
    cl_u8  dim, active, heading, anchored;
    signed char throttle;      /* -1 reverse .. 3 full */
} Ship;

#define MAX_WAYPOINTS 16

typedef struct Waypoint {
    char  name[16];
    long  x, y, z;
    cl_u8 dim, active;
} Waypoint;

enum { DIFF_PEACEFUL = 0, DIFF_EASY, DIFF_NORMAL, DIFF_HARD, DIFF_COUNT };

/* a living creature (definition in gameplay/entities/creature.h) */
typedef struct Mob {
    long   x, y, z;
    long   home_x, home_z;
    cl_s16 def;
    cl_s16 hp;
    cl_s16 think, cd_move, cd_attack;
    cl_u8  dim, active, state, aggro;
} Mob;

/* a planted crop or sapling waiting for its next growth stage */
typedef struct Crop {
    long  x, y, z;
    long  next_minute;
    cl_u8 dim, active;
} Crop;

typedef struct Chest {
    long  x, y, z;
    cl_u8 dim, active;
    ItemStack s[CHEST_SLOTS];
} Chest;

/* requests from game logic to the UI (opened after an interaction) */
enum { UIREQ_NONE = 0, UIREQ_CRAFT, UIREQ_CHEST };

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
    Crop        crops[MAX_CROPS];
    Chest       chests[MAX_CHESTS];
    int         ui_request;
    int         ui_arg;
    Mob         mobs[MAX_MOBS];
    int         difficulty;
    int         spawning;      /* natural creature spawning on */
    long        kills;
    Ship        ships[MAX_SHIPS];
    int         aboard;        /* ship the player is on, -1 = none */
    int         sfx;           /* sound requested by the simulation (audio.h SFX_*) */
    Waypoint    wp[MAX_WAYPOINTS];
    int         ai_off;        /* creature AI frozen (/ai off) */
    long        deaths;
    long        death_x, death_y, death_z;
    int         death_dim;
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
/* creatures (gameplay/entities/mob.c): 1 if a hostile creature is within radius */
int  ent_hostile_near(Game *g, int radius);
/* interact with a creature in a cell (trade / look); 1 if handled */
int  ent_interact(Game *g, long x, long y, long z);
int  mob_spawn(Game *g, int def, int dim, long x, long y, long z);
int  mob_at(Game *g, int dim, long x, long y, long z);
int  mob_count(const Game *g);
int  mob_damage(Game *g, int idx, int dmg, int skill);   /* 0 none, 1 hit, 2 killed */
int  mob_kill_type(Game *g, int def);                    /* def -1 = all */
void mob_tick(Game *g);
void mob_spawn_tick(Game *g);
int  game_attack(Game *g);   /* 1 if an attack happened (or was on cooldown) */
const char *difficulty_key(int d);
int  difficulty_find(const char *name);

/* ships (gameplay/vehicles/ship.c) */
int  ship_spawn(Game *g, int def, int dim, long x, long y, long z, int heading, int hull);
void ship_remove(Game *g, int idx, int wreck);
int  ship_at(Game *g, int dim, long x, long y, long z);
int  ship_covers(Game *g, int idx, long x, long z);
int  ship_cell_role(Game *g, int idx, long x, long z);
int  ship_count(const Game *g);
int  ship_stage(const Game *g, int idx);
long ship_max_water(const Game *g, int idx);
int  ship_flood_pct(const Game *g, int idx);
long ship_load(Game *g, int idx);
int  ship_wind_pct(Game *g, int idx);
int  ship_speed(Game *g, int idx);
int  ship_place(Game *g);
int  ship_pickup(Game *g, int idx);
int  ship_board(Game *g);
int  ship_disembark(Game *g);
void ship_steer(Game *g, int dir);
void ship_tick(Game *g);
int  ship_use(Game *g);
int  ship_open_cargo(Game *g);
int  ship_rowing(Game *g);

/* dimensions (gameplay/dimension/portal.c) */
const char *dim_key(int dim);
int  dim_find(const char *name);
/* move the player to dim `target` (same x/z, nearest land); 1 on success */
int  portal_travel(Game *g, int target, int build_return);   /* 1 while the player rows (no stamina regeneration) */

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

/* crafting (gameplay/crafting/crafting.c) */
enum { CRAFT_OK = 0, CRAFT_NEED_ITEMS, CRAFT_NEED_STATION, CRAFT_NEED_FUEL, CRAFT_NEED_SKILL, CRAFT_NO_ROOM };
int  craft_station_near(Game *g, int station);
int  craft_fuel_units(const Inventory *inv);
int  craft_check(Game *g, int recipe);
int  craft_max(Game *g, int recipe);
int  craft_do(Game *g, int recipe);
int  repair_check(Game *g, int slot);   /* 0 ok, else CRAFT_* reason */
int  repair_do(Game *g, int slot);
long game_carry_capacity(Game *g);

/* farming (gameplay/farming/farming.c) */
int  farm_till(Game *g);
int  farm_register(Game *g, int dim, long x, long y, long z);
void farm_minute(Game *g);
int  farm_grow_all(Game *g, long radius);
int  farm_count(const Game *g);

/* chests (gameplay/storage/chest.c) */
int  chest_find(const Game *g, int dim, long x, long y, long z);
int  chest_create(Game *g, int dim, long x, long y, long z);
void chest_remove(Game *g, int idx);
int  chest_count(const Game *g);
void chest_fill_loot(Game *g, int idx, const char *table);

/* fluids and rain (game.c) */
void game_rain_minute(Game *g);

/* status effects and sleep (gameplay/survival/effects.c) */
void fx_add(Game *g, int effect, int minutes);
int  fx_has(const Game *g, int effect);
int  fx_remove(Game *g, int effect);
void fx_clear(Game *g);
void fx_minute(Game *g);
int  fx_temp(const Game *g);
int  fx_move(const Game *g);
int  fx_regen(const Game *g);
int  game_sleep(Game *g, int force);

/* mining / placing internals */
int  mine_time(Game *g, int block);
void mine_finish(Game *g);

#endif
