#include "std/memory.h"
#include "std/error.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Arena allocator implementation details from:
// https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/

// By what multiple is an arena grown on a reallocation?
#define EXPANSION_FACTOR 2

#define align_forward(size) std_align_forward((size), ARENA_ALIGN)

typedef enum internal_flags {
  IS_ALLOCATED = 1 << 0, // Whether this arena's backing memory is valid.
  IS_STACK = 1 << 1,     // Whether this arena's backing memory is on the stack.
  CANNOT_RESIZE = 1 << 2, // Whether this arena cannot facilitate resizing.
} internal_flags;

// A single dynamic memory expansion page for an arena.
// The structure is metadata stored on the first few bytes of
// its underlying memory.
// The allocation pattern for expansion pages is as follows:
// | mem_page struct | free memory |
typedef struct mem_page {
  alignas(ARENA_ALIGN) size_t size; // The memory page size.
  size_t offset;                    // Current offset in memory page.
  struct mem_page **next_page;      // Optional next memory page.
                                    // [NULL] if doesn't exist.
  byte *memory;                     // Backing memory.
} mem_page;

// The memory allocation pattern for the first arena page is as follows:
// | (optional) next_page ptr + alignment | std_arena struct | free memory |
//
// A next_page ptr is initialized only if there is room for the pointer.
struct std_arena {
  alignas(ARENA_ALIGN) size_t size; // The arena size.
  size_t offset;                    // Current offset in memory.
  enum std_arena_flags flags;       // Set arena flags.
  enum internal_flags iflags;       // Set internal arena flags.
  byte *memory;                     // Backing memory.
};

static_assert(ARENA_META_SIZE == sizeof(std_arena),
              "Incorrectly defined ARENA_META_SIZE.");

static_assert(RESIZABLE_ARENA_META_SIZE >=
                  sizeof(std_arena) + sizeof(mem_page **) +
                      align_forward(sizeof(mem_page **)),
              "Incorrectly defined RESIZABLE_ARENA_META_SIZE.");

std_arena *std_arena_create(size_t size, enum std_arena_flags flags) {
  // Allocate no more than [size], use some of the allocated space
  // to store arena metadata.
  if (flags & EXACT_ARENA_SIZING) {
    std_assert(size > ARENA_META_SIZE,
               "arena size must be able to contain an arena object (more than "
               "%zu bytes)",
               ARENA_META_SIZE);

    byte *memory = malloc(size);
    if (memory == NULL) {
      if (!(flags & CONTINUE_ON_ALLOC_FAILURE))
        std_panic("Failed to allocate memory for arena object");
      return nullptr;
    }

    // Attempt to allocate next_page ptr
    internal_flags iflags = IS_ALLOCATED;
    size_t offset = 0;
    if (size > RESIZABLE_ARENA_META_SIZE) {
      offset += sizeof(mem_page **) + align_forward(sizeof(mem_page **));
    } else {
      iflags |= CANNOT_RESIZE;
    }

    // Allocate arena metadata
    std_arena *arena = (std_arena *)(memory + offset);
    offset += sizeof *arena;
    memset(memory, 0, offset);

    arena->size = size;
    arena->offset = offset;
    arena->flags = flags;
    arena->iflags = iflags;
    arena->memory = memory;

    return arena;
  } else { // Allocate extra space for metadata and [mem_page *]
    byte *memory = malloc(RESIZABLE_ARENA_META_SIZE + size);
    if (memory == NULL) {
      if (!(flags & CONTINUE_ON_ALLOC_FAILURE))
        std_panic("Failed to allocate memory for arena object");
      return nullptr;
    }

    std_arena *arena = (std_arena *)(memory + sizeof(mem_page **) +
                                     align_forward(sizeof(mem_page **)));
    memset(memory, 0, RESIZABLE_ARENA_META_SIZE);

    arena->size = RESIZABLE_ARENA_META_SIZE + size;
    arena->offset = RESIZABLE_ARENA_META_SIZE;
    arena->flags = flags;
    arena->iflags = IS_ALLOCATED;
    arena->memory = memory;

    return arena;
  }
}

