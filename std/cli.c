#include "std/cli.h"
#include "std/strings.h"
#include <stdbool.h>

static int g_opt_cur = 1;
static bool g_is_args_region = false;

static std_arg_type get_arg_type(bool is_args_region, std_string arg) {
  if (is_args_region)
    return ARG_ARGUMENT;

  return std_str_at(arg, 0) == '-' ? ARG_OPTION : ARG_ARGUMENT;
}

static std_argument parse_long_opt(std_string opt) {
  size_t split = std_str_find(opt, '=');

  // Found argument of type --long
  if (split == std_str_len(opt))
    return (std_argument){
        .type = ARG_OPTION,
        .option = {.arg = std_str_empty(), .name = opt, .has_arg = false}};

  // Found argument of type --long=arg
  return (std_argument){
      .type = ARG_OPTION,
      .option = {.arg = std_str_substr(opt, split + 1, std_str_len(opt)),
                 .name = std_str_substr(opt, 0, split),
                 .has_arg = true}};
}

static std_argument parse_short_opt(std_string opt) {

  // Found argument of type -farg
  if (std_str_len(opt) > 2)
    return (std_argument){.type = ARG_OPTION,
                          .option = {.arg = std_str_substr(opt, 2, std_str_len(opt)),
                                     .name = std_str_substr(opt, 0, 2),
                                     .has_arg = true}};

  // Found argument of type -f
  return (std_argument){
      .type = ARG_OPTION,
      .option = {.arg = std_str_empty(), .name = opt, .has_arg = false}};
}

static std_argument parse_opt(std_string opt) {
  bool is_long_opt = std_str_compare(std_str_substr(opt, 0, 2), str("--")) == 0;
  return is_long_opt ? parse_long_opt(opt) : parse_short_opt(opt);
}

/**
 * Thread-safe version of [cli_argv_next]. [opt_cur] points to the current 
 * argument index of [argv], and [is_args_region] is true if the seen
 * [argv] element should be parsed as arguments always.
 */
static std_argument cli_argv_next_t(int *opt_cur, bool *is_args_region,
                                    int argc, const char **argv) {
  // No more arguments to parse
  if (*opt_cur >= argc)
    return (std_argument){.type = ARG_END};

  std_string this_arg = str(argv[*opt_cur]);
  (*opt_cur)++;

  // "--" represents beginning of argument only region.
  if (std_str_compare(this_arg, str("--")) == 0) {
    *is_args_region = true;
    this_arg = str(argv[*opt_cur]);
    (*opt_cur)++;
  }

  // option found
  if (get_arg_type(*is_args_region, this_arg) == ARG_OPTION) {
    std_argument opt = parse_opt(this_arg);
    return opt;
  }

  // argument found
  return (std_argument){.type = ARG_ARGUMENT, .argument = {.val = this_arg}};
}

std_argument std_cli_argv_next(int argc, const char **argv) {
  return cli_argv_next_t(&g_opt_cur, &g_is_args_region, argc, argv);
}

void std_cli_argv_reset() {
  g_opt_cur = 1;
  g_is_args_region = false;
}

bool std_cli_is_option(std_argument arg, std_string name) {
  return arg.type == ARG_OPTION && std_str_compare(arg.option.name, name) == 0;
}
