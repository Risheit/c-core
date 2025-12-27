#ifndef STD_MEMORY_H
#define STD_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef ARENA_ALIGN

/**
 * Default memory alignment. Can be overridden by defining [ARENA_ALIGN]
 * before including [memory.h]
 */
#define ARENA_ALIGN alignof(max_align_t)
#endif

/**
 * Convenience type of the smallest possible representation of data on a
 * machine. This is a minimum of one byte, but may be greater on some machines.
 */
typedef uint_least8_t byte;

typedef enum std_arena_flags {
  ARENA_STOP_RESIZE = 1 << 0,         // Stops this arena's backing memory
                                      // should not be resized. This means
                                      // nothing for arenas with externally
                                      // managed backing memory.
  CONTINUE_ON_ALLOC_FAILURE = 1 << 1, // Stops this arena from panicking if
                                      // backing memory fails to be allocated.
  EXACT_ARENA_SIZING = 1 << 2,        // Prevents allocating more than the
                                      // given [size] when originally creating
                                      // this arena. This means nothing for
                                      // arenas with externally managed backing
                                      // memory.
} std_arena_flags;

/**
 * Memory storage type. Arenas store large chunks of contiguous memory, and
 * allow for smaller allocations within using [arena_alloc]. Memory allocated
 * within an arena is guaranteed to exist for the lifetime of that arena. Blocks
 * of memory cannot be individually freed, though an arena can be cleaned for
 * reuse using [arena_clean] or freed entirely using [arena_destroy].
 *
 * [std_arena] is single-threaded. Sharing an arena between multiple threads
 * can cause race conditions. Currently, no multi-thread support for arenas
 * exists.
 */
typedef struct std_arena std_arena;

#define std_align_forward(size, align)                                         \
  (((align) - ((size) % (align))) % (align))

/**
 * The size of a [std_arena] struct.
 */
#define ARENA_META_SIZE (2 * sizeof(size_t) + 2 * sizeof(int) + sizeof(byte *))

/**
 * The size of a [std_arena] struct plus extra space to facilitate resizable
 * arenas.
 */
#define RESIZABLE_ARENA_META_SIZE                                              \
  (sizeof(char **) + std_align_forward(sizeof(char **), ARENA_ALIGN) +         \
   ARENA_META_SIZE)

/**
 * Initializes an arena of initial size [size] bytes with [flags] flags. If
 * the allocation of an arena's backing memory or allocation of the arena itself
 * fails, then this function panics, unless [CONTINUE_ON_ALLOC_FAILURE] is
 * set, in which case it defines the arena's memory as unallocated, or
 * returns [NULL], respectively.
 *
 * Remarks:
 *
 * * Dynamically allocated arenas will often allocate more than is specified in
 * [size] internally. This can be prevented with the [EXACT_ARENA_SIZING] flag.
 * In this case, if possible, a portion of the allocated memory will be instead
 * used internally for metadata and to allow resizing. As such, the
 * usable memory in the arena will be smaller than [arena->size]. If there is
 * not enough excess space available to use to allow resizing, or if the
 * [ARENA_STOP_RESIZE] flag is set, then no space will be reserved for resizing
 * use, and the arena memory will not be expandable. The behaviour for arenas
 * allocated using [EXACT_ARENA_SIZING] is undefined if [size] is not greater
 * than [ARENA_META_SIZE].
 *
 * * When creating a new arena with [EXACT_ARENA_SIZING], allocate a size
 * greater than [ARENA_META_SIZE], or allocate a size greater than
 * [RESIZABLE_ARENA_META_SIZE] to allow resizing.
 *
 * * Each new resizing grows the backing memory roughly by a multiplicative
 * factor. As such, the number of resizes of an arena can be minimized by
 * providing a larger initial backing memory size.
 *
 * * By default, memory allocated in an arena is aligned to [max_align_t]. When
 * backing memory is not aligned to [max_align_t], the arena will possibly not
 * be able to use the entire memory. For example, assume an alignment of 8
 * bytes, and a backing memory of 7 bytes. If we attempt to allocate 3 bytes,
 * then 4, the second allocation will fail, as the arena will attempt to add 5
 * bytes of padding to the 3 byte allocation. Contrast this with a single 7-byte
 * allocation, which will succeed.
 *
 * Alignment can be overridden by defining [ARENA_ALIGN] to a custom value
 * before including [memory.h].
 */
std_arena *arena_create(size_t size, enum std_arena_flags flags);

/**
 * Macro create a simple dynamic arena with initial capacity storing 4
 * [size_t] objects.
 */
#define std_dyn_arena() arena_create(4 * sizeof(size_t), 0)

/**
 * Initializes the arena [arena] with [size] bytes and [flags] flags,
 * associated with the backing memory [memory]. If the backing memory
 * [memory] and [size] is not greater than [ARENA_META_SIZE], or [memory] is
 * [NULL], the function's behaviour is undefined.
 *
 * When creating an arena this way, it is assumed that [memory] and the
 * [arena] pointer are managed externally to the arena structure, and
 * reallocations and automatic memory freeing is prevented. Consider using
 * this function instead of [arena_create] when providing backing memory that
 * is freed automatically, such as a stack-allocated memory buffer.
 *
 * The returned arena pointer is also located in this memory, and is
 * destroyed when [memory] is destroyed.
 *
 * Arenas created this way are statically sized, and are treated equivalently
 * to an arena with the [ARENA_STOP_RESIZE] and [EXACT_ARENA_SIZING] flag
 * set, and these flags have no effect on arenas created this way. See
 * [arena_create] for remarks on the way dynamic and static sized arenas are
 * treated internally.
 */
std_arena *arena_create_s(void *memory, size_t size,
                          enum std_arena_flags flags);

/**
 * Frees memory allocated by an arena and sets its [is_allocated] flag to
 * [false]. Accessing an arena pointer after calling [arena_delete] is
 * undefined behaviour.
 *
 * Remarks:
 *
 * * Destroys are O(# of resizes performed) in performance.
 */
void arena_destroy(std_arena *arena);

/**
 * Allocate a pointer of [size] bytes within the arena. If allocation fails,
 * this function panics, unless [CONTINUE_ON_ALLOC_FAILURE] is set, in which
 * case it returns [NULL].
 *
 * By default, if a pointer allocation would cause the arena to run out of
 * space, this function reallocates more space to the arena. If
 * [ARENA_STOP_RESIZE] is set or the arena is allocated on using
 * [arena_create_s], then if the arena would resize memory, this function
 * fails (and maybe panics) instead. If an arena memory resizing occurs and
 * fails during this call and [CONTINUE_ON_ALLOC_FAILURE] is set, the arena
 * does not panic, and the function returns [NULL].
 */
void *arena_alloc(std_arena *arena, size_t size);

/**
 * De-allocates all memory initialized within an arena, allowing
 * re-allocation of previously allocated memory. Accessing any previous
 * memory allocated within an arena after calling [arena_clean] is undefined
 * behaviour.
 *
 * Remarks:
 *
 * * Cleans are O(# of resizes performed) in performance.
 *
 * * Cleans may reset dynamically expanded arenas to their original size. Any
 * reallocations previously performed must be performed again.
 */
void arena_clean(std_arena *arena);

/**
 * Returns [true] if the backing memory for this arena is correctly
 * allocated, and [false] otherwise.
 */
bool is_allocated(std_arena *arena);

#endif // STD_MEMORY_H
