# Add Fixed-Timestep Player Input

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `PLANS.md` at the repository root. It describes Phase 5: making the rendered E1M1 view respond to keyboard and mouse input while preserving Doom's fixed 35 Hz simulation tick.

## Purpose / Big Picture

After this phase, the player can run the game executable, see E1M1, move with WASD, and turn with the mouse. The simulation advances in fixed 1/35-second ticks, matching classic Doom's 35 Hz game logic, while rendering can happen as often as the host loop allows. The observable result is a responsive SDL window and `make check` proving the deterministic movement math without mocking the renderer or WAD pipeline.

## Progress

- [x] (2026-05-02 23:18+01:00) Added public input and player simulation types.
- [x] (2026-05-02 23:18+01:00) Implemented fixed-tick player movement math with forward/back and strafe axes plus mouse-look yaw.
- [x] (2026-05-02 23:18+01:00) Integrated SDL keyboard and relative mouse input into the app loop.
- [x] (2026-05-02 23:18+01:00) Rendered a fresh software frame each display iteration from the updated player view.
- [x] (2026-05-02 23:18+01:00) Added deterministic input tests for 35 Hz movement and mouse yaw.
- [x] (2026-05-02 23:19+01:00) Ran `make check`; clang-format reported style violations in `engine/sdl_app.cpp` and `include/doomcpp/input.hpp`.
- [x] (2026-05-02 23:20+01:00) Applied clang-format to the new input files and SDL app changes.
- [x] (2026-05-02 23:21+01:00) Reran `make check`; clang-tidy reported `run_game` cognitive complexity and pointer arithmetic in SDL keyboard state access.
- [x] (2026-05-02 23:24+01:00) Split event polling, keyboard sampling, and fixed-tick stepping into helpers, and wrapped SDL keyboard state in `std::span`.
- [x] (2026-05-02 23:25+01:00) Reran `make check`; clang-tidy reported cloned quit branches, and the local standard library's `std::span` did not provide `.at()`.
- [x] (2026-05-02 23:27+01:00) Merged quit/escape handling into one condition and added a bounds-checked `key_is_down` helper using `std::span::operator[]`.
- [x] (2026-05-02 23:28+01:00) Reran `make check`; clang-format reported style violations in `engine/sdl_app.cpp`.
- [x] (2026-05-02 23:29+01:00) Applied clang-format to `engine/sdl_app.cpp`.
- [x] (2026-05-02 23:31+01:00) Reran `make check`; clang-format, clang-tidy, Ninja build, and CTest all passed with `6/6` tests green.
- [x] (2026-05-02 23:32+01:00) Manually ran `build/check/doom_cpp.exe`; the SDL input/render loop launched and exited cleanly with status 0 after the bounded loop.
- [x] (2026-05-02 23:34+01:00) Reran `make check` after this plan update; it passed with `6/6` tests green.
- [ ] Commit Phase 5 with a present-tense milestone message.

## Surprises & Discoveries

- Observation: The first fixed-timestep input draft needed clang-format normalization before static analysis and build could run.
  Evidence: `make check` stopped at `clang-format check failed` with diagnostics in `engine/sdl_app.cpp` and `include/doomcpp/input.hpp`.

- Observation: SDL's raw keyboard state pointer triggers the project pointer-arithmetic rule if indexed directly.
  Evidence: `make check` reported `cppcoreguidelines-pro-bounds-pointer-arithmetic` for `keyboard[SDL_SCANCODE_W]` and related keys.

- Observation: This MSYS2 GCC standard library does not expose `std::span::at()`.
  Evidence: `make check` failed with `no member named 'at' in 'std::span<const unsigned char>'`.

- Observation: The automated manual smoke run can verify the SDL input/render loop starts and exits cleanly, but it cannot prove physical WASD/mouse interaction without a human at the window.
  Evidence: `build/check/doom_cpp.exe` returned status 0 after the bounded loop; deterministic movement behavior is covered by `input_fixed_tick`.

## Decision Log

- Decision: Keep movement simulation independent from SDL.
  Rationale: SDL should translate device events into input state, but deterministic movement belongs in engine code that tests can exercise directly. This keeps `make check` meaningful without requiring interactive keyboard or mouse input.
  Date/Author: 2026-05-02 / Codex

- Decision: Use a named `simulation_tick_rate_hz` constant of 35.
  Rationale: The project rules explicitly require a fixed timestep and mention 35 Hz as the Doom-compatible cadence. Naming the value avoids a magic number in movement code and tests.
  Date/Author: 2026-05-02 / Codex

## Outcomes & Retrospective

Phase 5 now has deterministic fixed-timestep player movement at 35 Hz, WASD keyboard state translation, relative mouse yaw input, and per-loop rerendering from the updated player view. The input unit test proves that 35 ticks at 128 map units per second moves the player exactly one second of distance and that mouse yaw applies through the same simulation function used by SDL input. The main lesson from this phase is that keeping SDL input translation thin made the gameplay movement rules easy to test directly.

