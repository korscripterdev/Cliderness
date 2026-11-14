# Cliderness - Open World Survival Sandbox
# Created by korscripterdev
#
# Builds the engine library (ISO C90), the game executable and the test
# runner for Windows, Linux, macOS and other Unix systems. DOS builds use
# builds/dos (Open Watcom / DJGPP), see docs/platforms/dos.md.
cmake_minimum_required(VERSION 3.10)
project(Cliderness VERSION 0.4.0 LANGUAGES C)

option(CLIDE_STRICT "Fail on non-C90 constructs in the engine (pedantic errors)" ON)
option(CLIDE_TESTS "Build the automated tests" ON)

file(GLOB_RECURSE CLIDE_ENGINE_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
list(FILTER CLIDE_ENGINE_SOURCES EXCLUDE REGEX "/src/main\\.c$")
list(FILTER CLIDE_ENGINE_SOURCES EXCLUDE REGEX "/src/platform/")

if(WIN32)
  set(CLIDE_PLATFORM_SOURCES src/platform/windows/win_platform.c)
else()
  set(CLIDE_PLATFORM_SOURCES src/platform/unix/posix_platform.c)
endif()
list(APPEND CLIDE_PLATFORM_SOURCES src/platform/null/null_platform.c)

add_library(clide_engine STATIC ${CLIDE_ENGINE_SOURCES} ${CLIDE_PLATFORM_SOURCES})
target_include_directories(clide_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
  target_compile_definitions(clide_engine PUBLIC _CRT_SECURE_NO_WARNINGS)
  target_compile_options(clide_engine PRIVATE /W3)
else()
  # Engine: strict ISO C90. Platform adapters include OS headers that are not
  # C90 clean, so they are compiled in C99 mode (their own code is C90 style).
  set(_c90 -std=c90 -Wall -Wextra -Wno-unused-parameter)
  if(CLIDE_STRICT)
    list(APPEND _c90 -pedantic-errors)
  endif()
  set_source_files_properties(${CLIDE_ENGINE_SOURCES} PROPERTIES COMPILE_OPTIONS "${_c90}")
  set_source_files_properties(${CLIDE_PLATFORM_SOURCES} PROPERTIES COMPILE_OPTIONS "-std=c99;-Wall")
  if(WIN32)
    target_compile_definitions(clide_engine PUBLIC _CRT_SECURE_NO_WARNINGS)
  endif()
endif()

add_executable(cliderness src/main.c)
target_link_libraries(cliderness PRIVATE clide_engine)
if(NOT MSVC)
  set_source_files_properties(src/main.c PROPERTIES COMPILE_OPTIONS "-std=c90;-pedantic-errors;-Wall")
endif()

# copy data next to the executable so it runs from the build directory
add_custom_command(TARGET cliderness POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/data $<TARGET_FILE_DIR:cliderness>/data)

if(CLIDE_TESTS)
  enable_testing()
  file(GLOB CLIDE_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c")
  add_executable(clide_tests ${CLIDE_TEST_SOURCES})
  target_link_libraries(clide_tests PRIVATE clide_engine)
  if(NOT MSVC)
    set_source_files_properties(${CLIDE_TEST_SOURCES} PROPERTIES COMPILE_OPTIONS "-std=c90;-pedantic-errors;-Wall")
  endif()
  add_test(NAME unit COMMAND clide_tests --data ${CMAKE_CURRENT_SOURCE_DIR}/data
           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  add_test(NAME version COMMAND cliderness --version)
  set_tests_properties(version PROPERTIES PASS_REGULAR_EXPRESSION "korscripterdev")
  add_test(NAME translations COMMAND cliderness --check-lang
           --data ${CMAKE_CURRENT_SOURCE_DIR}/data)
  add_test(NAME headless_menu COMMAND cliderness --data ${CMAKE_CURRENT_SOURCE_DIR}/data
           --headless "{WAIT:3}")
  set_tests_properties(headless_menu PROPERTIES PASS_REGULAR_EXPRESSION "Created by korscripterdev")
  add_test(NAME dos_data_names COMMAND ${CMAKE_COMMAND}
           -DROOT=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_dos_names.cmake)
  add_test(NAME dos_makefile_sync COMMAND ${CMAKE_COMMAND}
           -DROOT=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_dos_sources.cmake)
endif()
