/* Cliderness - gameplay/storage/chest.c : chest contents bound to chest blocks */
#include <string.h>
#include "gameplay/game.h"
#include "content/content.h"

int chest_find(const Game *g, int dim, long x, long y, long z)
{
    int i;
    for (i = 0; i < MAX_CHESTS; ++i) {
        const Chest *c = &g->chests[i];
        if (c->active && c->x == x && c->y == y && c->z == z && c->dim == dim) {
            return i;
        }
    }
    return -1;
}

int chest_create(Game *g, int dim, long x, long y, long z)
{
    int i = chest_find(g, dim, x, y, z), k;
    if (i >= 0) {
        return i;
    }
    for (i = 0; i < MAX_CHESTS; ++i) {
        Chest *c = &g->chests[i];
        if (!c->active) {
            memset(c, 0, sizeof(*c));
            c->active = 1;
            c->x = x;
            c->y = y;
            c->z = z;
            c->dim = (cl_u8)dim;
            for (k = 0; k < CHEST_SLOTS; ++k) {
                c->s[k].item = -1;
            }
            return i;
        }
    }
    return -1;
}

void chest_remove(Game *g, int idx)
{
    Chest *c;
    int k;
    if (idx < 0 || idx >= MAX_CHESTS || !g->chests[idx].active) {
        return;
    }
    c = &g->chests[idx];
    for (k = 0; k < CHEST_SLOTS; ++k) {
        if (c->s[k].item >= 0) {
            ent_spawn(g, c->dim, c->x, c->y, c->z, c->s[k].item, c->s[k].count, c->s[k].dur);
        }
    }
    c->active = 0;
}

int chest_count(const Game *g)
{
    int i, n = 0;
    for (i = 0; i < MAX_CHESTS; ++i) {
        n += g->chests[i].active;
    }
    return n;
}
