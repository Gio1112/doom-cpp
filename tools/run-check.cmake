cmake_minimum_required(VERSION 3.22)

get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(BUILD_DIR "${SOURCE_DIR}/build/check")

if(EXISTS "${BUILD_DIR}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${BUILD_DIR}"
    RESULT_VARIABLE CLEAN_RESULT)

  if(NOT CLEAN_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to clean generated build directory: ${BUILD_DIR}")
  endif()
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${BUILD_DIR}"
  RESULT_VARIABLE MKDIR_RESULT)

if(NOT MKDIR_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to create build directory: ${BUILD_DIR}")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -D "SOURCE_DIR=${SOURCE_DIR}" -P
          "${SOURCE_DIR}/tools/check-format.cmake"
  WORKING_DIRECTORY "${SOURCE_DIR}"
  RESULT_VARIABLE FORMAT_RESULT)

if(NOT FORMAT_RESULT EQUAL 0)
  message(FATAL_ERROR "Format verification failed")
endif()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${BUILD_DIR}" -G Ninja
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  WORKING_DIRECTORY "${SOURCE_DIR}"
  RESULT_VARIABLE CONFIGURE_RESULT)

if(NOT CONFIGURE_RESULT EQUAL 0)
  message(FATAL_ERROR "CMake configure failed")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -D "SOURCE_DIR=${SOURCE_DIR}" -D "BUILD_DIR=${BUILD_DIR}" -P
          "${SOURCE_DIR}/tools/check-tidy.cmake"
  WORKING_DIRECTORY "${SOURCE_DIR}"
  RESULT_VARIABLE TIDY_RESULT)

if(NOT TIDY_RESULT EQUAL 0)
  message(FATAL_ERROR "Static analysis failed")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --config Debug
  WORKING_DIRECTORY "${SOURCE_DIR}"
  RESULT_VARIABLE BUILD_RESULT)

if(NOT BUILD_RESULT EQUAL 0)
  message(FATAL_ERROR "Build verification failed")
endif()

execute_process(
  COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${BUILD_DIR}" --build-config Debug
          --output-on-failure
  WORKING_DIRECTORY "${SOURCE_DIR}"
  RESULT_VARIABLE TEST_RESULT)

if(NOT TEST_RESULT EQUAL 0)
  message(FATAL_ERROR "CTest verification failed")
endif()
