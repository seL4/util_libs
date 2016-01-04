/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#define _XOPEN_SOURCE 700
#include <CUnit/Automated.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct testcase {
    const char *name;
    CU_TestFunc function;
    struct testcase *next;
};

static struct testcase *tests;

static void register_testcase(struct testcase *test) {
    test->next = tests;
    tests = test;
}

#define JSON_TESTCASE(f) \
    static void f(void); \
    static void __attribute__((constructor(__COUNTER__ + 200))) f##_register(void) { \
        static struct testcase test = { \
            .name = #f, \
            .function = f, \
        }; \
        register_testcase(&test); \
    } \
    static void f(void)

static void expected(const char *s);

#include "../src/json_raw.c"

/* Setup the test suite environment. */
static void testing_init(void) {
    FILE* f = tmpfile();
    if (f == NULL) {
        perror("failed to create temporary file");
        exit(-1);
    }
    int fd = fileno(f);
    if (fd == -1) {
        fprintf(stderr, "failed to translate temporary file descriptor\n");
        exit(-1);
    }
    /* Swap stdout with a temporary file so printf output is redirected. */
    int res = dup2(fd, STDOUT_FILENO);
    if (res == -1) {
        perror("failed to redirect stdout");
        exit(-1);
    }
}

int main(void) {

    testing_init();

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CU_ErrorCode err = CU_get_error();
        fprintf(stderr, "failed to initialise CUnit registry\n");
        return err;
    }

    CU_pSuite suite = CU_add_suite("json", NULL, NULL);
    if (suite == NULL) {
        CU_ErrorCode err = CU_get_error();
        CU_cleanup_registry();
        fprintf(stderr, "failed to add suite\n");
        return err;
    }

    unsigned int total = 0;

    for (struct testcase *p = tests; p != NULL; p = p->next) {
        if (CU_add_test(suite, p->name, p->function) == NULL) {
            CU_ErrorCode err = CU_get_error();
             fprintf(stderr, "failed to add test \"%s\"\n", p->name);
             CU_cleanup_registry();
             return err;
        }
        total++;
    }

    CU_set_output_filename("test-json");
    CU_automated_run_tests();

    unsigned int failed = CU_get_number_of_tests_failed();

    CU_cleanup_registry();

    fprintf(stderr, "%u/%u tests passed.\n", total - failed, total);
    if (failed > 0) {
        fprintf(stderr, "*** FAILURES DETECTED ***\n");
    } else {
        fprintf(stderr, "All is well in the universe.\n");
    }

    return failed;
}

/* Run jsonlint on a string of expected JSON. */
static int lint(const char *s) {
    FILE *p = popen("jsonlint", "w");
    if (p == NULL) {
        perror("failed to exec jsonlint");
        exit(-1);
    }
    fputs(s, p);
    return pclose(p);
}

/* Run jsondiff on two strings of JSON. */
static int diff(const char *a, const char *b) {
    FILE *p = popen("jsondiff", "w");
    if (p == NULL) {
        perror("failed to exec jsondiff");
        exit(-1);
    }
    fputs(a, p);
    fputc(0, p); /* See jsondiff source for why we do this. */
    fputs(b, p);
    return pclose(p);
}

/* Compare the current contents of the temporary file that we've replaced stdout
 * with, to the given string. They won't necessarily be equal literally, but
 * should be equivalent with respect to JSON.
 */
static void expected(const char *s) {
    static char buffer[8192];

    /* Make sure anything the caller test has done is flushed before we try and
     * read its output back in.
     */
    fflush(stdout);

    /* Remember, stdout is not what you think here. */
    FILE *f = fopen("/proc/self/fd/1", "r");
    if (f == NULL) {
        perror("failed to open temporary file for reading");
        exit(-1);
    }
    char *r = fgets(buffer, sizeof(buffer), f);
    if (r == NULL) {
        perror("failed to read temporary file");
        exit(-1);
    }

    /* Blank the temporary file under the assumption that the test is done
     * with it.
     */
    if (ftruncate(STDOUT_FILENO, 0) != 0) {
        perror("failed to truncate temporary file");
        exit(-1);
    }
    rewind(stdout);

    int lint_result = lint(buffer);
    CU_ASSERT_EQUAL(lint_result, 0);

    int diff_result = diff(buffer, s);
    CU_ASSERT_EQUAL(diff_result, 0);
}
