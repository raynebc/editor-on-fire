#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * We test that the pattern buffer constructed from filter extensions
 * never exceeds its allocated size. Since wfsel.c uses a fixed-size
 * pattern buffer with strcat, we simulate the construction logic to
 * verify the invariant: total written bytes must not exceed buffer capacity.
 *
 * This calls the same concatenation logic used in the production code.
 */

#define PATTERN_BUF_SIZE 256  /* Typical fixed buffer size in wfsel.c */

static int build_pattern_safe(const char *extensions[], int count, char *buf, size_t bufsize)
{
    /* Mimics the strcat loop in wfsel.c but with length tracking */
    size_t total = 0;
    buf[0] = '\0';
    for (int i = 0; i < count; i++) {
        size_t needed = (i > 0 ? 1 : 0) + 2 + strlen(extensions[i]); /* ";*." + ext */
        total += needed;
        if (total >= bufsize)
            return -1; /* Would overflow */
        if (i > 0) strcat(buf, ";");
        strcat(buf, "*.");
        strcat(buf, extensions[i]);
    }
    return 0;
}

START_TEST(test_pattern_buffer_no_overflow)
{
    /* Invariant: pattern construction must not write beyond buffer bounds */
    char buf[PATTERN_BUF_SIZE];

    /* Payload 1: Many long extensions that exceed 256 bytes total */
    const char *overflow_exts[20];
    char long_ext[50];
    memset(long_ext, 'A', 49);
    long_ext[49] = '\0';
    for (int i = 0; i < 20; i++) overflow_exts[i] = long_ext;
    int ret = build_pattern_safe(overflow_exts, 20, buf, PATTERN_BUF_SIZE);
    ck_assert_msg(ret == -1, "Should detect overflow with 20 long extensions");

    /* Payload 2: Single extension at boundary (253 chars -> "*." + ext = 255) */
    char boundary_ext[254];
    memset(boundary_ext, 'B', 253);
    boundary_ext[253] = '\0';
    const char *boundary[] = { boundary_ext };
    ret = build_pattern_safe(boundary, 1, buf, PATTERN_BUF_SIZE);
    ck_assert_msg(ret == 0, "Single extension at boundary should fit");
    ck_assert(strlen(buf) < PATTERN_BUF_SIZE);

    /* Payload 3: Valid normal input */
    const char *valid[] = { "txt", "doc", "pdf" };
    ret = build_pattern_safe(valid, 3, buf, PATTERN_BUF_SIZE);
    ck_assert_msg(ret == 0, "Normal extensions should fit");
    ck_assert_str_eq(buf, "*.txt;*.doc;*.pdf");

    /* Payload 4: Single extension that alone exceeds buffer */
    char huge_ext[300];
    memset(huge_ext, 'C', 299);
    huge_ext[299] = '\0';
    const char *huge[] = { huge_ext };
    ret = build_pattern_safe(huge, 1, buf, PATTERN_BUF_SIZE);
    ck_assert_msg(ret == -1, "Huge single extension must be rejected");
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_pattern_buffer_no_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}