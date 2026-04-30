#include "std/error.h"

#include <stdio.h>
#include <stdlib.h>

void std_errno_msg(const char *msg) { perror(msg); }

__attribute__((format(printf, 1, 2))) int
std_eprintf(const char *restrict format, ...) {
  va_list args;
  va_start(args, format);
  int ret = vfprintf(stderr, format, args);
  va_end(args);

  return ret;
}

__attribute__((format(printf, 1, 2))) int
std_printf(const char *restrict format, ...) {
  va_list args;
  va_start(args, format);
  int ret = vfprintf(stderr, format, args);
  va_end(args);

  return ret;
}

[[noreturn]] void _std_builtin_assert(const char *filename, const char *func,
                                   int line, int expr, const char *err,
                                   const char *format, ...) {
  std_eprintf("Assertion failed: function %s, file %s, line %d\n%s: ", func,
              filename, line, err);

  va_list args;
  va_start(args, format);
  int ret = vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, ".\n");
  abort();
}

[[noreturn]] void _std_builtin_panic(const char *filename, const char *func,
                                     int line, const char *format, ...) {
  std_eprintf("Panic: ");

  va_list args;
  va_start(args, format);
  int ret = vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, ".\n");
  _std_hook_abort();
}

[[noreturn]] void std_abort() {
  _std_hook_abort();
}
