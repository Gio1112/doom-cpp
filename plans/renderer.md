# Render E1M1 in a Software Window

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `PLANS.md` at the repository root. It describes Phase 4: opening an SDL2 window and rendering a recognizable first view of the real E1M1 map using a CPU-side column-oriented software renderer.

## Purpose / Big Picture

After this phase, running the game executable opens a real window and shows the player standing in the loaded E1M1 map. The renderer is intentionally simple but data-driven: it uses vertices and linedefs parsed from `assets/freedoom1.wad`, projects visible wall segments from the player 1 start, clips them to the viewport, and draws vertical wall columns into a software framebuffer. The observable result is a window with a recognizable corridor-like view derived from real WAD geometry, plus `make check` remaining green.

## Progress

- [x] (2026-05-02 22:22+01:00) Installed MSYS2 SDL2 development package `mingw-w64-ucrt-x86_64-SDL2`.
- [x] (2026-05-02 22:32+01:00) Added renderer-facing constants and player camera types.
- [x] (2026-05-02 22:32+01:00) Implemented a software framebuffer renderer that projects real E1M1 linedefs into vertical columns.
- [x] (2026-05-02 22:32+01:00) Added SDL2 platform code that opens a window, uploads the software framebuffer, and runs a short render loop.
- [x] (2026-05-02 22:32+01:00) Updated `doom_cpp` to load `assets/freedoom1.wad`, parse E1M1, find player 1 start, and render the map.
- [x] (2026-05-02 22:32+01:00) Added renderer tests for nonblank data-driven frame output without requiring an interactive window.
- [x] (2026-05-02 22:33+01:00) Ran `make check`; clang-format reported style violations across the new renderer, SDL app, and renderer test files.
- [x] (2026-05-02 22:34+01:00) Applied clang-format to the new renderer and SDL files.
- [x] (2026-05-02 22:36+01:00) Reran `make check`; clang-tidy reported renderer helper issues including std::numbers, short identifiers, swappable parameters, signed comparisons, narrowing, and auto-with-cast.
- [x] (2026-05-02 22:43+01:00) Refined renderer helpers with named structs, `std::numbers::pi_v`, clearer variable names, `std::cmp_greater_equal`, explicit float bounds, and `auto` for cast initializers.
- [x] (2026-05-02 22:44+01:00) Reran `make check`; clang-format reported style violations in the refactored `engine/render.cpp`.
- [x] (2026-05-02 22:45+01:00) Applied clang-format to `engine/render.cpp`.
- [x] (2026-05-02 22:46+01:00) Reran `make check`; clang-tidy reported missing `std::cmp_greater_equal` include support and suspicious custom clamp calls.
- [x] (2026-05-02 22:48+01:00) Switched the include to `<utility>` and replaced the custom clamp helper with `std::clamp`.
- [x] (2026-05-02 22:49+01:00) Reran `make check`; clang-format reported style violations after the clamp refactor.
- [x] (2026-05-02 22:50+01:00) Applied clang-format to `engine/render.cpp`.
- [x] (2026-05-02 22:51+01:00) Reran `make check`; clang-tidy reported SDL RAII wrapper special-member and default-initializer issues.
- [x] (2026-05-02 22:52+01:00) Made SDL RAII wrappers explicitly non-copyable and non-movable, and moved pointer null initialization to default member initializers.
- [x] (2026-05-02 22:54+01:00) Reran `make check`; build and static analysis passed, but CTest failed because the old bootstrap test expected the Phase 0 boot message and the executable could not find `assets/freedoom1.wad` from `build/check`.
- [x] (2026-05-02 22:57+01:00) Added WAD path resolution relative to both the current working directory and SDL executable base path, and updated the executable smoke test to assert successful app execution.
- [x] (2026-05-02 22:58+01:00) Reran `make check`; clang-tidy reported `performance-no-automatic-move` for returning a const local path.
- [x] (2026-05-02 22:59+01:00) Removed unnecessary const from local path objects returned by the WAD resolver.
- [x] (2026-05-02 23:01+01:00) Reran `make check`; clang-format, clang-tidy, Ninja build, and CTest all passed with `5/5` tests green.
- [x] (2026-05-02 23:02+01:00) Manually ran `build/check/doom_cpp.exe`; it launched the SDL path and exited cleanly with status 0 after the bounded loop.
- [x] (2026-05-02 23:04+01:00) Added renderer test artifact export to `build/check/e1m1-render.ppm` for observable software-frame evidence.
- [x] (2026-05-02 23:06+01:00) Reran `make check`; renderer test failed because the artifact path resolved to a nonexistent nested `build/check` path under the CTest working directory.
- [x] (2026-05-02 23:07+01:00) Changed the renderer artifact path to `e1m1-render.ppm` relative to CTest's build-directory working directory.
- [x] (2026-05-02 23:09+01:00) Reran `make check` after fixing the renderer artifact path; it passed with `5/5` tests green.
- [x] (2026-05-02 23:10+01:00) Verified `build/check/e1m1-render.ppm` exists, is 192,015 bytes, and has the binary PPM header `P6 320 200 255`.
- [x] (2026-05-02 23:12+01:00) Reran `make check` after this plan update; it passed with `5/5` tests green.
- [ ] Commit Phase 4 with a present-tense milestone message.

