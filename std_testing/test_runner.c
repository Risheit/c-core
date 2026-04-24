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

typedef struct test_node {
  std_string test_file;
  struct test_node *next;
} test_node;

void print_help(const char *name) {
  std_printf("%s [-f file | --file=file] ... \n", name);
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
  int32_t failed_tests = 0;

  std_eprintf("\nRunning Tests:\n\n");
  test_node *cur = head;
  while (cur != nullptr) {
    int ret = system(
        std_str_get(cur->test_file)); // Argument strings are null-terminated
    if (WIFEXITED(ret)) {
      failed_tests += WEXITSTATUS(ret);
    }
    cur = cur->next;
  }

  if (failed_tests == 0) {
    std_eprintf("\nThere were no failing tests files!\n");
  } else {
    std_eprintf("\nIn total, there were %d failing test file(s).\n",
                failed_tests);
  }

  return failed_tests;
}

int main(int argc, const char **argv) {
  // TODO: Implement directory parsing for test files.

  std_arena *arena = std_arena_create(sizeof(test_node), 0);
  test_node *head = nullptr;

  std_argument arg = std_cli_argv_next(argc, argv);
  while (arg.type != ARG_END) {
    if (std_cli_is_option(arg, str("-h")) ||
        std_cli_is_option(arg, str("--help"))) {
      print_help(argv[0]);
    } else if (std_cli_is_option(arg, str("-f")) ||
               std_cli_is_option(arg, str("--file"))) {
      if (!arg.option.has_arg) {
        std_eprintf("Missing argument filename for -f or --file option.\n");
        return 2;
      }

      add_test_file(arena, &head, arg.option.arg);
    } else {
      std_eprintf("Unrecognized option provided.\n");
      return 2;
    }

    arg = std_cli_argv_next(argc, argv);
  }

  int failed = run_tests(head) > 0 ? 1 : 0;
  std_arena_destroy(arena);
  return failed;
}
