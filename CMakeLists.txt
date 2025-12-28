cmake_minimum_required(VERSION 3.20)

project(
  C_Core
  VERSION 0.1.0
  LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Configuring C-Core library")
add_subdirectory(std/)

option(EXT_TESTS "Optional extension for running tests" OFF)
if(EXT_TESTS)
  message(STATUS "Configuring the C-Core testing framework")
  add_subdirectory(tests/)
endif()

option(EXT_SDL "Optional extension for running SDL" OFF)
if(EXT_SDL)
  message(STATUS "Configuring included SDL3 library")
  add_subdirectory(vendor/sdl)
  message(STATUS "Configuring C-Core SDL3 extenstions")
  add_subdirectory(sdl)
endif()
