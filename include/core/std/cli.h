#ifndef STD_CLI_H
#define STD_CLI_H

#include "strings.h"

typedef enum std_arg_type {
  ARG_OPTION,   // Is command line option (-a, --long)
  ARG_ARGUMENT, // Is command line argument (arg1)
  ARG_END,      // No further arguments or options available to parse.
} std_arg_type;

/**
 * Argument holder, which contains a command line option or argument.
 * If [type] is [ARG_OPTION], then the union [option] is available.
 * If [type] is [ARG_ARGUMENT], then the union [argument] is available.
 *
 * See [std_cli_argv_next] for parsing CLI arguments.
 */
typedef struct std_argument {
  union {
    struct {
      std_string name; // The option name (This includes any preceding "-")
      std_string _arg; // The option argument string. Use [std_cli_get_arg]
                       // instead of checking this directly.
      bool _has_arg;   // Whether this option has an argument.
    } option;
    struct {
      std_string val; // The argument string.
    } argument;
  };
  std_arg_type type; // The type of this argument. See [std_arg_type].
} std_argument;

/**
 * Returns the next argument or option unparsed within [argv]. Repeated calls
 * will let you iterate through passed arguments. If all [argc] arguments in
 * [argv] have been parsed, all further calls will return an argument of the
 * [ARG_END] type, with no contained values. This can be reset using
 * [cli_argv_reset].
 *
 * Options are any strings that
 * begin with "-", excluding "--". Options that have only one preceding "-" are
 * considered short options, while those with multiple preceding "-" are
 * considered long arguments.
 *
 * All other strings are considered arguments. Any
 * CLI arguments provided after a "--" are considered arguments, including
 * strings normally considered options.
 *
 * Options can be provided with option arguments.
 * * [-f] provides the [-f] short option with the argument [arg].
 * * [--long] provides the [--long] long option with the argument [arg].
 * The [std_cli_get_arg] function can be used to attempt to get an argument for
 * a given option if it exists.
 *
 * Arguments passed in of the form [-f arg] or [--long arg] are not recognized
 * by default, and options will be marked as having no argument. If an argument
 * is expected or allowed,
 *
 * This function is not thread-safe.
 */
std_argument std_cli_argv_next(int argc, const char **argv);

/**
 * Attempts to get the next argument for a previous option parsed [opt] using
 * [std_cli_argv_next], returning an empty string if no argument exists. This
 * function should only be called directly after a [std_cli_argv_next] call
 * that has parsed the option [opt], and not after an succeeding
 * [std_cli_argv_next] calls. It should not be called if the option [opt] cannot
 * have an argument.
 *
 * The following methods are valid when recognizing arguments:
 * * [-farg] or [-f arg] provide the argument [arg] to the flag [-f].
 * * [--long=arg] or [--long arg] provide the argument [arg] to the flag
 * [--long].
 *
 * Undefined behaviour if [opt] isn't an option.
 */
std_string std_cli_get_arg(std_argument opt, int argc, const char **argv);

/**
 * Resets the state of argument parsing. Calling [next_argv] after calling this
 * function will begin argument parsing from the first argument.
 *
 * This function is not thread-safe.
 */
void std_cli_argv_reset();

/**
 * Returns true if and only if [arg] is a CLI option, and it has the name
 * [name]. Note that option names including any preceding "-" characters.
 */
bool std_cli_is_option(std_argument arg, std_string name);

#endif // STD_CLI_H
