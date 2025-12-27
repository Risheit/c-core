#ifndef STD_IO_H
#define STD_IO_H

#include "memory.h"
#include "strings.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * [FILE *] wrapper for handling file I/O. Elements should be accessed using
 * accessor functions rather than directly.
 */
typedef struct std_file std_file;

/**
 * Open states of a file. These are equivalent to fopen states.
 */
typedef enum std_fopen_state {
  FOPEN_READ,     // Equivalent to "r"
  FOPEN_READPL,   // Equivalent to "r+"
  FOPEN_WRITE,    // Equivalent to "w"
  FOPEN_WRITEPL,  // Equivalent to "w+"
  FOPEN_APPEND,   // Equivalent to "a"
  FOPEN_APPENDPL, // Equivalent to "a+"
} std_fopen_state;

typedef enum std_fopen_flags {
  FOPEN_NO_OVERWRITE = 1, // Equivalent to adding "x" to a "w" or "w+" call.
} std_fopen_flags;

enum {
  FERR_INACTIVE = -1, // Operation performed on inactive file.
  FERR_EOF = -2,      // Operation performed on EOF.
  FERR_WRITE = -3,    // Partially or completely failed a write operation.
};

/**
 * Opens a file. On a failure, errno is set as specified by [fopen], and the
 * file returned is inactive and marked with the relevant error number.
 * If the file fails to allocate, then [NULL] is returned.
 */
std_file *file_open(std_arena *arena, std_string name, std_fopen_state state,
                    std_fopen_flags flags);

/**
 * Opens a temporary file in accordance with the function [tmpfile].
 * On a failure, errno is set as specified by [tmpfile] and the file returned is
 * inactive and marked with the relevant error number. If the file fails to
 * allocate, then [NULL] is returned.
 */
std_file *file_temp(std_arena *arena);

/**
 * Closes a file. On a failure, errno is set as specified by [fclose], and
 * [file] is marked with the relevant error number. Irregardless of success, the
 * file object is marked inactive. If an inactive file is closed, nothing
 * occurs, and [file] is marked with [FERR_INACTIVE].
 */
void file_close(std_file *file);

/**
 * Reads up to and including a newline character or EOF in [file]. On a failure,
 * an error string is returned, errno is set as specified by [fgetln], and
 * [file] is marked with the relevant error number. The string containing the
 * read line is returned, allocated in [arena]. Note that if the line is the
 * last in a file that does not end in a newline, the returned text will not
 * contain a newline (in accordance with [fgetln]. On a read of EOF, an empty
 * string is returned, and [file] is marked with [FERR_EOF].
 */
std_string file_read_line(std_arena *restrict arena, std_file *restrict file);

/**
 * Writes [n] items to [file], each of size [size] from memory [ptr], with
 * semantics equivalent to [fwrite]. If an error occurs and less than [n] items
 * are written to the file, then [file] is marked with [FERR_WRITE]. Returns the
 * number of items written.
 */
size_t file_write(const void *restrict ptr, size_t size, size_t n,
                  std_file *restrict file);

/**
 * Flushes any buffered I/O to [file] with semantics equivalent to [fflush]. If
 * an error occurs, then errno is set and [file] is marked with the relevant
 * error number.
 */
void file_flush(std_file *file);

/**
 * Returns the current value of the file position indicator, with semantics
 * equivalent to [ftell]. If an error occurs, then -1 is returned, errno is set
 * as specified by [ftell], and [file] is marked with the relevant error number.
 */
long file_tell(std_file *file);

/**
 * Standard [whence] values allowed for [file_seek].
 */
typedef enum std_file_seek {
  FSEEK_SET = 0, // Seeks to the beginning of the file.
  FSEEK_CUR = 1, // Seeks to the current file position.
  FSEEK_END = 2, // Seeks to the end of the file.
} std_file_seek;

/**
 * Sets the position offset of [file], with semantics equivalent to [fseek].
 * [whence] can be set to [FSEEK_SET], [FSEEK_CUR], or [FSEEK_END]. If an error occurs,
 * then errno is set as specified by [fseek] and [file] is marked with the
 * relevant error number.
 */
void file_seek(std_file *file, long offset, std_file_seek whence);

/**
 * Returns the error code associated with [file]. The error code of a file is 0
 * if there is no error present. This error code is set by functions the last
 * function on this file that fails. The error code is equivalent to errno
 * values set by the called function, with negative error codes equivalent to
 * their respective FERR_ error codes.
 *
 * Note: successful calls don't set the file error code to 0. If a previous
 * function sets an error code, that code will remain on the file until cleared
 * by the [file_reset_err] function. If a function does error, it overwrites the
 * previous error value on the file. If the error code of a function is
 * important, store or check the error code with [file_err] before calling
 * another file function.
 */
int file_err(const std_file *file);

/**
 * Resets the error code associated with [file] to 0.
 */
void file_reset_err(std_file *file);

/**
 * Returns true if [file] is active, and false if not. Files are marked
 * inactive after they are closed, or if a critical error causes the file to
 * become inoperable. All file manipulation functions on inactive files do
 * nothing, and mark the file with an error of [FERR_INACTIVE].
 */
bool file_active(const std_file *file);

#endif // STD_IO_H
