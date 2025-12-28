#include "std/io.h"
#include "std/error.h"
#include "std/memory.h"
#include "std/strings.h"
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syslimits.h>

typedef struct std_file {
  FILE *handle;
  int err;
  bool active;
} std_file;

#define RAW(file) (file).handle
#define ACTIVE(file) std_assert((file).active, #file " should be active")

static const char *fopen_string(std_fopen_state state, bool dont_overwrite) {
  switch (state) {
  case FOPEN_READ:
    return "r";
  case FOPEN_READPL:
    return "r+";
  case FOPEN_WRITE:
    return dont_overwrite ? "wx" : "w";
  case FOPEN_WRITEPL:
    return dont_overwrite ? "wx+" : "w+";
  case FOPEN_APPEND:
    return "a";
  case FOPEN_APPENDPL:
    return "a+";
  }
}

std_file *std_file_open(std_arena *arena, std_string name, std_fopen_state state,
                    std_fopen_flags flags) {
  // Create a copy of [name] that is guaranteed to be null-terminated by
  // appending a null-terminator to the EOS.
  char buf[ARENA_META_SIZE + NAME_MAX + 1]; // Max path length buffer.
  std_arena *working_memory =
      std_arena_create_s(buf, (ARENA_META_SIZE + NAME_MAX + 1) * sizeof buf[0], 0);

  std_file *file = std_arena_alloc(arena, sizeof(std_file));
  if (!file) {
    return nullptr;
  }

  file->handle = fopen(std_str_get_safe(working_memory, name),
                       fopen_string(state, flags & FOPEN_NO_OVERWRITE));
  file->err = 0;
  file->active = true;

  // Failed to open file.
  if (!RAW(*file)) {
    file->err = errno;
    file->active = false;
  }

  return file;
}

std_file *std_file_temp(std_arena *arena) {
  std_file *file = std_arena_alloc(arena, sizeof(std_file));
  file->handle = tmpfile();
  file->err = 0;
  file->active = true;

  // Failed to open file.
  if (!RAW(*file)) {
    file->err = errno;
    file->active = false;
  }

  return file;
}

void std_file_close(std_file *file) {
  std_nonnull(file);
  ACTIVE(*file);

  int ret = fclose(file->handle);
  if (ret != 0) {
    file->err = errno;
  }
  file->active = false;
}

std_string std_file_read_line(std_arena *restrict arena, std_file *restrict file) {
  std_nonnull(arena);
  std_nonnull(file);
  ACTIVE(*file);

  size_t len;
  char *line = fgetln(file->handle, &len);

  // Failed to read line
  if (line == NULL) {
    if (feof(RAW(*file))) { // EOF reached
      file->err = FERR_EOF;
      return std_str_empty();
    }
    if (ferror(RAW(*file))) { // Error reached
      file->err = errno;
      return std_str_bad_ped(STERR_READ);
    }
  }

  return std_str_create(arena, line);
}

size_t std_file_write(const void *restrict ptr, size_t size, size_t n,
                  std_file *restrict file) {
  std_nonnull(ptr);
  std_nonnull(file);
  ACTIVE(*file);

  size_t write = fwrite(ptr, size, n, RAW(*file));
  if (write < n) {
    file->err = FERR_WRITE;
  }

  return write;
}

void std_file_flush(std_file *file) {
  std_nonnull(file);
  ACTIVE(*file);

  int res = fflush(RAW(*file));
  if (!res)
    file->err = errno;
}

long std_file_tell(std_file *file) {
  std_nonnull(file);
  ACTIVE(*file);

  long offset = ftell(RAW(*file));
  if (offset == -1)
    file->err = errno;

  return offset;
}

void std_file_seek(std_file *file, long offset, std_seek_pos whence) {
  std_nonnull(file);
  ACTIVE(*file);

  int sys_whence; // System whence value based on stdio.h
  switch (whence) {
  case SEEK_SET:
    sys_whence = SEEK_SET;
    break;
  case FSEEK_CUR:
    sys_whence = SEEK_CUR;
    break;
  case FSEEK_END:
    sys_whence = SEEK_END;
    break;
  }

  int res = fseek(RAW(*file), offset, sys_whence);
}

int std_file_err(const std_file *file) {
  std_nonnull(file);
  return file->err;
}

void std_file_reset_err(std_file *file) {
  std_nonnull(file);
  file->err = 0;
}

bool std_file_is_active(const std_file *file) {
  std_nonnull(file);
  return file->active;
}
