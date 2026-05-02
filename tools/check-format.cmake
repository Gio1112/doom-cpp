cmake_minimum_required(VERSION 3.22)

find_program(CLANG_FORMAT_EXE NAMES clang-format REQUIRED)

file(
  GLOB_RECURSE
  FORMAT_SOURCES
  "${SOURCE_DIR}/engine/*.cpp"
  "${SOURCE_DIR}/game/*.cpp"
  "${SOURCE_DIR}/include/*.hpp"
  "${SOURCE_DIR}/tests/*.cpp")

if(NOT FORMAT_SOURCES)
  message(FATAL_ERROR "No C++ source files found for clang-format")
endif()

execute_process(
  COMMAND "${CLANG_FORMAT_EXE}" --dry-run --Werror ${FORMAT_SOURCES}
  WORKING_DIRECTORY "${SOURCE_DIR}"
  RESULT_VARIABLE FORMAT_RESULT)

if(NOT FORMAT_RESULT EQUAL 0)
  message(FATAL_ERROR "clang-format check failed")
endif()
