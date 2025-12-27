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
 * See [cli_argv_next] or [cli_parse] for parsing CLI arguments.
 */
typedef struct std_argument {
  union {
    struct {
      std_string name; // The option name (This includes any preceding "-")
      std_string arg;  // The option argument string. This is an empty string if
                       // the option has no argument.
      bool has_arg;    // Whether this option has an argument.
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
 * * [-farg] provides the [-f] short option with the argument [arg].
 * * [--long=arg] provides the [--long] long option with the argument [arg].
 * Arguments passed in of the form [-f arg] or [--long arg] are not recognized
 * by default, and options will be marked as having no argument. It is up to the
 * caller to recognize the succeeding [cli_argv_next] call as containing the
 * argument for the previous option.
 *
 * This function is not thread-safe. 
 */
std_argument cli_argv_next(int argc, const char **argv);

/**
 * Resets the state of argument parsing. Calling [next_argv] after calling this
 * function will begin argument parsing from the first argument.
 *
 * NOTE: This function is not thread-safe.
 */
void cli_argv_reset();

/**
 * Returns true if and only if [arg] is a CLI option, and it has the name
 * [name]. Note that option names including any preceding "-" characters.
 */
bool cli_is_option(std_argument arg, std_string name);

#endif // STD_CLI_H
