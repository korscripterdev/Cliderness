/* Cliderness - gameplay/inventory/inventory.c */
#include "gameplay/inventory/inventory.h"
#include "content/content.h"

void inv_init(Inventory *inv)
{
    inv_clear(inv);
    inv->selected = 0;
}

void inv_clear(Inventory *inv)
{
    int i;
    for (i = 0; i < INV_SLOTS; ++i) {
        inv->s[i].item = -1;
        inv->s[i].count = 0;
        inv->s[i].dur = 0;
    }
}

static int max_stack(int item)
{
    return (item >= 0 && item < g_item_count) ? g_items[item].max_stack : 64;
}

int inv_add(Inventory *inv, int item, int count, int dur)
{
    int i, pass;
    int ms;
    if (item < 0 || item >= g_item_count || count <= 0) {
        return count;
    }
    ms = max_stack(item);
    if (ms > 1) {
        for (i = 0; i < INV_SLOTS && count > 0; ++i) {
            ItemStack *st = &inv->s[i];
            if (st->item == item && st->count < ms) {
                int room = ms - st->count;
                int n = count < room ? count : room;
                st->count = (cl_s16)(st->count + n);
                count -= n;
            }
        }
    }
    /* fill empty slots: hotbar first, then backpack */
    for (pass = 0; pass < 1 && count > 0; ++pass) {
        for (i = 0; i < INV_SLOTS && count > 0; ++i) {
            ItemStack *st = &inv->s[i];
            if (st->item < 0) {
                int n = count < ms ? count : ms;
                st->item = (cl_s16)item;
                st->count = (cl_s16)n;
                st->dur = (cl_s16)(dur > 0 ? dur : g_items[item].durability);
                count -= n;
            }
        }
    }
    return count;
}

int inv_remove_slot(Inventory *inv, int slot, int count)
{
    ItemStack *st;
    int n;
    if (slot < 0 || slot >= INV_SLOTS) {
        return 0;
    }
    st = &inv->s[slot];
    if (st->item < 0) {
        return 0;
    }
    n = count < st->count ? count : st->count;
    st->count = (cl_s16)(st->count - n);
    if (st->count <= 0) {
        st->item = -1;
        st->count = 0;
        st->dur = 0;
    }
    return n;
}

int inv_remove(Inventory *inv, int item, int count)
{
    int i, removed = 0;
    for (i = INV_SLOTS - 1; i >= 0 && removed < count; --i) {
        if (inv->s[i].item == item) {
            removed += inv_remove_slot(inv, i, count - removed);
        }
    }
    return removed;
}

int inv_count(const Inventory *inv, int item)
{
    int i, n = 0;
    for (i = 0; i < INV_SLOTS; ++i) {
        if (inv->s[i].item == item) {
            n += inv->s[i].count;
        }
    }
    return n;
}

int inv_free_slots(const Inventory *inv)
{
    int i, n = 0;
    for (i = 0; i < INV_SLOTS; ++i) {
        if (inv->s[i].item < 0) {
            ++n;
        }
    }
    return n;
}

void inv_swap(Inventory *inv, int a, int b)
{
    ItemStack t;
    if (a < 0 || b < 0 || a >= INV_SLOTS || b >= INV_SLOTS || a == b) {
        return;
    }
    t = inv->s[a];
    inv->s[a] = inv->s[b];
    inv->s[b] = t;
}

void inv_move(Inventory *inv, int a, int b)
{
    ItemStack *sa, *sb;
    if (a < 0 || b < 0 || a >= INV_SLOTS || b >= INV_SLOTS || a == b) {
        return;
    }
    sa = &inv->s[a];
    sb = &inv->s[b];
    if (sa->item >= 0 && sa->item == sb->item && max_stack(sa->item) > 1) {
        int room = max_stack(sb->item) - sb->count;
        int n = sa->count < room ? sa->count : room;
        sb->count = (cl_s16)(sb->count + n);
        inv_remove_slot(inv, a, n);
        return;
    }
    inv_swap(inv, a, b);
}

int inv_split(Inventory *inv, int a)
{
    int i, half;
    ItemStack *sa;
    if (a < 0 || a >= INV_SLOTS) {
        return -1;
    }
    sa = &inv->s[a];
    if (sa->item < 0 || sa->count < 2) {
        return -1;
    }
    for (i = INV_HOTBAR; i < INV_SLOTS + INV_HOTBAR; ++i) {
        int slot = i % INV_SLOTS;
        if (inv->s[slot].item < 0) {
            half = sa->count / 2;
            inv->s[slot] = *sa;
            inv->s[slot].count = (cl_s16)half;
            sa->count = (cl_s16)(sa->count - half);
            return slot;
        }
    }
    return -1;
}

void inv_sort(Inventory *inv)
{
    int i, j;
    /* merge partial stacks */
    for (i = INV_HOTBAR; i < INV_SLOTS; ++i) {
        for (j = i + 1; j < INV_SLOTS && inv->s[i].item >= 0; ++j) {
            if (inv->s[j].item == inv->s[i].item) {
                inv_move(inv, j, i);
            }
        }
    }
    /* insertion sort, empty slots last */
    for (i = INV_HOTBAR + 1; i < INV_SLOTS; ++i) {
        ItemStack key = inv->s[i];
        int kid = key.item < 0 ? 32000 : key.item;
        j = i - 1;
        while (j >= INV_HOTBAR) {
            int jid = inv->s[j].item < 0 ? 32000 : inv->s[j].item;
            if (jid <= kid) {
                break;
            }
            inv->s[j + 1] = inv->s[j];
            --j;
        }
        inv->s[j + 1] = key;
    }
}

long inv_weight(const Inventory *inv)
{
    int i;
    long w = 0;
    for (i = 0; i < INV_SLOTS; ++i) {
        if (inv->s[i].item >= 0) {
            w += (long)g_items[inv->s[i].item].weight * (long)inv->s[i].count;
        }
    }
    return w;
}

long inv_volume(const Inventory *inv)
{
    int i;
    long v = 0;
    for (i = 0; i < INV_SLOTS; ++i) {
        if (inv->s[i].item >= 0) {
            v += (long)g_items[inv->s[i].item].volume * (long)inv->s[i].count;
        }
    }
    return v;
}

ItemStack *inv_selected(Inventory *inv)
{
    return &inv->s[inv->selected];
}
