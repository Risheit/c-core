file(GLOB SOURCES CONFIGURE_DEPENDS "*.c")

foreach(source ${SOURCES})
  get_filename_component(TARGET_NAME ${source} NAME_WE)
  add_executable(${TARGET_NAME} ${source})
  target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra)
  target_link_libraries(${TARGET_NAME} PRIVATE std)
endforeach()
