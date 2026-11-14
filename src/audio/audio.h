/*
 * Cliderness - audio/audio.h  (ClideAudio front end)
 * Game code requests sound effects by id. Two back ends:
 *   PCM      (Windows waveOut, PulseAudio / PipeWire / ALSA pipes, macOS
 *             afplay): the effect is synthesised on the fly by the
 *             procedural DSP core (audio/dsp.h) - no recorded samples
 *   tone     (PC Speaker on DOS): short square-wave tone sequences
 * Without an audio device every call is a no-op and the game runs silently.
 */
#ifndef CLIDE_AUDIO_H
#define CLIDE_AUDIO_H

#include "platform/platform.h"

enum {
    SFX_NONE = 0, SFX_STEP, SFX_MINE, SFX_BREAK, SFX_PLACE, SFX_PICKUP, SFX_HURT,
    SFX_MENU, SFX_SPLASH, SFX_THUNDER, SFX_CROW, SFX_BIRD, SFX_CREAK, SFX_CRACK,
    /* ambience (PCM only; the PC Speaker stays quiet) */
    SFX_RAIN, SFX_WIND, SFX_STORM, SFX_CAVE, SFX_ANCIENT, SFX_RIVER, SFX_FIRE,
    SFX_COUNT
};

void aud_init(const ClidePlatform *p, int enabled);
void aud_shutdown(void);
void aud_set_enabled(int on);
int  aud_enabled(void);
int  aud_available(void);
int  aud_pcm(void);              /* 1 = live PCM back end in use */
void aud_play(int sfx);
/* advance the sequencer by real milliseconds (call every frame) */
void aud_update(long ms);
void aud_silence(void);
const char *aud_backend(void);
int  aud_find(const char *name);   /* SFX id by name, -1 */
const char *aud_sfx_name(int sfx);
long aud_played(void);            /* effects started since aud_init */

#endif
