/*
 * Cliderness - gameplay/vehicles/ship.h
 * Data-driven ships (data/vehicles/ships.def): rafts, rowing boats, sailing
 * boats and large ships.
 *
 * A ship floats in the air cell directly above a water cell. Its hull is a
 * line of `length` cells along its heading (large ships are 3 cells wide).
 *
 * Model (integer only, evaluated every tick / every second):
 *   speed (cells per 100 ticks) = max(oars, sails)
 *     oars  = oar_speed x throttle / 3          (costs stamina per cell)
 *     sails = sail_speed x throttle / 3 x wind% x angle% / 10000
 *             angle% = 100 tailwind, 75 broad reach, 45 beam, 15 headwind
 *     x (100 + Sailing level) / 100, halved when overloaded,
 *     x (100 - flooding% / 2) / 100
 *   load      = cargo + the player's inventory + 75 kg per person aboard
 *   flooding  = water in the hull, 0..capacity/4 litres; the ship sinks at
 *               100%, or when load + water exceeds 150% of capacity
 *   damage    = 3 stages by hull %: intact (>66), damaged (34-66: leaks
 *               1 l/s), critical (<=33: leaks 4 l/s)
 *   waves     = while wind% > wave_limit the hull takes 1 damage and
 *               (wind - limit) / 10 litres with chance (wind - limit)% / s
 *   aground   = moving into land stops the ship; above 30 cells/100 ticks it
 *               costs speed / 20 hull
 */
#ifndef CLIDE_GAMEPLAY_SHIP_H
#define CLIDE_GAMEPLAY_SHIP_H

#include "core/types.h"

#define MAX_SHIP_DEFS 8
#define CHEST_DIM_SHIP 200   /* cargo holds are virtual chests in dim 200 + ship */

enum { SHIP_INTACT = 0, SHIP_DAMAGED, SHIP_CRITICAL };

typedef struct ShipDef {
    char   id[24];
    char   name[28];
    cl_s16 item;           /* item that places the ship */
    cl_u8  length, width;
    cl_u8  glyph, color, sail_glyph;
    cl_u8  crew;           /* people aboard (for the load) */
    cl_s16 hull;           /* max hull points */
    cl_s16 oar_speed, sail_speed;
    cl_s16 capacity;       /* kg */
    cl_u8  cargo;          /* cargo slots (0..27) */
    cl_s16 wave_limit;     /* wind % the hull can take */
    cl_s16 repair_item;
    cl_s16 wreck_item, wreck_count;
} ShipDef;

extern ShipDef g_ships[MAX_SHIP_DEFS];
extern int g_ship_count;

int  ships_load(const char *root, char *err, size_t errn);
int  shipdef_find(const char *id);
int  shipdef_for_item(int item);
const char *shipdef_name(int s);
const char *ship_stage_key(int stage);

#endif
