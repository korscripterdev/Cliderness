/* Cliderness - core/mem.c */
#include <stdlib.h>
#include <string.h>
#include "core/mem.h"

/* Header keeps the size; union guarantees alignment for any type. */
typedef union MemHdr {
    cl_u32 size;
    long   l;
    double d;
    void  *p;
} MemHdr;

static cl_u32 g_in_use = 0;
static cl_u32 g_peak = 0;
static cl_u32 g_count = 0;
static cl_u32 g_budget = 0;

void *mem_alloc(size_t n)
{
    MemHdr *h;
    if (g_budget && g_in_use + (cl_u32)n > g_budget) {
        return NULL;
    }
    h = (MemHdr *)malloc(sizeof(MemHdr) + n);
    if (!h) {
        return NULL;
    }
    h->size = (cl_u32)n;
    g_in_use += (cl_u32)n;
    ++g_count;
    if (g_in_use > g_peak) {
        g_peak = g_in_use;
    }
    return (void *)(h + 1);
}

void *mem_calloc(size_t n)
{
    void *p = mem_alloc(n);
    if (p) {
        memset(p, 0, n);
    }
    return p;
}

void mem_free(void *p)
{
    MemHdr *h;
    if (!p) {
        return;
    }
    h = ((MemHdr *)p) - 1;
    g_in_use -= h->size;
    --g_count;
    free(h);
}

cl_u32 mem_in_use(void) { return g_in_use; }
cl_u32 mem_peak(void) { return g_peak; }
cl_u32 mem_alloc_count(void) { return g_count; }
void mem_set_budget(cl_u32 bytes) { g_budget = bytes; }
cl_u32 mem_budget(void) { return g_budget; }
