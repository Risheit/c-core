# c-core

A custom standard library around raw C libraries.

The goal of the this library is to provide safer and more modern wrappers
around old functionality, including proper string and file types, and memory arenas to reduce manual memory management.

## Library Compilation and Use

The standard library can be added to a CMake project through the following
steps.

1. Include the library in your CMake project (here in the vendored/c-core) directory.

```sh
git clone https://github.com/Risheit/c-core.git vendored/c-core
```

2. Add the library subdirectory to the root `CMakeLists.txt` file

```cmake
add_subdirectory(vendored/c-core EXCLUDE_FROM_ALL)
```

3. Include and link the standard library within the executable's `CMakeLists.txt` file

```cmake
target_link_libraries(${EXE_NAME} PRIVATE CCORE::std)
```

4. Access standard library headers under the `std/` include flag:

```c
#include <std/memory.h>
```

## Including the SDL3 extension.
C-Core provides SDL3 support, along with SDL3 extension under the `std/sdl` include path.
SDL3 support is under the `EXT_SDL` option, and can be enabled with the `-DEXT_SDL` command line option when configuring, or within CMake by adding the following set command before adding the `c-core` subdirectory.
```cmake
set(CACHE{EXT_SDL} HELP "Enable C-Core SDL extension" VALUE ON)
```

## Library use as a Submodule

Including the `c-core` directory as a git submodule instead of a raw directory makes it easy to update the `c-core` library from other projects. Add the submodule (to the path `vendored/c-core`) using

```bash
git submodule add https://github.com/Risheit/c-core/ vendored/c-core
```
