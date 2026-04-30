/*
 * Defines the test runner executable.
 *
 * Any single test file should compile into a single executable file.
 * The test executable takes in all test executables and runs them in
 * sequence. Tests files executables should return the total number of failing
 * tests. E.x. A working test file should return 0, while a test file containing
 * 2 failing tests should return 2.
 *
 * Use [test -h] or [test --help] for more information about the flags
 * the test runner can take.
 *
 * See docs/testing.md for information on writing tests.
 */

#include "std/cli.h"
#include "std/error.h"
#include "std/memory.h"
#include "std/strings.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct test_opts {
  int log_level; // 0 - print essential
                 // 1 - print test names as they run
} test_opts;

typedef struct test_node {
  std_string test_file;
  struct test_node *next;
} test_node;

void print_help(const char *name) {
  std_printf("%s [-l level | --  [-f file | --file=file] ... \n", name);
  std_printf("\t -f, --file: Runs the given test file.\n");
}

void add_test_file(std_arena *restrict arena, test_node **restrict head,
                   std_string file_name) {
  test_node *new_test = std_arena_alloc(arena, sizeof(test_node));
  new_test->test_file = file_name;
  new_test->next = *head;
  *head = new_test;
}

int run_tests(test_node *head) {
  uint32_t failed_tests = 0;
  uint32_t invalid_tests = 0;

  std_eprintf("\nRunning Tests:\n\n");
  test_node *cur = head;
  while (cur != nullptr) {
    // Argument strings are null-terminated
    int ret = system(std_buf(cur->test_file));
    if (ret == -1) {
      std_eprintf("Unable to launch test file: %s\n", std_buf(cur->test_file));
      invalid_tests++;
    } else if (ret > 0) {
      failed_tests++;
    }
    cur = cur->next;
  }

  if (failed_tests == 0) {
    std_printf("\nThere were no failing tests files!\n");
  } else {
    std_printf("\nIn total, there were %d failing test file(s).\n",
               failed_tests);
  }
  if (invalid_tests > 0) {
    std_printf("\nUnable to launch some test file(s)!\n");
  }

  return failed_tests;
}

// Macro prevents pointer decay for c strings, making the str() macro faster.
#define is_option(arg, sopt, lopt)                                          \
  (std_cli_is_option((arg), str((sopt))) ||                                    \
   std_cli_is_option((arg), str((lopt))))

int main(int argc, const char **argv) {
  // TODO: Implement directory parsing for test files.

  std_arena *arena = std_arena_create(sizeof(test_node), 0);
  test_node *head = nullptr;
  test_opts opts = {.log_level = 0};

  std_argument arg = std_cli_argv_next(argc, argv);
  while (arg.type != ARG_END) {
    if (is_option(arg, "-h", "--help")) {
      print_help(argv[0]);
    } else if (is_option(arg, "-f", "--file")) {
      std_string val = std_cli_get_arg(arg, argc, argv);
      if (std_str_is_empty(val)) {
        std_eprintf("Missing argument filename for -f or --file option.\n");
        return 2;
      }
      add_test_file(arena, &head, val);
    } else if (is_option(arg, "-l", "--log")) {

    } else {
      std_eprintf("Unrecognized option provided.\n");
      print_help(argv[0]);
      return 2;
    }

    arg = std_cli_argv_next(argc, argv);
  }

  int failed = run_tests(head) > 0 ? 1 : 0;
  std_arena_destroy(arena);
  return failed;
}
