/*
 * Cliderness - tests/tf.h : minimal C90 test framework
 */
#ifndef CLIDE_TF_H
#define CLIDE_TF_H

#include <stdio.h>

extern int tf_pass;
extern int tf_fail;
extern int tf_case_fail;
extern const char *tf_data;

#define CHECK(cond) do { \
    if (cond) { ++tf_pass; } \
    else { ++tf_fail; ++tf_case_fail; printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_EQ_L(a, b) do { \
    long tf_a_ = (long)(a), tf_b_ = (long)(b); \
    if (tf_a_ == tf_b_) { ++tf_pass; } \
    else { ++tf_fail; ++tf_case_fail; printf("    FAIL %s:%d: %s == %s (%ld != %ld)\n", __FILE__, __LINE__, #a, #b, tf_a_, tf_b_); } \
} while (0)

typedef void (*TfFn)(void);
void tf_run(const char *name, TfFn fn);

/* suites */
void suite_core(void);
void suite_world(void);
void suite_gameplay(void);
void suite_commands(void);
void suite_save(void);
void suite_content(void);
void suite_app(void);
void suite_fluid(void);
void suite_crafting(void);

/* helpers shared by suites */
struct Game;
int  tf_make_game(struct Game *g, int mode, const char *dir);
void tf_flat_area(struct Game *g, long cx, long y, long cz, int r);

#endif
