add_library(sdl STATIC timer.c)
target_link_libraries(sdl PUBLIC SDL3::SDL3 CCORE::std)
target_include_directories(
  sdl PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/extension_sdl>
             $<INSTALL_INTERFACE:include/extension_sdl>)

add_library(CCORE::sdl ALIAS sdl)
