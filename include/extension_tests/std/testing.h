/*
 * This file defines functions and structs necessary to run tests using the
 * test runner.
 *
 * See std_testing/docs/testing.md for information on writing tests.
 */
#pragma once

#include "std/error.h" // IWYU pragma: keep
#include <assert.h>    // IWYU pragma: keep
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>

#define _TEST_STRINGIFY(Val) #Val
#define _TEST_TO_STRING(Val) _TEST_STRINGIFY(Val)
#define _TEST_ASSERT_DEFINED(f)                                                \
  static_assert(_Generic(&(f), typeof(&(f)): 1, default: 0),                   \
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
      TEST_DNAME->message =                                                    \
          "line " _TEST_TO_STRING(__LINE__) ": " #expression                   \
                                            " was true when expected false";   \
      return;                                                                  \
    } else {                                                                   \
      TEST_DNAME->state = testing_PASSED;                                      \
    }                                                                          \
  } while (0)

/**
 * Causes the test to fail if the expression doesn't panic. Non-zero exits don't
 * count as panics.
 */
#define IS_PANIC(expression)                                                   \
  do {                                                                         \
    abort_hook old_abort = std_set_abort_hook(_std_accept_except_abort);       \
    int val = setjmp(_std_accept_except_jmp);                                  \
    if (val == 0) {                                                            \
      expression;                                                              \
    } else if (val == -1) {                                                    \
      if (TEST_DNAME->is_verbose) {                                            \
        std_eprintf("> -- Expected panic occurred.\n");                        \
      }                                                                        \
      std_set_abort_hook(old_abort);                                           \
      TEST_DNAME->state = testing_PASSED;                                      \
      return;                                                                  \
    }                                                                          \
    TEST_DNAME->state = testing_FAILED;                                        \
    TEST_DNAME->message =                                                      \
        "line " _TEST_TO_STRING(__LINE__) ": " #expression                     \
                                          " passed when expected panic";       \
    std_set_abort_hook(old_abort);                                             \
    return;                                                                    \
  } while (0)

/**
 * Causes the test to fail if the expression doesn't exit with the non-zero exit
 * code [code]. Panics don't count as non-zero exits.
 */
#define IS_NONZERO_EXIT(expression, code)                                      \
  static_assert((code) != 0, "Exit code must be non-zero");                    \
  do {                                                                         \
    exit_hook old_exit = std_set_exit_hook(_std_accept_except_exit);           \
    int val = setjmp(_std_accept_exit_jmp);                                    \
    if (val == 0) {                                                            \
      expression;                                                              \
    } else if (val == -1) {                                                    \
      if (TEST_DNAME->is_verbose) {                                            \
        std_eprintf("> -- Expected non-zero exit occurred.\n");                \
      }                                                                        \
      std_set_exit_hook(old_exit);                                             \
      TEST_DNAME->state = testing_PASSED;                                      \
      return;                                                                  \
    }                                                                          \
    TEST_DNAME->state = testing_FAILED;                                        \
    TEST_DNAME->message = "line " _TEST_TO_STRING(                             \
        __LINE__) ": " #expression " passed when expected non-zero exit";      \
    std_set_exit_hook(old_exit);                                               \
    return;                                                                    \
  } while (0)

/* Registration Macros */

/**
 * Initializes standard information about this test file.
 */
#define INIT(argc, argv)                                                       \
  _std_test_data TEST_DNAME;                                                   \
  do {                                                                         \
    TEST_DNAME.failed = 0;                                                     \
    TEST_DNAME.run = 0;                                                        \
    TEST_DNAME.skipped = 0;                                                    \
    TEST_DNAME.message = "";                                                   \
    TEST_DNAME.state = testing_NOT_RUN;                                        \
    TEST_DNAME.is_verbose = (argc) > 1 && (argv)[1][0] == 'v';                 \
    std_eprintf("> Running tests for " __FILE__ ":\n");                        \
  } while (0)

/**
 * Runs the test function [testname] defined using the [TEST] macro with
 * additional data defined as variadic arguments.
 */
