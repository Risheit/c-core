#ifndef STD_ERROR_H
#define STD_ERROR_H

#include <errno.h>
#include <stdarg.h>

/**
 * Equivalent to standard stdio.h [perror] function.
 */
void errno_msg(const char *msg);

/**
 * Prints formatted output to standard error.
 */
int eprintf(const char *restrict format, ...)
    __attribute__((format(printf, 1, 2)));

#ifdef __builtin_expect
#define _std_builtin_expect(e, v) __builtin_expect((e), (v))
#else
#define _std_builtin_expect(e, v) (e)
#endif

#ifndef NDEBUG
/**
 * Exits the program if the given expression is true, printing a (printf)
 * formatted message. Asserts are stripped out of code in production
 * environments (when [NDEBUG] is defined), and should not be used to handle
 * regular control flow.
 */
#define std_assert(e, format, ...)                                             \
  (_std_builtin_expect(!(e), 0)                                                \
       ? _std_builtin_assert(__FILE_NAME__, __func__, __LINE__, e, #e,         \
                             format __VA_OPT__(, ) __VA_ARGS__)                \
       : (void)0)
#else
#define std_assert(e, format, ...)
#endif

#define std_nonnull(e) std_assert((e) != NULL, #e " should not be null")

/**
 * Forcefully crashes the program and generates a crash report with semantics
 * equivalent to the standard [abort] function.
 */
#define std_panic(format, ...)                                                 \
  _std_builtin_panic(__FILE_NAME__, __func__, __LINE__,                        \
                     format __VA_OPT__(, ) __VA_ARGS__)

/**
 * Standard library assert function. Use [std_assert].
 */
_Noreturn void _std_builtin_assert(const char *filename, const char *func,
                                   int line, int expr, const char *err,
                                   const char *format, ...)
    __attribute__((format(printf, 6, 7)));

/**
 * Standard library panic function. Use [std_panic].
 */
_Noreturn void _std_builtin_panic(const char *filename, const char *func,
                                  int line, const char *format, ...)
    __attribute__((format(printf, 4, 5)));

#endif // STD_ERROR_H
