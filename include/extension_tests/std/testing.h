/*
 * This file defines functions and structs necessary to run tests using the
 * test runner.
 *
 * See std_testing/docs/testing.md for information on writing tests.
 */
#pragma once

#define _TEST_STRINGIFY(Val) #Val
#define _TEST_TO_STRING(Val) _TEST_STRINGIFY(Val)
#define _TEST_ASSERT_DEFINED(f)                                                \
  static_assert(_Generic(&(f), __typeof__(&(f)): 1, default: 0),               \
                #f " is not defined")

#define TEST_DNAME _std_test_data_main

/* Verification Macros */

/**
 * Immediately causes the currently running test to end, registering as a
 * success.
 */
#define PASS_TEST()                                                            \
  do {                                                                         \
    TEST_DNAME->state = testing_PASSED;                                        \
    return;                                                                    \
  } while (0)

/**
 * Immediately causes the currently running test to end, registering as a
 * failure.
 */
#define FAIL_TEST(fail_message)                                                \
  do {                                                                         \
    TEST_DNAME->state = testing_FAILED;                                        \
    TEST_DNAME->failed++;                                                      \
    TEST_DNAME->message = "line " _TEST_TO_STRING(__LINE__) ": " fail_message; \
    return;                                                                    \
  } while (0)

/**
 * Causes the test to fail if the expression is false.
 */
#define IS_TRUE(expression)                                                    \
  do {                                                                         \
    if (!(expression)) {                                                       \
      TEST_DNAME->state = testing_FAILED;                                      \
      TEST_DNAME->failed++;                                                    \
      TEST_DNAME->message =                                                    \
          "line " _TEST_TO_STRING(__LINE__) ": " #expression                   \
                                            " was false when expected true";   \
      return;                                                                  \
    } else {                                                                   \
      TEST_DNAME->state = testing_PASSED;                                      \
    }                                                                          \
  } while (0)

/**
 * Causes the test to fail if the expression is true.
 */
#define IS_FALSE(expression)                                                   \
  do {                                                                         \
    if (expression) {                                                          \
      TEST_DNAME->state = testing_FAILED;                                      \
      TEST_DNAME->failed++;                                                    \
      TEST_DNAME->message =                                                    \
          "line " _TEST_TO_STRING(__LINE__) ": " #expression                   \
                                            " was true when expected false";   \
      return;                                                                  \
    } else {                                                                   \
      TEST_DNAME->state = testing_PASSED;                                      \
    }                                                                          \
  } while (0)

#define _std_test_abort()                                                      \
  do {                                                                         \
    TEST_DNAME->state = testing_FAILED;                                        \
    TEST_DNAME->failed++;                                                      \
    TEST_DNAME->message =                                                      \
        "line " _TEST_TO_STRING(__LINE__) ": code panicked.";                  \
    return;                                                                    \
  } while (0)

/* Registration Macros */

/**
 * Initializes standard information about this test file.
 */
#define INIT()                                                                 \
  _std_test_data TEST_DNAME;                                                   \
  do {                                                                         \
    TEST_DNAME.failed = 0;                                                     \
    TEST_DNAME.run = 0;                                                        \
    TEST_DNAME.skipped = 0;                                                    \
    TEST_DNAME.message = "";                                                   \
    TEST_DNAME.state = testing_NOT_RUN;                                        \
    std_eprintf("Running tests for " __FILE__ ":\n");                          \
  } while (0)

/**
 * Runs the test function [testname] defined using the [TEST] macro.
 */
#define RUN(testname)                                                          \
  _TEST_ASSERT_DEFINED(testname);                                              \
  do {                                                                         \
    TEST_DNAME.run++;                                                          \
    testname(&TEST_DNAME);                                                     \
    if (TEST_DNAME.state == testing_FAILED) {                                  \
      std_eprintf(#testname ":\n\t%s\n", TEST_DNAME.message);                  \
    }                                                                          \
    TEST_DNAME.state = testing_NOT_RUN;                                        \
  } while (0)

/**
 * Skips running the test function [testname].
 */
#define SKIP(testname)                                                         \
  _TEST_ASSERT_DEFINED(testname);                                              \
  do {                                                                         \
    TEST_DNAME.skipped++;                                                      \
    std_eprintf(#testname ": skipped.\n");                                     \
    TEST_DNAME.state = testing_NOT_RUN;                                        \
  } while (0)

/**
 * Ends the currently running test file, performing any necessary clean up
 * actions.
 */
#define CONCLUDE()                                                             \
  do {                                                                         \
    std_eprintf("%d/%d failed tests (%d skipped).\n\n", TEST_DNAME.failed,     \
                TEST_DNAME.run + TEST_DNAME.skipped, TEST_DNAME.skipped);      \
    return TEST_DNAME.failed;                                                  \
  } while (0)

/**
 * Defines a test function named [testname]. Test function names should be valid
 * function names.
 */
#define TEST(testname) void testname(_std_test_data *TEST_DNAME)

#define _std_hook_abort _std_test_abort // Overwrite abort() calls in error.h

#include "std/error.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stdint.h>

typedef enum _std_test_state {
  testing_PASSED,
  testing_FAILED,
  testing_NOT_RUN,
} _std_test_state;

typedef struct _std_test_data {
  int32_t failed;
  int32_t run;
  int32_t skipped;
  const char *message;
  _std_test_state state;
} _std_test_data;
