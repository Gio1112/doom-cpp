cmake_minimum_required(VERSION 3.22)

find_program(CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)

file(
  GLOB_RECURSE
  TIDY_SOURCES
  "${SOURCE_DIR}/engine/*.cpp"
  "${SOURCE_DIR}/game/*.cpp"
  "${SOURCE_DIR}/tests/*.cpp")

if(NOT TIDY_SOURCES)
  message(FATAL_ERROR "No C++ source files found for clang-tidy")
endif()

foreach(source_file IN LISTS TIDY_SOURCES)
  execute_process(
    COMMAND "${CLANG_TIDY_EXE}" "${source_file}" -p "${BUILD_DIR}"
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE TIDY_RESULT)

  if(NOT TIDY_RESULT EQUAL 0)
    message(FATAL_ERROR "clang-tidy failed for ${source_file}")
  endif()
endforeach()
