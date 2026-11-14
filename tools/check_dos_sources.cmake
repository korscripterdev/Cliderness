# Cliderness - verifies that builds/dos/sources.lst lists exactly the
# portable sources plus the DOS and null platform adapters.
#   cmake -DROOT=<repo> -P tools/check_dos_sources.cmake
file(GLOB_RECURSE all RELATIVE "${ROOT}" "${ROOT}/src/*.c")
list(FILTER all EXCLUDE REGEX "^src/platform/(windows|unix)/")
list(SORT all)
file(STRINGS "${ROOT}/builds/dos/sources.lst" listed REGEX "^[^#].*")
list(SORT listed)
if(NOT all STREQUAL listed)
  message(FATAL_ERROR "builds/dos/sources.lst is out of date.\n expected: ${all}\n listed:   ${listed}")
endif()
list(LENGTH all n)
message(STATUS "DOS source list OK (${n} files)")
