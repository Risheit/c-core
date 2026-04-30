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

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

typedef enum internal_flags {
  IS_ALLOCATED = 1 << 0, // Whether this arena's backing memory is valid.
  IS_STACK = 1 << 1,     // Whether this arena's backing memory is on the stack.
} internal_flags;

// A single dynamic memory expansion page for an arena.
// The structure is metadata stored on the first few bytes of
// its underlying memory.
typedef struct mem_page {
  alignas(ARENA_ALIGN) size_t size; // The memory page size.
  size_t offset;                    // Current offset in memory page.
  struct mem_page *next_page;       // Optional next memory page.
                                    // [NULL] if doesn't exist.
  byte *memory;                     // Backing memory.
} mem_page;

#define BASE_MEM_PAGE_OFFSET                                                   \
  (sizeof(mem_page) + align_forward(sizeof(mem_page)))

struct std_arena {
  alignas(ARENA_ALIGN) size_t size; // Arena size.
  enum std_arena_flags flags;       // Set arena flags.
  enum internal_flags iflags;       // Set internal arena flags.
  mem_page *first_page;             // Backing memory page.
  mem_page *cur_page;               // Page in use.
};

static_assert(ARENA_META_SIZE == sizeof(std_arena) + sizeof(mem_page),
              "Incorrectly defined ARENA_META_SIZE: Should not be " TO_STRING(
                  ARENA_META_SIZE));
static_assert(alignof(std_arena) == ARENA_ALIGN);
static_assert(alignof(mem_page) == ARENA_ALIGN);

// Returns true iff the allocation succeeded.
// Doesn't allocate if size is 0, to allow for 0-size arena creations.
static bool alloc_mem_page(mem_page **page, size_t size) {
  if (size == 0) {
    *page = nullptr;
    return true;
  }

  byte *backing_memory = malloc(BASE_MEM_PAGE_OFFSET + size);

  if (backing_memory == nullptr) {
    return false;
  }

  mem_page *new_page = (mem_page *)backing_memory;
  new_page->size = size + BASE_MEM_PAGE_OFFSET;
  new_page->offset = BASE_MEM_PAGE_OFFSET;
  new_page->next_page = nullptr;
  new_page->memory = backing_memory;
  *page = new_page;

  return true;
}

std_arena *std_arena_create(size_t size, std_arena_flags flags) {
  std_arena *arena = malloc(sizeof(std_arena));

  if (!(flags & ARENA_CONT_ON_ALLOC_FAIL) && arena == nullptr) {
    std_panic("Unable to allocate space for arena");
  }

  arena->flags = flags;

  if (!(flags & ARENA_CONT_ON_ALLOC_FAIL) &&
      !alloc_mem_page(&arena->first_page, size)) {
    free(arena);
    std_panic("Unable to allocate space for arena");
  }

  arena->size = size;
  arena->cur_page = arena->first_page;
  arena->iflags = IS_ALLOCATED;

  return arena;
}

std_arena *std_arena_create_s(void *memory, size_t size,
                              std_arena_flags flags) {
  std_nonnull(memory);
  std_assert(size > ARENA_META_SIZE,
             "arena size must be able to contain an arena object (more than "
             "%zu bytes)",
             ARENA_META_SIZE);

  size_t backing_memory_size = size - sizeof(std_arena) - sizeof(mem_page);

  std_arena *arena = (std_arena *)memory;
  arena->size = backing_memory_size;
  arena->flags = flags;
  arena->first_page = (mem_page *)(memory + sizeof(arena));

  arena->first_page->size = backing_memory_size;
  arena->first_page->offset = BASE_MEM_PAGE_OFFSET;
  arena->first_page->next_page = nullptr;

  // This won't be freed, so we're good to not point to the start of
  // the allocated memory. We keep the offset the same with dynamic
  // arenas to avoid special cases when cleaning arenas.
  arena->first_page->memory = memory + sizeof(arena);

  arena->cur_page = arena->first_page;
  arena->iflags = IS_STACK | IS_ALLOCATED;

  return arena;
}

