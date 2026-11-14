/* Cliderness - commands/cmd_items.c : item, inventory, equipment, durability */
#include <stdio.h>
#include <string.h>
#include "commands/console.h"
#include "content/content.h"
#include "localization/locale.h"
#include "core/str.h"

static int self_target(const char *s)
{
    return str_ieq(s, "@s") || str_ieq(s, "@p") || str_ieq(s, "@a") || str_ieq(s, "me");
}

static int c_give(ConCtx *c)
{
    int argi = 1, it, left;
    long n = 1;
    char buf[96];
    if (c->argc >= 2 && c->argv[1][0] == '@') {
        if (!self_target(c->argv[1])) {
            con_err(loc_get("con.bad_target"));
            return CMD_ERR;
        }
        argi = 2;
    }
    if (c->argc <= argi) {
        return con_usage(c);
    }
    it = item_find(c->argv[argi]);
    if (it < 0) {
        con_err(loc_fmt("con.unknown_item", c->argv[argi], NULL, NULL));
        return CMD_ERR;
    }
    if (c->argc > argi + 1 && (!con_arg_long(c, argi + 1, &n) || n < 1 || n > 2304)) {
        return con_usage(c);
    }
    left = inv_add(&c->g->inv, it, (int)n, 0);
    sprintf(buf, "%ld", n - left);
    con_good(loc_fmt("con.given", buf, item_name(it), NULL));
    if (left > 0) {
        con_err(loc_get("msg.inventory_full"));
    }
    return CMD_OK;
}

static int c_clear(ConCtx *c)
{
    char buf[32];
    int argi = 1;
    if (c->argc >= 2 && c->argv[1][0] == '@') {
        if (!self_target(c->argv[1])) {
            con_err(loc_get("con.bad_target"));
            return CMD_ERR;
        }
        argi = 2;
    }
    if (c->argc > argi) {
        int it = item_find(c->argv[argi]);
        int n;
        if (it < 0) {
            con_err(loc_fmt("con.unknown_item", c->argv[argi], NULL, NULL));
            return CMD_ERR;
        }
        n = inv_remove(&c->g->inv, it, 32000);
        sprintf(buf, "%d", n);
    } else {
        int i, n = 0;
        for (i = 0; i < INV_SLOTS; ++i) {
            if (c->g->inv.s[i].item >= 0) {
                n += c->g->inv.s[i].count;
            }
        }
        inv_clear(&c->g->inv);
        sprintf(buf, "%d", n);
    }
    con_good(loc_fmt("con.cleared", buf, NULL, NULL));
    return CMD_OK;
}

static int c_inventory(ConCtx *c)
{
    int i;
    char buf[128];
    if (c->argc < 2 || str_ieq(c->argv[1], "list")) {
        int any = 0;
        for (i = 0; i < INV_SLOTS; ++i) {
            const ItemStack *s = &c->g->inv.s[i];
            if (s->item >= 0) {
                char nm[48];
                str_copy(nm, item_name(s->item), sizeof(nm));
                sprintf(buf, "[%2d] %s x%d", i + 1, nm, s->count);
                con_out(buf);
                any = 1;
            }
        }
        if (!any) {
            con_out(loc_get("inv.empty"));
        }
        sprintf(buf, "%ld g / %ld g", inv_weight(&c->g->inv), player_carry_capacity(&c->g->p));
        con_out(buf);
        return CMD_OK;
    }
    if (str_ieq(c->argv[1], "clear")) {
        if (!c->g->cheats) {
            con_err(loc_get("con.no_cheats"));
            return CMD_DENIED;
        }
        inv_clear(&c->g->inv);
        con_good(loc_get("con.done"));
        return CMD_OK;
    }
    if (str_ieq(c->argv[1], "sort")) {
        inv_sort(&c->g->inv);
        con_good(loc_get("con.done"));
        return CMD_OK;
    }
    return con_usage(c);
}

static int c_hotbar(ConCtx *c)
{
    long n;
    if (!con_arg_long(c, 1, &n) || n < 1 || n > INV_HOTBAR) {
        return con_usage(c);
    }
    game_select(c->g, (int)n - 1);
    return CMD_OK;
}

