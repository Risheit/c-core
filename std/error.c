#include "std/error.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void (*g_abort_hook)(void) = abort;

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
  g_abort_hook();
  unreachable();
}

[[noreturn]] void _std_builtin_panic(const char *filename, const char *func,
                                     int line, const char *format, ...) {
  std_eprintf("Panic: ");

  va_list args;
  va_start(args, format);
  int ret = vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, ".\n");
  g_abort_hook();
  unreachable();
}

[[noreturn]] void std_abort() {
  g_abort_hook();
  unreachable();
}

abort_hook std_set_abort_hook(abort_hook hook) {
  abort_hook old_hook = g_abort_hook;
  g_abort_hook = hook;
  return old_hook;
}
