#include "std/strings.h"
#include "std/error.h"
#include "std/memory.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define STR_VALID(str)                                                         \
  std_assert(std_str_err(str) == 0, "String has an error code %d",             \
             std_str_err(str))

static std_string str_alloc_n(std_arena *arena, const char *buf, size_t n) {
  char *memory = std_arena_alloc(arena, n * sizeof(buf));

  std_string string = {
      ._buf = nullptr, ._len = n, ._err = memory == NULL ? STERR_OMEM : 0};

  if (!memory)
    return string;

  strncpy(memory, buf, n);
  string._buf = memory;
  return string;
}

std_string std_str_create(std_arena *arena, const char *buf) {
  size_t n = strlen(buf);
  return str_alloc_n(arena, buf, n);
}

std_string std_str_const_create(const char *buf) {
  size_t n = strlen(buf);
  std_string string = {._buf = buf, ._len = n, ._err = 0};
  return string;
}

int std_str_compare(std_string a, std_string b) {
  // Early length check to try and avoid full string comp
  if (std_str_len(a) != std_str_len(b))
    return std_str_len(a) > std_str_len(b) ? 1 : -1;

  return strncmp(std_str_get(a), std_str_get(b), std_str_len(a));
}

std_string std_str_substr(std_string str, size_t from, size_t to) {
  STR_VALID(str);

  if (from >= to)
    return std_str_empty();

  size_t len = to > std_str_len(str) ? std_str_len(str) : to;
  std_string string = {
      ._buf = std_str_get(str) + from, ._len = len - from, ._err = false};

  return string;
}

struct std_str_token {
  size_t pos;         // The current position in the string.
  std_string value;   // The substring value.
  std_string *parent; // The parent string.
  char split;         // The split character
};

void std_str_tokenize(std_str_token *token, std_string string, char split) {
  STR_VALID(string);

  size_t start, end;
  for (start = 0; start < std_str_len(string); start++) {
    if (std_str_at(string, start) != split)
      break;
  }

  for (end = start; end < std_str_len(string); end++) {
    if (std_str_at(string, end) == split)
      break;
  }

  token->pos = end;
  token->parent = &string;
  token->value = std_str_substr(string, start, end);
  token->split = split;
}

bool std_str_token_next(std_str_token *token) {
  std_nonnull(token);

  if (token->pos > std_str_len(*token->parent)) {
    return false;
  }

  if (token->pos == std_str_len(*token->parent)) {
    token->pos++; // Increase past end of string so that token get calls can
                  // fail after end of string.
    return false;
  }

  size_t start, end;
  for (start = token->pos; start < std_str_len(*token->parent); start++) {
    if (std_str_at(*token->parent, start) != token->split)
      break;
  }

  for (end = start; end < std_str_len(*token->parent); end++) {
    if (std_str_at(*token->parent, end) == token->split)
      break;
  }

  token->value = std_str_substr(*token->parent, start, end);
  return true;
}

std_string std_str_token_get(std_str_token *token) {
  std_nonnull(token);
  if (token->pos > std_str_len(*token->parent))
    return std_str_bad_ped(STERR_READ);
  return token->value;
}

size_t std_str_find(std_string str, char c) {
  STR_VALID(str);

  size_t i;
  for (i = 0; i < std_str_len(str); i++) {
    if (std_str_at(str, i) == c)
      return i;
  }
  return i;
}

size_t std_str_len(std_string str) {
  STR_VALID(str);
  return str._len;
}

std_string std_str_append(std_arena *arena, std_string left, std_string right) {
  size_t size = std_str_len(left) + std_str_len(right);

  // Overflow with size.
  if (size < std_str_len(left) || size < std_str_len(right))
    return (std_string){._err = STERR_BIG};

  // Allocate enough space for left and right, then copy them into the buffer in
  // parts.
  char *res = std_arena_alloc(arena, size * sizeof(left._buf));
  if (res == NULL)
    return (std_string){._err = STERR_OMEM};

  strncpy(res, left._buf, std_str_len(left));
  strncpy(res + std_str_len(left), right._buf, std_str_len(right));
  return (std_string){._len = size, ._buf = res, ._err = 0};
}

std_string std_str_empty() {
  return (std_string){._buf = "", ._len = 0, ._err = 0};
}

std_string std_str_null() {
  return (std_string){._buf = "\0", ._len = 1, ._err = 0};
}

bool std_str_is_empty(std_string str) {
  STR_VALID(str);
  return std_str_len(str) == 0;
}

const char *std_str_get(std_string str) {
  STR_VALID(str);
  return str._buf;
}

const char *std_str_get_safe(std_arena *arena, std_string str) {
  STR_VALID(str);
  std_string safe_string = std_str_append(arena, str, std_str_null());
  return safe_string._buf;
}

char std_str_at(std_string str, size_t at) {
  STR_VALID(str);

  if (at >= str._len)
    std_panic("index greater than string length");

  return str._buf[at];
}

std_string std_str_bad() {
  std_string string = {._buf = "", ._len = 0, ._err = STERR_TPOT};
  return string;
}

std_string std_str_bad_ped(int err) {
  std_string string = {._buf = "", ._len = 0, ._err = err};
  return string;
}

int std_str_err(std_string str) { return str._err; }
