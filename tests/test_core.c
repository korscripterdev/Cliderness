/* Cliderness - tests/test_core.c : math, rng, noise, strings, cp437, fb, ini, memory, time */
#include <string.h>
#include "tf.h"
#include "core/crc.h"
#include "core/mathx.h"
#include "core/rng.h"
#include "core/noise.h"
#include "core/str.h"
#include "core/mem.h"
#include "core/log.h"
#include "renderer/fb.h"
#include "renderer/cp437.h"
#include "world/time/clidetime.h"
#include "audio/dsp.h"

static void t_crc(void)
{
    CHECK(crc32_final(crc32_update(CRC32_INIT, "123456789", 9)) == 0xCBF43926UL);
}

static void t_floor(void)
{
    CHECK_EQ_L(mx_floor_div(-1, 32), -1);
    CHECK_EQ_L(mx_floor_div(-32, 32), -1);
    CHECK_EQ_L(mx_floor_div(-33, 32), -2);
    CHECK_EQ_L(mx_floor_div(31, 32), 0);
    CHECK_EQ_L(mx_floor_mod(-1, 32), 31);
    CHECK_EQ_L(mx_floor_mod(-33, 32), 31);
    CHECK_EQ_L(mx_isqrt(1000000L), 1000);
    CHECK_EQ_L(mx_isqrt(99), 9);
    CHECK_EQ_L(mx_sin256(64), 1024);
    CHECK_EQ_L(mx_sin256(192), -1024);
    CHECK_EQ_L(mx_cos256(0), 1024);
}

/*
 * Reference values: identical on 16-bit DOS, 32-bit, LLP64 Windows and LP64
 * Unix (verified: clang/Windows LLP64 and gcc/Linux LP64 give the same
 * results). If these change, every existing world changes.
 */
static void t_rng_determinism(void)
{
    ClideRng r;
    CHECK(rng_hash32(0UL) == 0UL);
    CHECK(rng_hash32(1UL) == 0x688990C0UL);
    CHECK(rng_hash3(12345UL, -7L, 64L, 1000000L) == 0x36CC7EB0UL);
    rng_seed(&r, 42UL);
    CHECK(rng_next(&r) == 0xDABE66CDUL);
    CHECK(rng_next(&r) == 0xDA9E723EUL);
    CHECK(rng_hash_str("Cliderness") == 0xAB4E09B7UL);
}

static void t_noise(void)
{
    long x, z, mn = 99999, mx = -1;
    for (z = -300; z < 300; z += 7) {
        for (x = -300; x < 300; x += 11) {
            long v = fbm2(777UL, x, z, 6, 4);
            long n3 = noise3(777UL, x, z, -x, 4);
            if (v < mn) mn = v;
            if (v > mx) mx = v;
            CHECK(n3 >= 0 && n3 <= NOISE_MAX);
        }
    }
    CHECK(mn >= 0 && mx <= NOISE_MAX);
    CHECK(mx - mn > 1000);   /* not constant */
    CHECK_EQ_L(noise2(1UL, 100, -100, 5), noise2(1UL, 100, -100, 5));
    CHECK_EQ_L(fbm2(12345UL, -1000L, 2000L, 8, 4), 1440L);
    CHECK_EQ_L(noise3(9UL, -5L, 40L, 77L, 4), 2020L);
}

static void t_strings(void)
{
    char buf[64];
    const char *args[2];
    const char *p = "\xED\x95\x9C" "a";   /* Hangul HAN + 'a' */
    CHECK_EQ_L(utf8_next(&p), 0xD55CL);
    CHECK_EQ_L(utf8_next(&p), 'a');
    CHECK_EQ_L(utf8_next(&p), -1);
    CHECK_EQ_L(utf8_width("\xED\x95\x9C\xEA\xB8\x80"), 4);
    CHECK_EQ_L(utf8_width("abc"), 3);
    CHECK_EQ_L(utf8_encode(0x263AL, buf), 3);
    args[0] = "x";
    args[1] = "42";
    str_subst(buf, sizeof(buf), "{0}={1}{2}", args, 2);
    CHECK(strcmp(buf, "x=42") == 0);
    {
        long v;
        CHECK(str_to_long("-123", &v) && v == -123);
        CHECK(!str_to_long("12a", &v));
        CHECK(!str_to_long("", &v));
    }
    CHECK(str_ieq("Cliderness:Stone", "cliderness:stone"));
    CHECK(str_icontains("Open World Survival", "world"));
    str_from_long(buf, -2147483647L);
    CHECK(strcmp(buf, "-2147483647") == 0);
}

static void t_cp437(void)
{
    int i, ok = 1;
    for (i = 1; i < 256; ++i) {
        if (unicode_to_cp437(cp437_to_unicode(i)) != i) {
            ok = 0;
            printf("    glyph %d round trip failed\n", i);
        }
    }
    CHECK(ok);
    CHECK(cp437_to_unicode(0x06) == 0x2660L);   /* spade, a control-range glyph */
    CHECK(cp437_to_unicode(0xB0) == 0x2591L);
    CHECK(cp437_can_show("Caf\xC3\xA9 \xC3\xBC"));
    CHECK(!cp437_can_show("\xED\x95\x9C"));
}

