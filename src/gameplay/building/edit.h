/*
 * Cliderness - gameplay/building/edit.h
 * World editing with an undo/redo journal (used by building commands) and
 * the player's mining / placing actions.
 */
#ifndef CLIDE_GAMEPLAY_EDIT_H
#define CLIDE_GAMEPLAY_EDIT_H

#include "core/types.h"

#ifdef CLIDE_TARGET_DOS16
#define EDIT_MAX_RECORDS 1024L
#define EDIT_MAX_OPS     16
#define EDIT_MAX_VOLUME  4096L
#else
#define EDIT_MAX_RECORDS 131072L
#define EDIT_MAX_OPS     64
#define EDIT_MAX_VOLUME  262144L
#endif

typedef struct EditRec {
    long   x, z;
    cl_s16 y;
    cl_u8  dim, oldb, oldm, newb, newm;
} EditRec;

typedef struct EditJournal {
    EditRec *r;
    long  cap;
    long  count;          /* records in the undo history */
    long  op_start[EDIT_MAX_OPS + 1];
    int   nops;           /* recorded operations */
    int   pos;            /* operations currently applied (<= nops) */
    long  cur_start;
    int   recording;
    int   overflow;
} EditJournal;

#endif
