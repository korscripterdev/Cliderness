/*
 * Cliderness - world/sky/sky.h  (ClideSky)
 * Procedural sky: sun, moon phases, seeded stars and constellations, the
 * Milky Way, wind-driven clouds, birds, crows, meteors, comets, aurora,
 * eclipses, sunrise/sunset colours and lightning. Everything is a pure
 * function of (world seed, game time, game tick, weather) so it freezes
 * exactly when the simulation is paused.
 */
#ifndef CLIDE_WORLD_SKY_H
#define CLIDE_WORLD_SKY_H

#include "renderer/fb.h"
#include "gameplay/game.h"

#define SKY_CONSTELLATIONS 8

typedef struct SkyInfo {
    int sun_visible, moon_visible, stars_visible;
    int meteor, comet, aurora, solar_eclipse, lunar_eclipse, birds, crows;
    /* the Ancient Dimension: second moon (3-day cycle), the ringed world,
     * the aether veil */
    int second_moon, ringed_world, aether_veil;
} SkyInfo;

/* phase 0..7 of the Ancient Dimension's second moon (Veyra) */
int sky_veyra_phase(const Game *g);

void sky_render(Framebuffer *fb, Game *g, int x0, int y0, int w, int h, int show_names, SkyInfo *info);
const char *sky_constellation_name(cl_u32 seed, int i, char *buf, int n);

#endif