static void t_framebuffer(void)
{
    static Framebuffer fb;
    char row[400];
    fb_clear(&fb, ATTR(C_LIGHTGRAY, C_BLACK));
    fb_text(&fb, 0, 0, "A\xED\x95\x9C" "B", ATTR(C_WHITE, C_BLACK), 0);
    CHECK(fb.cells[0].ch == 'A');
    CHECK(fb.cells[1].flags == FBC_UNI && fb.cells[1].ch == 0xD55C);
    CHECK(fb.cells[2].flags == FBC_CONT);
    CHECK(fb.cells[3].ch == 'B');
    /* overwriting half of a wide char clears the other half */
    fb_put(&fb, 2, 0, 'x', ATTR(C_WHITE, C_BLACK));
    CHECK(fb.cells[1].ch == ' ' && fb.cells[1].flags == 0);
    fb_put(&fb, 5, 0, G_SPADE, ATTR(C_GREEN, C_BLACK));
    fb_dump_row(&fb, 0, row, sizeof(row));
    CHECK(strcmp(row, "A x" "B" " " "\xE2\x99\xA0") == 0);
    CHECK_EQ_L(color_from_name("lightgreen"), C_LIGHTGREEN);
    CHECK_EQ_L(color_dim(C_YELLOW), C_BROWN);
    CHECK_EQ_L(ATTR(C_WHITE, C_BLUE), 0x1F);
}

static void t_memory(void)
{
    cl_u32 before = mem_in_use();
    void *a = mem_alloc(1000);
    CHECK(a != NULL);
    CHECK(mem_in_use() == before + 1000UL);
    mem_set_budget(mem_in_use() + 100UL);
    CHECK(mem_alloc(500) == NULL);           /* budget enforced */
    mem_set_budget(0);
    mem_free(a);
    CHECK(mem_in_use() == before);
}

static void t_log(void)
{
    log_clear();
    log_add(7, "hello");
    log_add(7, "hello");
    log_add(7, "world");
    CHECK_EQ_L(log_count(), 2);
    CHECK(log_get(1)->repeat == 2);
    CHECK(strcmp(log_get(0)->text, "world") == 0);
    log_clear();
}

static void t_time(void)
{
    ClideTimeCfg c;
    ClideTime t;
    ClideDate d;
    long i;
    ctime_default_cfg(&c);
    ctime_init(&t, &c, 8, 0);
    ctime_date(&t, &d);
    CHECK(d.hour == 8 && d.minute == 0 && d.year == 1 && d.season == SEASON_SPRING);
    /* one tick of 50 ms = 3.6 game seconds (1200 s per day) */
    ctime_advance(&t, 50);
    CHECK_EQ_L(t.ms, 8L * 3600000L + 3600L);
    /* 20 real minutes = exactly one game day */
    ctime_init(&t, &c, 8, 0);
    for (i = 0; i < 24000L; ++i) {
        ctime_advance(&t, 50);
    }
    CHECK_EQ_L(t.day, 1);
    CHECK_EQ_L(t.ms, 8L * 3600000L);
    /* time scale x2 */
    c.scale_pct = 200;
    ctime_init(&t, &c, 0, 0);
    ctime_advance(&t, 1000);
    CHECK_EQ_L(t.ms, 144000L);
    /* daylight */
    ctime_default_cfg(&c);
    ctime_init(&t, &c, 12, 0);
    CHECK(ctime_daylight(&t) >= 90);
    ctime_set_time_of_day(&t, 0);
    CHECK(ctime_daylight(&t) == 0);
    CHECK_EQ_L(t.day, 1);   /* setting an earlier hour moves to the next day */
    /* seasons and moon */
    ctime_set_season(&t, SEASON_WINTER);
    ctime_date(&t, &d);
    CHECK(d.season == SEASON_WINTER);
    {
        int seen[8], k, all = 1;
        memset(seen, 0, sizeof(seen));
        for (k = 0; k < 30; ++k) {
            ctime_add_seconds(&t, 86400L);
            ctime_date(&t, &d);
            seen[d.moon_phase] = 1;
        }
        for (k = 0; k < 8; ++k) {
            all &= seen[k];
        }
        CHECK(all);
    }
    /* summer days are longer than winter days */
    {
        ClideDate s, w;
        ctime_set_season(&t, SEASON_SUMMER);
        ctime_add_seconds(&t, 86400L * 45L);
        ctime_date(&t, &s);
        ctime_set_season(&t, SEASON_WINTER);
        ctime_add_seconds(&t, 86400L * 45L);
        ctime_date(&t, &w);
        CHECK(s.sunset_min - s.sunrise_min > w.sunset_min - w.sunrise_min);
    }
}

static void t_dsp(void)
{
    static short a[4000], b[4000];
    int i, k, same = 1;
    long sum = 0;
    for (k = 0; dsp_sound_name(k); ++k) {
        CHECK(dsp_render(dsp_sound_name(k), a, 4000, 22050, 7UL) == 4000);
    }
    CHECK(dsp_render("no_such_sound", a, 100, 22050, 1UL) == 0);
    dsp_render("wind", a, 4000, 22050, 99UL);
    dsp_render("wind", b, 4000, 22050, 99UL);
    for (i = 0; i < 4000; ++i) {
        if (a[i] != b[i]) same = 0;
    }
    CHECK(same);
    dsp_render("pink", a, 4000, 22050, 3UL);
    for (i = 0; i < 4000; ++i) {
        sum += a[i];
    }
    CHECK(sum / 4000L > -4000L && sum / 4000L < 4000L);
    CHECK(dsp_distance_gain(1, 0) == 256);
    CHECK(dsp_distance_gain(20, 2) < dsp_distance_gain(20, 0));
}

void suite_core(void)
{
    tf_run("crc32", t_crc);
    tf_run("floor division / isqrt / sine", t_floor);
    tf_run("rng cross-platform determinism", t_rng_determinism);
    tf_run("integer noise", t_noise);
    tf_run("strings and utf-8", t_strings);
    tf_run("cp437 glyph table", t_cp437);
    tf_run("framebuffer and wide chars", t_framebuffer);
    tf_run("memory accounting and budget", t_memory);
    tf_run("event log", t_log);
    tf_run("ClideTime", t_time);
    tf_run("procedural audio dsp", t_dsp);
}