## Context and Orientation

Phase 4 renders a software frame from a `PlayerView` and presents it through SDL2. The current app renders one static frame from the E1M1 player start and presents it for a bounded smoke-test loop. Phase 5 changes that loop into a small real-time game loop: SDL events update an input state, the simulation consumes accumulated elapsed time in fixed 35 Hz ticks, and rendering uses interpolation-free current player state for now.

A fixed timestep means game logic advances by a constant duration each tick rather than using variable frame time directly. This makes movement deterministic, prevents faster machines from changing game speed, and matches Doom's original 35 ticks per second.

## Plan of Work

Create `include/doomcpp/input.hpp` with `InputState`, `PlayerState`, `MovementConfig`, `simulation_tick_rate_hz`, `fixed_tick_seconds`, and `advance_player(PlayerState, InputState, MovementConfig)`. Use axes for forward and strafe movement so SDL and tests can drive the same API. Mouse look updates yaw in degrees.

Create `engine/input.cpp` implementing movement. Forward movement should use the player's yaw direction, strafe movement should use the perpendicular direction, and yaw should wrap into `[0, 360)` degrees. There is no collision in this phase; Phase 7 owns collision against linedefs.

Update `engine/sdl_app.cpp` to hold a `PlayerState`, poll SDL events, update key state for W/A/S/D, accumulate mouse relative X into yaw input, and process fixed ticks using `std::chrono`. Call `SDL_SetRelativeMouseMode(SDL_TRUE)` while running so mouse motion turns the view. Re-render the software frame every loop from the current player state.

Add `tests/input_test.cpp` with deterministic checks: after 35 forward ticks at 128 map units per second while facing 0 degrees, X increases by about 128 and Y stays stable; after one tick with positive mouse delta, yaw changes by the configured sensitivity. These tests prove the 35 Hz simulation math without relying on interactive input.

## Concrete Steps

From `C:\Users\giorg\Documents\codex\doom-cpp`, run:

    make check

Then manually run:

    .\build\check\doom_cpp.exe

The executable should open the E1M1 window. Press W/S to move forward/back, A/D to strafe, and move the mouse horizontally to turn. The bounded smoke loop may be extended or removed if necessary for manual interaction, but automated tests must not hang.

## Validation and Acceptance

Phase 5 is accepted only when `make check` exits with status 0, deterministic input tests pass, and a manual executable run confirms that WASD and mouse movement alter the rendered view. The app must continue to load and render the real E1M1 WAD data; replacing the renderer with a test scene is not acceptable.

## Idempotence and Recovery

The input implementation is source-only and read-only with respect to assets. `make check` recreates `build/check`. If SDL relative mouse mode behaves unexpectedly on a platform, keep the simulation API intact and adjust only SDL event translation.

## Artifacts and Notes

The named simulation constants should be visible in `include/doomcpp/input.hpp`:

    simulation_tick_rate_hz = 35
    fixed_tick_seconds = 1.0F / 35.0F

## Interfaces and Dependencies

In `include/doomcpp/input.hpp`, define:

    struct InputState { float forward_axis; float strafe_axis; float mouse_delta_x; };
    struct PlayerState { float x; float y; float angle_degrees; };
    struct MovementConfig { float move_units_per_second; float mouse_degrees_per_count; };
    PlayerState advance_player(PlayerState player, InputState input, MovementConfig config);
    PlayerView to_player_view(PlayerState player);

This phase introduces no new third-party dependency beyond SDL2 already added in Phase 4.

Revision note 2026-05-02 / Codex: Created this plan before adding Phase 5 input code so fixed-timestep movement work is recoverable from a self-contained ExecPlan.

Revision note 2026-05-02 / Codex: Updated progress after adding deterministic movement simulation, SDL input integration, per-frame rerendering, and input tests.

Revision note 2026-05-02 / Codex: Recorded the clang-format failure and fix for the Phase 5 input files.

Revision note 2026-05-02 / Codex: Recorded SDL app loop complexity and keyboard pointer findings, plus the helper extraction and span-based access fixes.

Revision note 2026-05-02 / Codex: Recorded the branch-clone and span `.at()` findings and the event/key helper fixes.

Revision note 2026-05-02 / Codex: Recorded the formatting follow-up after the SDL event/key helper changes.

Revision note 2026-05-02 / Codex: Recorded the green Phase 5 `make check` result and outcomes after deterministic input tests passed.

Revision note 2026-05-02 / Codex: Recorded the SDL input/render loop smoke run and the limitation of noninteractive input verification.

Revision note 2026-05-02 / Codex: Recorded the green `make check` result after updating this ExecPlan.
