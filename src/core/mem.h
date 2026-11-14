/*
 * Cliderness - core/mem.h  (ClideMemory front end)
 * Every engine allocation goes through mem_alloc so usage can be measured and
 * a hard budget can be enforced (640 KB conventional memory on 16-bit DOS).
 */
#ifndef CLIDE_CORE_MEM_H
#define CLIDE_CORE_MEM_H

#include "core/types.h"

void  *mem_alloc(size_t n);
void  *mem_calloc(size_t n);
void   mem_free(void *p);
cl_u32 mem_in_use(void);
cl_u32 mem_peak(void);
cl_u32 mem_alloc_count(void);
/* 0 = unlimited */
void   mem_set_budget(cl_u32 bytes);
cl_u32 mem_budget(void);

#endif
