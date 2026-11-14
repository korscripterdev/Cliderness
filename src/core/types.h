/*
 * Cliderness - core/types.h
 * Fixed-meaning integer types for ISO C90 (no stdint.h in C90).
 * Created by korscripterdev.
 *
 * Rules:
 *  - int may be 16 bits (16-bit DOS). Anything that can exceed 32767 uses long.
 *  - unsigned long is at least 32 bits; on LP64 it is 64 bits, so every
 *    hash / RNG operation masks with CL_U32() to stay bit-identical.
 */
#ifndef CLIDE_CORE_TYPES_H
#define CLIDE_CORE_TYPES_H

#include <stddef.h>

typedef unsigned char  cl_u8;
typedef signed char    cl_s8;
typedef unsigned short cl_u16;
typedef short          cl_s16;
typedef unsigned long  cl_u32;
typedef long           cl_s32;
typedef int            cl_bool;

#define CL_TRUE  1
#define CL_FALSE 0

#define CL_U32(x) ((cl_u32)(x) & 0xFFFFFFFFUL)
#define CL_MIN(a, b) ((a) < (b) ? (a) : (b))
#define CL_MAX(a, b) ((a) > (b) ? (a) : (b))
#define CL_CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))
#define CL_COUNTOF(a) (sizeof(a) / sizeof((a)[0]))

/* Platform class used for memory budgets (decided at compile time). */
#if defined(__MSDOS__) || defined(__DOS__) || defined(MSDOS) || defined(__DJGPP__)
#define CLIDE_TARGET_DOS 1
#endif

#if defined(CLIDE_TARGET_DOS) && (defined(__I86__) || defined(_M_I86) || defined(__SMALL__) || defined(__LARGE__))
#define CLIDE_TARGET_DOS16 1
#endif

#endif
