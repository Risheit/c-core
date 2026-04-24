add_library(std STATIC cli.c error.c io.c memory.c strings.c)
target_include_directories(
  std PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/core>
             $<INSTALL_INTERFACE:include/core>)

if(EXT_TESTS)
  target_include_directories(
    std PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/extension_tests>
               $<INSTALL_INTERFACE:include/extension_tests>)
endif()

add_library(CCORE::std ALIAS std)
