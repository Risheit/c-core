#include "std/memory.h"
#include "std/testing.h"
#include <stdint.h>

void *test_alloc(std_arena *arena, size_t size) {
  void *ptr = std_arena_alloc(arena, size);
  std_memset(ptr, 0, size);
  return ptr;
}

TEST(init_basic_arena) {
  std_arena *arena = std_dyn_arena();
  std_arena_destroy(arena);
  PASS_TEST(); // Expect no panic.
}

TEST(init_zero_arena) {
  std_arena *arena = std_arena_create(0, 0);

  IS_TRUE(std_arena_size(arena) == 0);
  test_alloc(arena, sizeof(int32_t));
  IS_TRUE(std_arena_size(arena) > 0);
}

TEST(init_dynamic_arena) {
  const int arena_size = 100;

  std_arena *arena = std_arena_create(arena_size, 0);

  void *alloc = std_arena_alloc(arena, arena_size);
  std_memset(alloc, 0, arena_size); // Test allocation
  std_arena_destroy(arena);
  PASS_TEST();
}

TEST(init_static_arena) {
  byte *buf[200];
  std_arena *arena = std_arena_create_s(buf, 200, 0);
  std_arena_destroy(arena);
  PASS_TEST();
}

int main() {
  INIT();

  RUN(init_basic_arena);
  // RUN(init_dynamic_arena);
  // RUN(init_static_arena);

  CONCLUDE();
}
