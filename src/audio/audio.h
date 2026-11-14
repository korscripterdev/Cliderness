/*
 * Cliderness - audio/audio.h  (ClideAudio front end)
 * Game code requests sound effects by id; the front end turns them into
 * tone sequences for the PC Speaker back end. When the platform has no audio
 * device (null back end) every call is a no-op and the game runs silently.
 *
 * The procedural PCM synthesiser lives in audio/dsp.h; it is used by the
 * `--render-sfx` tool today and will feed PCM back ends (planned).
 */
#ifndef CLIDE_AUDIO_H
#define CLIDE_AUDIO_H

#include "platform/platform.h"

enum {
    SFX_NONE = 0, SFX_STEP, SFX_MINE, SFX_BREAK, SFX_PLACE, SFX_PICKUP, SFX_HURT,
    SFX_MENU, SFX_SPLASH, SFX_THUNDER, SFX_CROW, SFX_BIRD, SFX_COUNT
};

void aud_init(const ClidePlatform *p, int enabled);
void aud_shutdown(void);
void aud_set_enabled(int on);
int  aud_enabled(void);
int  aud_available(void);
void aud_play(int sfx);
/* advance the tone sequencer by real milliseconds (call every frame) */
void aud_update(long ms);
void aud_silence(void);
const char *aud_backend(void);

#endif
