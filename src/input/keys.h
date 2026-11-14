/*
 * Cliderness - input/keys.h  (ClideInput event model)
 * Printable keys are delivered as their Unicode code point (long).
 * Special keys use codes above 0x110000 so they never collide.
 */
#ifndef CLIDE_INPUT_KEYS_H
#define CLIDE_INPUT_KEYS_H

#define CK_NONE      0L
#define CK_BACKSPACE 8L
#define CK_TAB       9L
#define CK_ENTER     13L
#define CK_ESC       27L
#define CK_SPACE     32L

#define CK_SPECIAL   0x110000L
#define CK_UP        (CK_SPECIAL + 1)
#define CK_DOWN      (CK_SPECIAL + 2)
#define CK_LEFT      (CK_SPECIAL + 3)
#define CK_RIGHT     (CK_SPECIAL + 4)
#define CK_HOME      (CK_SPECIAL + 5)
#define CK_END       (CK_SPECIAL + 6)
#define CK_PGUP      (CK_SPECIAL + 7)
#define CK_PGDN      (CK_SPECIAL + 8)
#define CK_INSERT    (CK_SPECIAL + 9)
#define CK_DELETE    (CK_SPECIAL + 10)
#define CK_F1        (CK_SPECIAL + 11)   /* F1..F12 contiguous */
#define CK_F12       (CK_SPECIAL + 22)
/* "Tap" events: the modifier was pressed and released alone.
 * Only produced by adapters with TERMCAP_BARE_MODS. */
#define CK_LSHIFT    (CK_SPECIAL + 30)
#define CK_RSHIFT    (CK_SPECIAL + 31)
#define CK_CTRL      (CK_SPECIAL + 32)
#define CK_RESIZE    (CK_SPECIAL + 40)
#define CK_QUIT      (CK_SPECIAL + 41)   /* window closed / Ctrl+C */

#define KM_SHIFT 1
#define KM_CTRL  2
#define KM_ALT   4

typedef struct ClideKeyEvent {
    long key;
    int  mods;
} ClideKeyEvent;

#endif
