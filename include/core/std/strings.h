#ifndef STD_STRINGS_H
#define STD_STRINGS_H

#include "memory.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * String wrapper around raw char arrays. Elements should be accessed using
 * accessor functions rather than directly.
 *
 * Strings are immutable, and strings might arbitrarily refer to the same
 * internal [char] buffer if they are related. When a string is created
 * directly from a buffer using a function like [str], further modification of
 * that char buffer outside of string functions is undefined behaviour.
 *
 * A note on lifetimes:
 * Strings operate under the assumption that their underlying buffer will be
 * valid for the duration of the string's lifetime. Undefined behaviour occurs
 * if a backing string buffer, or an arena containing the backing string buffer
 * goes out of scope. Be careful when using standard strings as return values
 * that the underlying buffer is static, or has been allocated in an arena that
 * will continue to exist after function return.
 */
typedef struct std_string {
  size_t _len;
  const char *_buf;
  int _err;
} std_string;

enum {
  STERR_OMEM = 1, // Out of memory to allocate string.
  STERR_BIG = 2,  // String too big to be stored (length field overflows).
  STERR_TPOT = 3, // No reason given for bad string.
  STERR_READ = 4  // String could not be properly read.
};

/**
 * Initializes a dynamic string into [str] from a given null-terminated char
 * array constant [buf], allocating it to the arena [arena]. [buf] is copied
 * until it reaches the null terminator. Calling this function on a non
 * null-terminated string is undefined behaviour.
 *
 * On an error, the [err] field of string is set to a non-zero value.
 */
std_string str_create(std_arena *arena, const char *buf);

// NOLINTBEGIN(bugprone-sizeof-expression)
// When used with pointers, the expanded macro contains a sizeof(char *) 
// expression, which causes clang-tidy to issue a warning.

/**
 * A utility macro that wraps around [str_const_create].
 *
 * Unlike [str_const_create], which calculates the length of the string at
 * runtime, the [str] utility macro will perform compile time length calculation
 * of buffers when possible. This allows strings defined using [str] to be
 * placed inside static variables, and is more performant.
 */
#define str(buf)                                                               \
  _Generic(typeof(buf),                                                        \
      const char[]: (                                                          \
               std_string){._buf = (buf), ._len = sizeof(buf) - 1, ._err = 0}, \
      char[]: (std_string){._buf = (buf), ._len = sizeof(buf) - 1, ._err = 0}, \
      default: str_const_create(buf))

// NOLINTEND(bugprone-sizeof-expression)

/**
 * Initializes a string into [str] from a given null-terminated char array
 * constant [buf]. [buf] is copied until it reaches the null-terminator. Calling
 * this function on a non null-terminated string is undefined behaviour.
 *
 * This function assumes that the string buffer [buf] is externally managed and
 * will not change or be deallocated while the returned string is in scope.
 * Consider using this function over [str_create] when dealing with memory that
 * will be automatically freed, like stack-allocated [char] arrays or for
 * read-only [const char] arrays that will never be modified.
 *
 * Prefer using the [str] utility macro over this function.
 */
std_string str_const_create(const char *buf);

/**
 * Compares the two strings [a] and [b].
 * Returns 0 if they are equal, a positive integer if [a] > [b], and a negative
 * integer if [b] > [a].
 */
int str_compare(std_string a, std_string b);

/**
 * Get the substring of [str] starting at (inclusive) [from] to (exclusive)
 * [end]. Returns an empty string if [from] >= [to].
 */
std_string str_substr(std_string str, size_t from, size_t to);

/**
 * Tokenized component of a string.
 */
typedef struct std_str_token std_str_token;

/**
 * Tokenizes the string [str], splitting it into substrings at each instance of
 * [split], excluding the [split] character. The first token substring is stored
 * in [token]. The next token can be retrieved by calling [str_token_next]. The
 * substring the token signifies can be retrieved by calling [str_token_get].
 *
 * Multiple sequential [split] characters are ignored. For example: for a split
 * character "C", "CHelloCCWorldC" -> ["Hello", "World"]
 */
void str_tokenize(std_str_token *token, std_string str, char split);

/**
 * Returns the next token substring for [token]. Returns true if it could do
 * this successfully. Otherwise, return false if the end of string is reached,
 * with the token set to contain an error string.
 */
bool str_token_next(std_str_token *token);

/**
 * Returns the substring that is signified by [token].
 */
std_string str_token_get(std_str_token *token);

/**
 * Get the index location of the first occurrence of [c] in [str].
 */
size_t str_find(std_string str, char c);

/**
 * Get the length of string [str].
 */
size_t str_len(std_string str);

/**
 * Appends [left] to [right] and stores it in [arena]. If [arena] cannot store
 * the appended strings and doesn't panic, an error string is returned.
 */
std_string str_append(std_arena *arena, std_string left, std_string right);

/**
 * Returns an empty string with length 0.
 */
std_string str_empty();

/**
 * Returns a null string. Unlike an empty string, this string has a size of 1,
 * but only contains null-terminator characters. Use this to read an explicit
 * null-terminator into an [std_string] if needed.
 */
std_string str_null();

/**
 * Returns true iff the string is empty.
 */
bool str_isempty(std_string str);

/**
 * Get the underlying buffer of the string [str].
 *
 * Note that this buffer is not guaranteed to be null-terminated! Cap all
 * interactions with raw buffers with the string's length using [str_len], or
 * use [str_get_safe] to get a safe null-terminated copy.
 */
const char *str_get(std_string str);

/**
 * Gets a guaranteed null-terminated buffer of the string [str]. This
 * null-terminated copy is stored in [arena].
 *
 * Note that this null-terminated version is created from a copy of this string.
 * Use [str_get] to access the underlying buffer without a copy, but note that
 * buffer is not guaranteed to be null-terminated.
 */
const char *str_get_safe(std_arena *arena, std_string str);

/**
 * Get the character at the index [at] in [str]. Throws an error if [at] is
 * past the end of the string.
 */
char str_at(std_string str, size_t at);

/**
 * Returns an invalid string.
 */
std_string str_bad();

/**
 * Returns an invalid string with a specific error code.
 */
std_string str_bad_ped(int err);

/**
 * Returns the error code of the string if it is invalid, and 0 if there is no
 * problem. See associated [STERR_] string error codes in this header file for
 * the meaning of each code.
 */
int str_err(std_string str);

#endif // STD_STRINGS_H
