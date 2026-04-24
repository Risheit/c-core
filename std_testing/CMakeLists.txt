add_executable(test test_runner.c)
target_compile_options(test PRIVATE -Wall -Wextra)
target_link_libraries(test PRIVATE std)
