/*
 * Cliderness - gameplay/inventory/inventory.h
 * 36 slots: 0..8 are the hotbar, 9..35 the backpack.
 */
#ifndef CLIDE_GAMEPLAY_INVENTORY_H
#define CLIDE_GAMEPLAY_INVENTORY_H

#include "core/types.h"

#define INV_SLOTS  36
#define INV_HOTBAR 9

typedef struct ItemStack {
    cl_s16 item;   /* -1 = empty */
    cl_s16 count;
    cl_s16 dur;    /* remaining durability (tools) */
} ItemStack;

#define INV_EQUIP 5   /* head, body, legs, feet, back (content.h EQ_*) */

typedef struct Inventory {
    ItemStack s[INV_SLOTS];
    ItemStack equip[INV_EQUIP];
    int selected;  /* hotbar slot 0..8 */
} Inventory;

void inv_init(Inventory *inv);
/* returns how many could NOT be added */
int  inv_add(Inventory *inv, int item, int count, int dur);
int  inv_remove(Inventory *inv, int item, int count);
int  inv_remove_slot(Inventory *inv, int slot, int count);
int  inv_count(const Inventory *inv, int item);
int  inv_free_slots(const Inventory *inv);
void inv_swap(Inventory *inv, int a, int b);
/* move or merge slot a into slot b */
void inv_move(Inventory *inv, int a, int b);
/* half of slot a into the first empty slot; returns new slot or -1 */
int  inv_split(Inventory *inv, int a);
/* sort backpack (9..35) by item id, merging partial stacks */
void inv_sort(Inventory *inv);
void inv_clear(Inventory *inv);
long inv_weight(const Inventory *inv);  /* grams */
long inv_volume(const Inventory *inv);  /* centilitres */
ItemStack *inv_selected(Inventory *inv);
/* equipment: returns 1 on success */
int  inv_equip(Inventory *inv, int slot);
int  inv_unequip(Inventory *inv, int eq);
int  inv_defense(const Inventory *inv);
int  inv_insulation(const Inventory *inv);
long inv_bag_capacity(const Inventory *inv);   /* grams */

#endif
