/*
 * Cliderness - content/recipes.h
 * Data-driven recipes (data/recipes/<file>.def): crafting, smelting, cooking
 * and repair materials. A recipe may need a station block nearby and fuel
 * units (any item with fuel > 0) for furnaces and campfires.
 */
#ifndef CLIDE_CONTENT_RECIPES_H
#define CLIDE_CONTENT_RECIPES_H

#include "core/types.h"

#define MAX_RECIPES   192
#define RECIPE_IN     4

enum { RCAT_TOOLS = 0, RCAT_BUILDING, RCAT_MATERIALS, RCAT_FOOD, RCAT_EQUIPMENT, RCAT_MEDICAL, RCAT_VEHICLES, RCAT_ANCIENT, RCAT_COUNT };

typedef struct RecipeDef {
    char   id[24];
    cl_s16 result;
    cl_s16 count;
    cl_s16 in_item[RECIPE_IN];
    cl_s16 in_count[RECIPE_IN];
    cl_u8  nin;
    cl_u8  station;
    cl_u8  fuel;
    cl_u8  skill;      /* skill trained (player.h SK_*) */
    cl_u8  category;
    cl_u8  min_skill;  /* required skill level */
} RecipeDef;

extern RecipeDef g_recipes[MAX_RECIPES];
extern int g_recipe_count;

int  recipes_load(const char *root, char *err, size_t errn);
int  recipe_find(const char *id);
const char *recipe_category_key(int cat);

#endif