## Surprises & Discoveries

- Observation: SDL2 was not present before Phase 4.
  Evidence: no SDL CMake package appeared under `C:\msys64\ucrt64\lib\cmake`, and the package query required installing `mingw-w64-ucrt-x86_64-SDL2`.

- Observation: The first renderer implementation draft needed clang-format normalization before static analysis and build could run.
  Evidence: `make check` stopped at `clang-format check failed` with diagnostics in `engine/render.cpp`, `engine/sdl_app.cpp`, and `tests/renderer_test.cpp`.

- Observation: Renderer helper APIs with adjacent scalar parameters are fragile under the strict clang-tidy profile.
  Evidence: `make check` reported `bugprone-easily-swappable-parameters` for `draw_column` and `draw_wall_segment`, which was resolved by introducing `ColumnSpan` and `WallDrawStyle`.

- Observation: Once `doom_cpp` became the real SDL app, the Phase 0 bootstrap test had to change from checking a boot string to checking successful executable startup.
  Evidence: CTest failed `bootstrap_runs` with `doom-cpp failed: failed to open WAD file: assets/freedoom1.wad` while the WAD, map, BSP, and renderer tests passed.

- Observation: Terminal execution can prove the SDL executable starts and exits cleanly, but it does not capture the rendered pixels for later inspection.
  Evidence: manual `build/check/doom_cpp.exe` returned exit status 0 with no stderr output; the renderer test now writes `build/check/e1m1-render.ppm` from the same software frame used for SDL upload.

- Observation: CTest executes `renderer_tests` from `build/check`, so artifact paths should be relative to that directory.
  Evidence: writing `build/check/e1m1-render.ppm` failed because it attempted to create a nested path under the test working directory; writing `e1m1-render.ppm` creates `build/check/e1m1-render.ppm`.

## Decision Log

- Decision: Use SDL2 only for the window, event polling, and texture presentation; keep wall rendering CPU-side.
  Rationale: The project rules require a software renderer first. SDL2 is already the allowed dependency for windowing and input, while the actual pixel generation should remain in engine code where tests can exercise it without a GPU.
  Date/Author: 2026-05-02 / Codex

- Decision: Render untextured solid-color wall columns in Phase 4.
  Rationale: The user asked for the column-based renderer and a recognizable corridor before sprite/texture milestones. Texture compositing requires patch and texture lump parsing that is not scheduled until later; solid wall columns still prove real WAD geometry, projection, clipping, and SDL presentation without placeholder geometry.
  Date/Author: 2026-05-02 / Codex

## Outcomes & Retrospective

Phase 4 now opens an SDL2 window from `doom_cpp`, loads the real Freedoom E1M1 map, renders a CPU-generated wall-column frame from the real player start, and presents that frame through a streaming SDL texture. The noninteractive renderer test proves the same real-map frame contains ceiling, floor, and wall pixels, and writes `build/check/e1m1-render.ppm` for inspection. The main lesson from this phase is that executable smoke tests need asset resolution that works from both repository-root manual runs and build-directory CTest runs.

## Context and Orientation

Phase 1 loads a real Freedoom IWAD. Phase 2 decodes E1M1 map geometry into typed structures. Phase 3 decodes BSP records and can locate the subsector containing the player. Phase 4 turns that loaded data into pixels.

A software framebuffer is a CPU-owned array of pixels. This phase will store 32-bit RGBA pixels in a vector, draw into that vector, and upload it to an SDL texture for presentation. A column renderer draws vertical screen-space strips rather than arbitrary filled triangles. Classic Doom renders textured vertical columns after BSP ordering; this phase implements the first visible slice of that path by projecting wall segments and drawing their covered screen columns.

The camera starts at the real THINGS type 1 player start from E1M1. The Phase 3 inspection found `x=-416`, `y=256`, and angle `0`. Doom angles use degrees in THINGS, where 0 faces east along positive X.

## Plan of Work

Create `include/doomcpp/render.hpp` with `RenderConfig`, `PlayerView`, `SoftwareFrame`, and `render_map_frame(const MapData&, PlayerView, RenderConfig)`. Use named constants for width, height, field of view, and projection scale. The framebuffer should own its pixels and expose a span for tests and SDL upload.

Create `engine/render.cpp` to implement the CPU renderer. It should clear the top half to a ceiling color and bottom half to a floor color, then iterate over real linedefs. For each linedef with valid vertex indices, transform both endpoints into camera space, clip against a small near plane, reject segments outside the horizontal field of view, project endpoints to screen X coordinates, and draw vertical columns between floor and ceiling extents. Use the linedef index to vary wall brightness slightly so adjacent walls are visually distinguishable. This phase does not allocate inside the per-column inner loop; the framebuffer allocation happens before drawing.

