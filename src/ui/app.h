/*
 * Cliderness - ui/app.h
 * Application shell: screen stack, settings, fixed-step main loop.
 *
 * Time model: the simulation runs in fixed 50 ms ticks fed from an
 * accumulator of real elapsed time. Real time is only accumulated while no
 * screen on the stack has `pauses` set (help, pause menu, main menu), so
 * time spent paused is never replayed after resuming.
 */
#ifndef CLIDE_UI_APP_H
#define CLIDE_UI_APP_H

#include "platform/platform.h"
#include "renderer/fb.h"
#include "input/keys.h"
#include "world/time/clidetime.h"

#define APP_STACK 8

typedef struct Screen {
    const char *name;
    int  pauses;
    int  opaque;
    void (*enter)(void);
    void (*key)(const ClideKeyEvent *ev);
    void (*draw)(Framebuffer *fb);
} Screen;

typedef struct Settings {
    char lang[8];
    char lang_confirmed[64];
    int  audio;
    long day_length;
    int  panel_page;
} Settings;

typedef struct App {
    const ClidePlatform *p;
    Framebuffer back, front;
    int    running;
    int    force;
    const Screen *stack[APP_STACK];
    int    depth;
    long   accum;
    cl_u32 frame;
    Settings set;
    ClideTimeCfg tcfg;
    cl_u32 caps;
    int    too_small;
    char   help_context[24];
    long   headless_frames;
} App;

extern App g_app;

/* screens */
extern const Screen scr_menu, scr_newworld, scr_loadworld, scr_settings, scr_language,
    scr_keybinds, scr_credits, scr_help, scr_game, scr_pause, scr_inventory, scr_stats,
    scr_console, scr_sky, scr_message;

void app_push(const Screen *s);
void app_pop(void);
void app_replace(const Screen *s);
void app_reset(const Screen *s);
const Screen *app_top(void);
int  app_paused(void);
int  app_on_stack(const Screen *s);
void app_open_help(const char *context);
void app_message(const char *title, const char *text);

void settings_load(void);
void settings_save(void);

/* game session glue */
int  app_start_new(const char *name, cl_u32 seed, int mode, int cheats, char *err, size_t n);
int  app_load_world(const char *name, char *err, size_t n);
int  app_save_world(void);
void app_close_world(int save);

int  app_main(int argc, char **argv);

#endif
