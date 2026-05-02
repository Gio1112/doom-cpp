cmake_minimum_required(VERSION 3.22)

project(
  doom_cpp
  VERSION 0.1.0
  DESCRIPTION "A C++20 Doom engine implementation"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

add_library(doom_cpp_options INTERFACE)
target_compile_features(doom_cpp_options INTERFACE cxx_std_20)

if(MSVC)
  target_compile_options(doom_cpp_options INTERFACE /W4 /WX)
else()
  target_compile_options(doom_cpp_options INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(doom_cpp_game game/wad.cpp)
target_include_directories(doom_cpp_game PUBLIC include)
target_link_libraries(doom_cpp_game PUBLIC doom_cpp_options)

add_executable(doom_cpp engine/main.cpp)
target_include_directories(doom_cpp PRIVATE include)
target_link_libraries(doom_cpp PRIVATE doom_cpp_game doom_cpp_options)

if(BUILD_TESTING)
  add_executable(bootstrap_tests tests/bootstrap_test.cpp)
  target_include_directories(bootstrap_tests PRIVATE include)
  target_link_libraries(bootstrap_tests PRIVATE doom_cpp_options)
  target_compile_definitions(bootstrap_tests
                             PRIVATE DOOMCPP_BINARY_PATH="$<TARGET_FILE:doom_cpp>")
  add_test(NAME bootstrap_runs COMMAND bootstrap_tests)

  add_executable(wad_tests tests/wad_test.cpp)
  target_link_libraries(wad_tests PRIVATE doom_cpp_game doom_cpp_options)
  target_compile_definitions(
    wad_tests PRIVATE
                      DOOMCPP_FREEDOOM1_WAD="${CMAKE_SOURCE_DIR}/assets/freedoom1.wad")
  add_test(NAME wad_directory_loads COMMAND wad_tests)
endif()