Create `engine/sdl_app.cpp` and `include/doomcpp/app.hpp` with `int run_game(const std::filesystem::path& wad_path)`. This function should initialize SDL, create a window, create a streaming texture, load the WAD, parse E1M1, build a `PlayerView` from THINGS type 1, render a frame, upload it, and keep the window open for a short bounded loop in automated environments or until quit. A bounded loop prevents manual smoke tests from hanging forever in this development phase.

Update `engine/main.cpp` to call `run_game("assets/freedoom1.wad")` and report exceptions to stderr. The working directory for normal execution is the repository root.

Add `tests/renderer_test.cpp` that loads the real WAD, parses E1M1, renders a software frame without opening a window, and asserts the frame contains ceiling pixels, floor pixels, and at least one wall-colored pixel. This test proves data-driven frame output under `make check`.

## Concrete Steps

From `C:\Users\giorg\Documents\codex\doom-cpp`, run:

    make check

Then manually run:

    .\build\check\doom_cpp.exe

The executable should open an SDL window titled `doom-cpp`, display a rendered E1M1 view from the player start, and close after the bounded smoke-test loop if no input is provided.

## Validation and Acceptance

Phase 4 is accepted only when `make check` exits with status 0, the renderer test proves a nonblank real-map frame, and the executable opens an SDL window showing E1M1-derived geometry. The renderer must use real `MapData` from `assets/freedoom1.wad`; hardcoded test corridors or placeholder wall lists are not acceptable.

## Idempotence and Recovery

The renderer is read-only with respect to assets. The SDL2 package can be reinstalled with MSYS2 `pacman -S --needed mingw-w64-ucrt-x86_64-SDL2`. Generated build outputs remain under ignored `build/`. If the executable cannot find `assets/freedoom1.wad`, run it from the repository root or restore the asset from the official Freedoom 0.13.0 archive.

## Artifacts and Notes

SDL2 installation transcript:

    installing mingw-w64-ucrt-x86_64-vulkan-loader...
    installing mingw-w64-ucrt-x86_64-SDL2...

The first rendered view should be derived from:

    Player start: x=-416, y=256, angle=0
    Map source: assets/freedoom1.wad, E1M1

## Interfaces and Dependencies

In `include/doomcpp/render.hpp`, define:

    struct RenderConfig { std::uint16_t width; std::uint16_t height; float horizontal_fov_degrees; };
    struct PlayerView { float x; float y; float angle_degrees; };
    class SoftwareFrame { ... pixels() ... width() ... height() ... };
    SoftwareFrame render_map_frame(const MapData& map, PlayerView view, RenderConfig config);

In `include/doomcpp/app.hpp`, define:

    int run_game(const std::filesystem::path& wad_path);

This phase adds SDL2 as an external dependency, justified here because `AGENTS.md` explicitly allows SDL2 for windowing and input. CMake must declare it with `find_package(SDL2 REQUIRED)` and link only the executable or SDL platform target that needs it.

Revision note 2026-05-02 / Codex: Created this plan before adding Phase 4 renderer code so the SDL/software rendering work is recoverable from a self-contained ExecPlan.

Revision note 2026-05-02 / Codex: Updated progress after adding the software renderer, SDL presentation path, executable wiring, and renderer integration test.

Revision note 2026-05-02 / Codex: Recorded the clang-format failure and fix for the Phase 4 renderer files.

Revision note 2026-05-02 / Codex: Recorded the first clang-tidy renderer findings and the helper API refinements made in response.

Revision note 2026-05-02 / Codex: Recorded the formatting follow-up after the renderer helper refactor.

Revision note 2026-05-02 / Codex: Recorded the second renderer clang-tidy failure and the clamp/include fixes.

Revision note 2026-05-02 / Codex: Recorded the formatting follow-up after the clamp refactor.

Revision note 2026-05-02 / Codex: Recorded the SDL RAII clang-tidy findings and ownership fixes.

Revision note 2026-05-02 / Codex: Recorded the executable smoke-test failure after `doom_cpp` became the SDL app and the asset path/test updates made in response.

Revision note 2026-05-02 / Codex: Recorded the path resolver automatic-move clang-tidy finding and fix.

Revision note 2026-05-02 / Codex: Recorded the green Phase 4 `make check` result after the executable smoke test and renderer test passed.

Revision note 2026-05-02 / Codex: Recorded the manual SDL executable smoke run and added a persistent renderer frame artifact for inspection.

Revision note 2026-05-02 / Codex: Recorded the renderer artifact path failure and changed the path to match the CTest working directory.

Revision note 2026-05-02 / Codex: Recorded the green Phase 4 `make check` result, renderer artifact evidence, and outcomes.

Revision note 2026-05-02 / Codex: Recorded the green `make check` result after updating this ExecPlan.