std_arena *std_arena_create_s(void *memory, size_t size, std_arena_flags flags) {
  std_arena *arena = memory;

  std_nonnull(memory);
  std_assert(size > sizeof *arena,
             "arena size must be able to contain an arena object (more than "
             "%zu bytes)",
             sizeof *arena);

  memset(arena, 0, sizeof *arena);
  arena->memory = memory;
  arena->offset = sizeof *arena;
  arena->flags = flags;
  arena->size = size;
  arena->iflags = IS_ALLOCATED | IS_STACK | CANNOT_RESIZE;

  return arena;
}

void std_arena_destroy(std_arena *arena) {
  arena->iflags &= ~IS_ALLOCATED;

  if (arena->iflags & IS_STACK) {
    return;
  }

  mem_page *page = nullptr;

  if (!(arena->iflags & CANNOT_RESIZE)) {
    // The first bytes are the mem_page ptr. We copy it from the main arena
    // page.
    page = *(mem_page **)(arena->memory);
  }

  // Delete all extra pages
  while (page != NULL) {
    mem_page *next_page = nullptr;
    if (page->next_page) {
      next_page = *page->next_page;
    }

    free(page->memory);
    page = next_page;
  }

  free(arena->memory);
}

// [min_realloc] is the minimum amount to be reallocated. The reallocate
// function will allocate more than [min_realloc] space always, to reduce
// the number of reallocations that need to occur.
static void reallocate(std_arena *arena, size_t min_realloc) {
  size_t realloc_amt = arena->size != 0 ? arena->size * EXPANSION_FACTOR
                                        : min_realloc * EXPANSION_FACTOR;

  while (realloc_amt <= min_realloc)
    realloc_amt *= EXPANSION_FACTOR;

  realloc_amt += align_forward(realloc_amt); // Keep reallocations word-aligned

  byte *realloc_mem = realloc(arena->memory, realloc_amt);
  arena->memory = realloc_mem;
  arena->size = realloc_amt;
  if (realloc_mem != NULL)
    arena->iflags |= IS_ALLOCATED;
  else
    arena->iflags &= ~IS_ALLOCATED;
}

// [min_size] is the minimum amount to be reallocated. The reallocate
// function will allocate more than [min_size] space always, to reduce
// the number of resizes that need to occur.
static void resize(std_arena *arena, size_t min_size) {
  std_assert(!(arena->flags & ARENA_STOP_RESIZE) ||
                 (arena->flags & CANNOT_RESIZE),
             "Attempted to resize invalid arena");
  size_t resize_amt = arena->size != 0 ? arena->size * EXPANSION_FACTOR
                                       : min_size * EXPANSION_FACTOR;

  while (resize_amt <= min_size)
    resize_amt *= EXPANSION_FACTOR;

  // Allocate space for mem_page metadata
  resize_amt += sizeof(mem_page);
  resize_amt += align_forward(resize_amt); // Keep reallocations word-aligned

  // Allocate a new mem_page and assign it to the head of the mem_page
  // list.

  // Assume first bytes are a mem page address
  mem_page **page = (mem_page **)(arena->memory);

  byte *memory = malloc(resize_amt);
}

void *std_arena_alloc(std_arena arena[static 1], size_t size) {
  std_assert(std_arena_is_allocated(arena), "arena memory must exist");

  size_t alloc_amt = align_forward(size) + size;

  if (arena->size < alloc_amt + arena->offset) {
    bool allow_realloc =
        !(arena->flags & ARENA_STOP_RESIZE) || (arena->flags & CANNOT_RESIZE);

    if (allow_realloc)
      reallocate(arena, alloc_amt + arena->offset);
    else {
      if (!(arena->flags & CONTINUE_ON_ALLOC_FAILURE))
        std_panic("Failed to allocate %zu bytes", size);
      return nullptr;
    }

    if (!std_arena_is_allocated(arena)) // Reallocation failed
    {
      if (!(arena->flags & CONTINUE_ON_ALLOC_FAILURE))
        std_panic("Failed to reallocate backing memory.");
      return nullptr;
    }
  }

  byte *ptr = arena->memory + arena->offset;
  memset(ptr, 0, size);
  arena->offset += alloc_amt;
  return ptr;
}

void std_arena_clean(std_arena *arena) { arena->offset = 0; }

bool std_arena_is_allocated(std_arena *arena) { return arena->iflags & IS_ALLOCATED; }