#define RUN(testname, ...)                                                     \
  _TEST_ASSERT_DEFINED(testname);                                              \
  do {                                                                         \
    abort_hook old_abort = std_set_abort_hook(_std_test_abort);                \
    exit_hook old_exit = std_set_exit_hook(_std_test_exit);                    \
    sig_t old_sigsegv = signal(SIGSEGV, _std_sigsegv_handler);                 \
    int val = setjmp(_std_test_jmp);                                           \
    if (val == 0) {                                                            \
      TEST_DNAME.run++;                                                        \
      if (TEST_DNAME.is_verbose) {                                             \
        std_eprintf("> Running " #testname ".\n");                             \
      }                                                                        \
      testname(&TEST_DNAME __VA_OPT__(, ) __VA_ARGS__);                        \
    } else if (val == 1) {                                                     \
      TEST_DNAME.state = testing_FAILED;                                       \
      TEST_DNAME.message = "Encountered panic";                                \
    } else if (val == 2) {                                                     \
      TEST_DNAME.state = testing_FAILED;                                       \
      TEST_DNAME.message = "Exit with non-zero code";                          \
    } else if (val == 3) {                                                     \
      TEST_DNAME.state = testing_FAILED;                                       \
      TEST_DNAME.message = "Encountered SIGSEGV";                              \
    }                                                                          \
    if (TEST_DNAME.state == testing_FAILED) {                                  \
      TEST_DNAME.failed++;                                                     \
      std_eprintf("> -- FAILED: %s.\n", TEST_DNAME.message);                   \
    } else if (TEST_DNAME.is_verbose && TEST_DNAME.state == testing_PASSED) {  \
      std_eprintf("> -- PASSED.\n");                                           \
    }                                                                          \
    TEST_DNAME.state = testing_NOT_RUN;                                        \
    std_set_abort_hook(old_abort);                                             \
    std_set_exit_hook(old_exit);                                               \
    signal(SIGSEGV, old_sigsegv);                                              \
  } while (0)

/**
 * Skips running the test function [testname].
 */
#define SKIP(testname, ...)                                                    \
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
    std_eprintf("\n%d/%d failed tests (%d skipped).\n\n", TEST_DNAME.failed,   \
                TEST_DNAME.run + TEST_DNAME.skipped, TEST_DNAME.skipped);      \
    return TEST_DNAME.failed;                                                  \
  } while (0)

/**
 * Defines a test function named [testname]. Test function names should be valid
 * function names. Optional auxiliary parameters can also be added into the
 * test.
 *
 * Example:
 *  TEST(allocateTest, std_arena *arena, int flag)
 */
#define TEST(testname, ...)                                                    \
  void testname(_std_test_data *TEST_DNAME __VA_OPT__(, ) __VA_ARGS__)

typedef enum _std_test_state {
  testing_PASSED,
  testing_FAILED,
  testing_NOT_RUN,
} _std_test_state;

typedef struct _std_test_data {
  bool is_verbose;
  int32_t failed;
  int32_t run;
  int32_t skipped;
  const char *message;
  _std_test_state state;
} _std_test_data;

static jmp_buf _std_test_jmp; // Jumps buffer to avoid exceptions ending tests
static jmp_buf _std_accept_except_jmp; // Jumps to handle [IS_PANIC] tests
static jmp_buf _std_accept_exit_jmp; // Jumps to handle [IS_NONZERO_EXIT] tests

[[noreturn]]
static inline void _std_test_abort() {
  longjmp(_std_test_jmp, 1);
}

[[noreturn]]
static inline void _std_test_exit([[maybe_unused]] int code) {
  if (code != 0) {
    std_eprintf("> -- Test exited with code %d.\n", code);
    longjmp(_std_test_jmp, 2);
  } else
    longjmp(_std_test_jmp, -1);
}

[[noreturn]]
static inline void _std_accept_except_abort() {
  longjmp(_std_accept_except_jmp, -1);
}

[[noreturn]]
static inline void _std_accept_except_exit(int code) {
  if (code != 0) {
    std_eprintf("> -- Test exited with code %d.\n", code);
    longjmp(_std_accept_exit_jmp, -1);
  } else {
    longjmp(_std_accept_exit_jmp, 1);
  }
}

[[noreturn]]
static inline void _std_sigsegv_handler([[maybe_unused]] int sig) {
  longjmp(_std_test_jmp, 3);
}