void std_arena_destroy(std_arena *arena) {
  if (arena->iflags & IS_ALLOCATED) {
    return;
  }

  arena->iflags &= ~IS_ALLOCATED;

  if (arena->iflags & IS_STACK) {
    return;
  }

  mem_page *cur_page = arena->first_page;
  while (cur_page != nullptr) {
    mem_page *next_page = cur_page->next_page;
    free(cur_page->memory);
    cur_page = next_page;
  }

  free(arena);
}

// Allocates a new memory page. Returns true on success.
static bool resize(std_arena *arena, size_t min_size) {
  std_assert(!(arena->flags & ARENA_STOP_RESIZE),
             "Attempted to resize invalid arena");
  std_assert(arena->iflags & IS_STACK,
             "Attempted to resize externally-managed arena");
  std_assert(!(arena->iflags & IS_ALLOCATED),
             "Attempted to resize unallocated arena");

  mem_page *page = arena->cur_page;

  // Use existing allocated page if it exists.
  if (page != nullptr && page->next_page != nullptr) {
    std_assert(page->next_page->offset == BASE_MEM_PAGE_OFFSET,
               "Offset for cleaned page incorrectly set");
    arena->cur_page = page->next_page;
    return true;
  }

  // Create new page of EXPANSION_FACTOR size allocation.
  size_t cur_size = page != nullptr ? page->size : 1;
  size_t page_size = cur_size * EXPANSION_FACTOR;
  while (page_size <= min_size) {
    page_size *= EXPANSION_FACTOR;
  }
  page_size += align_forward(page_size);

  mem_page *new_page;
  if (!alloc_mem_page(&new_page, page_size)) {
    return false;
  }

  if (arena->cur_page == nullptr) { // 0-set arena
    std_assert(arena->first_page == nullptr, "Invalid page setup");
    arena->cur_page = new_page;
    arena->first_page = new_page;
  } else {
    arena->cur_page->next_page = new_page;
    arena->cur_page = new_page;
  }
  arena->size += new_page->size;

  return true;
}

void *std_arena_alloc(std_arena *arena, size_t size) {
  std_assert(std_arena_is_allocated(arena), "arena memory must exist");

  bool panic_on_alloc_fail = !(arena->flags & ARENA_CONT_ON_ALLOC_FAIL);
  size_t alloc_amt = align_forward(size) + size;

  // Resize if necessary and allowed
  if (arena->cur_page == nullptr ||
      arena->cur_page->size < alloc_amt + arena->cur_page->offset) {
    if (!(arena->flags & ARENA_STOP_RESIZE)) {
      if (!resize(arena, alloc_amt)) {
        if (panic_on_alloc_fail)
          std_panic("Failed to allocate %zu bytes", size);
        else
          return nullptr;
      }
    } else {
      if (panic_on_alloc_fail)
        std_panic("Failed to allocate %zu bytes", size);
      else
        return nullptr;
    }
  }

  // Allocate pointer
  mem_page *page = arena->cur_page;
  void *data = page->memory + page->offset;
  page->offset += size + align_forward(size);

  return data;
}

void std_arena_clean(std_arena *arena) {
  mem_page *cur_page = arena->first_page;

  // Cleans don't deallocate data, they just reset all offsets
  // to base.
  while (cur_page != nullptr) {
    cur_page->offset = BASE_MEM_PAGE_OFFSET;
    cur_page = cur_page->next_page;
  }
  arena->cur_page = arena->first_page;
}

bool std_arena_is_allocated(std_arena *arena) {
  return arena->iflags & IS_ALLOCATED;
}

size_t std_arena_size(std_arena *arena) {
  return arena->size;
}

void std_memset(void *buf, int val, size_t size) { memset(buf, val, size); }
