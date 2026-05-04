#include "std/memory.h"
#include "std/testing.h"
#include <stdint.h>

void *test_alloc(std_arena *arena, size_t size) {
  void *ptr = std_arena_alloc(arena, size);
  std_memset(ptr, 0xDD, size);
  return ptr;
}

TEST(init_basic_arena) {
  std_arena *arena = std_dyn_arena();
  std_arena_destroy(arena);
  PASS_TEST(); // Expect no panic.
}

TEST(resize_zero_arena) {
  const size_t alloc_amt = sizeof(int32_t);
  std_arena *arena = std_arena_create(0, 0);

  IS_TRUE(std_arena_size(arena) == 0);
  test_alloc(arena, alloc_amt);
  IS_TRUE(std_arena_size(arena) >= alloc_amt);
  std_arena_destroy(arena);
}

TEST(init_dynamic_arena) {
  const int arena_size = 100;

  std_arena *arena = std_arena_create(arena_size, 0);

  size_t allocated_arena_size = std_arena_size(arena);
  test_alloc(arena, arena_size);
  IS_TRUE(std_arena_size(arena) == allocated_arena_size);
  std_arena_destroy(arena);
}

TEST(init_static_arena) {
  const int arena_size = 200;
  byte *buf[200];
  std_memset(buf, 0xCC, arena_size);

  std_arena *arena = std_arena_create_s(buf, arena_size, 0);
  test_alloc(arena, arena_size / 2); // Allocate within arena size

  IS_TRUE(std_arena_size(arena) == arena_size - ARENA_META_SIZE); // No resize
  std_arena_destroy(arena);
}

TEST(stop_resize_flag) {
  const int arena_size = sizeof(int32_t);
  std_arena *arena = std_arena_create(arena_size, ARENA_STOP_RESIZE);

  size_t alloc_arena_size = std_arena_size(arena);
  test_alloc(arena, arena_size - 1);
  IS_TRUE(std_arena_size(arena) == alloc_arena_size);
  IS_PANIC(test_alloc(arena, arena_size - 1));
  std_arena_destroy(arena);
}

TEST(cont_on_alloc_failure_flag) {
  const int arena_size = 10;
  byte *buf[10];
  std_arena *arena =
      std_arena_create_s(buf, arena_size, ARENA_CONT_ON_ALLOC_FAIL);

  void *alloc = std_arena_alloc(arena, arena_size * 2);
  IS_TRUE(alloc == nullptr);

  std_arena_destroy(arena);
}

int main(int argc, char **argv) {
  INIT(argc, argv);

  RUN(init_dynamic_arena);
  RUN(init_basic_arena);
  RUN(init_static_arena);
  RUN(resize_zero_arena);
  RUN(stop_resize_flag);

  CONCLUDE();
}
