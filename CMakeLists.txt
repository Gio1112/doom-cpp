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

find_package(SDL2 REQUIRED)

add_library(doom_cpp_options INTERFACE)
target_compile_features(doom_cpp_options INTERFACE cxx_std_20)

if(MSVC)
  target_compile_options(doom_cpp_options INTERFACE /W4 /WX)
else()
  target_compile_options(doom_cpp_options INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(doom_cpp_game engine/input.cpp engine/render.cpp game/map.cpp game/sprites.cpp game/wad.cpp)
target_include_directories(doom_cpp_game PUBLIC include)
target_link_libraries(doom_cpp_game PUBLIC doom_cpp_options)

add_library(doom_cpp_platform engine/sdl_app.cpp)
target_include_directories(doom_cpp_platform PUBLIC include)
target_link_libraries(doom_cpp_platform PUBLIC doom_cpp_game doom_cpp_options SDL2::SDL2)

add_executable(doom_cpp engine/main.cpp)
target_include_directories(doom_cpp PRIVATE include)
target_link_libraries(doom_cpp PRIVATE doom_cpp_platform doom_cpp_options)

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

  add_executable(map_geometry_tests tests/map_geometry_test.cpp)
  target_link_libraries(map_geometry_tests PRIVATE doom_cpp_game doom_cpp_options)
  target_compile_definitions(
    map_geometry_tests
    PRIVATE DOOMCPP_FREEDOOM1_WAD="${CMAKE_SOURCE_DIR}/assets/freedoom1.wad")
  add_test(NAME map_geometry_loads COMMAND map_geometry_tests)

  add_executable(bsp_tests tests/bsp_test.cpp)
  target_link_libraries(bsp_tests PRIVATE doom_cpp_game doom_cpp_options)
  target_compile_definitions(
    bsp_tests PRIVATE DOOMCPP_FREEDOOM1_WAD="${CMAKE_SOURCE_DIR}/assets/freedoom1.wad")
  add_test(NAME bsp_player_start_subsector COMMAND bsp_tests)

  add_executable(renderer_tests tests/renderer_test.cpp)
  target_link_libraries(renderer_tests PRIVATE doom_cpp_game doom_cpp_options)
  target_compile_definitions(
    renderer_tests PRIVATE DOOMCPP_FREEDOOM1_WAD="${CMAKE_SOURCE_DIR}/assets/freedoom1.wad")
  add_test(NAME renderer_frame_nonblank COMMAND renderer_tests)

  add_executable(input_tests tests/input_test.cpp)
  target_link_libraries(input_tests PRIVATE doom_cpp_game doom_cpp_options)
  add_test(NAME input_fixed_tick COMMAND input_tests)

  add_executable(sprite_tests tests/sprite_test.cpp)
  target_link_libraries(sprite_tests PRIVATE doom_cpp_game doom_cpp_options)
  target_compile_definitions(
    sprite_tests PRIVATE DOOMCPP_FREEDOOM1_WAD="${CMAKE_SOURCE_DIR}/assets/freedoom1.wad")
  add_test(NAME sprites_load_and_render COMMAND sprite_tests)
endif()