static int c_iteminfo(ConCtx *c)
{
    int it;
    char buf[160];
    char nm[48];
    const ItemDef *d;
    if (c->argc >= 2) {
        it = item_find(c->argv[1]);
    } else {
        it = inv_selected(&c->g->inv)->item;
    }
    if (it < 0) {
        con_err(loc_get("msg.empty_hand"));
        return CMD_ERR;
    }
    d = &g_items[it];
    str_copy(nm, item_name(it), sizeof(nm));
    sprintf(buf, "%s [Cliderness:%s] type=%s stack=%d weight=%dg density=%d", nm, d->id,
            item_type_name(d->type), d->max_stack, d->weight, d->density);
    con_out(buf);
    if (d->durability > 0 || d->food > 0 || d->water > 0) {
        sprintf(buf, "durability=%d power=%d food=%d water=%d", d->durability, d->tool_power, d->food, d->water);
        con_out(buf);
    }
    return CMD_OK;
}

static int list_ids(ConCtx *c, int blocks)
{
    char line[160];
    int i, n = 0, total = 0;
    const char *filter = c->argc >= 2 ? c->argv[1] : "";
    line[0] = '\0';
    for (i = blocks ? 1 : 0; i < (blocks ? g_block_count : g_item_count); ++i) {
        const char *id = blocks ? g_blocks[i].id : g_items[i].id;
        if (!str_icontains(id, filter)) {
            continue;
        }
        ++total;
        if (strlen(line) + strlen(id) + 2 > 76) {
            con_out(line);
            line[0] = '\0';
            if (++n > 12) {
                con_out("...");
                break;
            }
        }
        str_cat(line, id, sizeof(line));
        str_cat(line, " ", sizeof(line));
    }
    if (line[0] && n <= 12) {
        con_out(line);
    }
    sprintf(line, "%d", total);
    con_out(loc_fmt("con.count", line, NULL, NULL));
    return CMD_OK;
}

static int c_items(ConCtx *c) { return list_ids(c, 0); }
static int c_blocks(ConCtx *c) { return list_ids(c, 1); }

static int c_durability(ConCtx *c)
{
    ItemStack *s = inv_selected(&c->g->inv);
    long n;
    char buf[64];
    if (s->item < 0 || g_items[s->item].durability <= 0) {
        con_err(loc_get("con.no_durability"));
        return CMD_ERR;
    }
    if (c->argc < 2 || str_ieq(c->argv[1], "get")) {
        sprintf(buf, "%d / %d (%d%%)", s->dur, g_items[s->item].durability,
                (int)((long)s->dur * 100L / g_items[s->item].durability));
        con_out(buf);
        return CMD_OK;
    }
    if (!str_ieq(c->argv[1], "set") || !con_arg_long(c, 2, &n) || n < 1 || n > 100) {
        return con_usage(c);
    }
    s->dur = (cl_s16)((long)g_items[s->item].durability * n / 100L);
    if (s->dur < 1) {
        s->dur = 1;
    }
    sprintf(buf, "%d / %d", s->dur, g_items[s->item].durability);
    con_good(buf);
    return CMD_OK;
}

static int c_repair(ConCtx *c)
{
    ItemStack *s = inv_selected(&c->g->inv);
    if (s->item < 0 || g_items[s->item].durability <= 0) {
        con_err(loc_get("con.no_durability"));
        return CMD_ERR;
    }
    s->dur = g_items[s->item].durability;
    con_good(loc_get("con.done"));
    return CMD_OK;
}

static const Command g_cmds[] = {
    { "give", "item", "[@s] <item> [count]", "@ii", NULL, PERM_CHEAT, c_give, NULL },
    { "clear", "item", "[@s] [item]", "@i", NULL, PERM_CHEAT, c_clear, NULL },
    { "inventory", "item", "<list|clear|sort>", "x", "list|clear|sort", PERM_ANY, c_inventory, "inv" },
    { "hotbar", "item", "<1-9>", "n", NULL, PERM_ANY, c_hotbar, "select" },
    { "iteminfo", "item", "[item]", "i", NULL, PERM_ANY, c_iteminfo, NULL },
    { "items", "item", "[filter]", "i", NULL, PERM_ANY, c_items, NULL },
    { "blocks", "block", "[filter]", "b", NULL, PERM_ANY, c_blocks, NULL },
    { "durability", "durability", "<set|get> [1-100 %]", "xn", "set|get", PERM_CHEAT, c_durability, "dur" },
    { "repair", "durability", "", "", NULL, PERM_CHEAT, c_repair, NULL }
};

CommandTable cmd_table_items(void)
{
    CommandTable t;
    t.cmds = g_cmds;
    t.count = (int)CL_COUNTOF(g_cmds);
    return t;
}
