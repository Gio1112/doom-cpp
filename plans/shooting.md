# Add Shooting and Linedef Collision

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `PLANS.md` at the repository root. It describes Phase 7: adding left-click hitscan shooting, simple enemy damage state transitions, and blocking movement against solid linedefs.

## Purpose / Big Picture

After this phase, the player can move around E1M1, collide with one-sided wall linedefs, and shoot enemies with left click. A hitscan attack is an instant ray-style weapon trace: it checks which enemy is under the crosshair and applies damage immediately. The observable result is `make check` proving collision and shooting against real E1M1 geometry and THINGS, plus the SDL game loop accepting mouse fire input.

## Progress

- [x] (2026-05-03 00:09+01:00) Added gameplay enemy, hit result, and collision public types.
- [x] (2026-05-03 00:09+01:00) Built enemy states from real E1M1 THINGS.
- [x] (2026-05-03 00:09+01:00) Implemented hitscan attack that damages the closest enemy in front of the player.
- [x] (2026-05-03 00:09+01:00) Implemented one-sided linedef collision for player movement.
- [x] (2026-05-03 00:09+01:00) Integrated left-click shooting and collision into the SDL loop.
- [x] (2026-05-03 00:09+01:00) Added real-map tests for collision and shooting state transitions.
- [x] (2026-05-03 00:10+01:00) Ran `make check`; clang-tidy reported adjacent swappable bool references in the SDL event polling helper.
- [x] (2026-05-03 00:11+01:00) Replaced the adjacent bool references with an `EventResult` value struct.
- [x] (2026-05-03 23:27+01:00) Reran `make check`; clang-tidy reported short cross-product parameter names, swappable segment endpoint scalars, and explicit float cast initializers in `game/gameplay.cpp`.
- [x] (2026-05-03 23:29+01:00) Replaced segment scalar parameters with a `Point2` helper and kept cast-initialized values inside aggregate initializers.
- [x] (2026-05-02 23:35+01:00) Reran `make check`; build, clang-format, clang-tidy, and all 8 CTest tests passed.
- [x] (2026-05-02 23:36+01:00) Manually ran `.\build\check\doom_cpp.exe` with `DOOMCPP_SMOKE_TEST=1`; SDL opened, rendered, and exited cleanly.
- [x] (2026-05-02 23:37+01:00) Moved the automatic smoke-test exit behind `DOOMCPP_SMOKE_TEST` so normal player launches stay open until Esc or window close.
- [x] (2026-05-02 23:41+01:00) Ran final `make check` after the ExecPlan update; build, lint, and all 8 tests passed.
- [x] (2026-05-02 23:47+01:00) Committed Phase 7 as `gameplay: add hitscan shooting and collision`.

## Surprises & Discoveries

- Observation: SDL event polling results need a named aggregate rather than adjacent bool references under the strict clang-tidy profile.
  Evidence: `make check` reported `bugprone-easily-swappable-parameters` for `poll_events(InputState&, bool&, bool&)`.

- Observation: Geometry helpers should pass named point aggregates instead of loose coordinate scalars.
  Evidence: `make check` reported `bugprone-easily-swappable-parameters` and identifier-length issues for `segments_intersect` and `cross` in `game/gameplay.cpp`.

- Observation: The game executable must remain interactive for normal play, while automated tests still need a bounded SDL run.
  Evidence: The bootstrap test launches the real executable, and the user also needs `.\build\check\doom_cpp.exe` to stay open when they play manually.

## Decision Log

- Decision: Model pain and death as explicit enemy state values rather than animations.
  Rationale: Phase 7 requires enemies to take damage and play pain/death states. Full sprite animation sequencing needs a broader Doom state table, but explicit `alive`, `pain`, and `dead` gameplay states provide real state transitions now and can later drive animation frames.
  Date/Author: 2026-05-03 / Codex

- Decision: Treat one-sided linedefs as blocking collision geometry.
  Rationale: A linedef with no left sidedef is a solid wall in the map data already parsed by Phase 2. This gives real E1M1 collision without inventing placeholder blockers.
  Date/Author: 2026-05-03 / Codex

- Decision: Gate the automatic three-second SDL smoke-test exit on `DOOMCPP_SMOKE_TEST`.
  Rationale: CTest needs the executable to terminate unattended, but plug-and-play manual runs must remain interactive until the player presses Esc or closes the window.
  Date/Author: 2026-05-02 / Codex

## Outcomes & Retrospective

Phase 7 adds real gameplay state on top of the loaded E1M1 data. Enemy actors are built from THINGS, left-click fires a deterministic hitscan against living enemy states, enemies transition to pain or death after taking damage, and player movement is blocked by one-sided linedefs from the WAD. The SDL loop now accepts fire input and applies collision on the fixed 35 Hz tick path.

Validation result: `make check` passed with build verification, clang-format, clang-tidy, and 8/8 CTest tests green. The manual smoke run opened the SDL executable against E1M1 with the bounded smoke-test environment flag and exited with status 0. The remaining limitation is animation fidelity: pain and death are implemented as gameplay states ready for sprite-state presentation, not yet as a full Doom state-table animation system.

## Context and Orientation

The current game loop moves the player with fixed 35 Hz input but does not prevent walking through walls. E1M1 map geometry includes linedefs with right and left sidedef indices. A left sidedef of -1 means the line is one-sided and should block the player. E1M1 THINGS include enemies such as former humans. The engine already maps some enemy thing types to sprites in Phase 6.

Hitscan shooting in this phase is intentionally gameplay-level and deterministic. It projects each living enemy into the player's forward direction, rejects enemies outside a small aim cone, chooses the closest valid target, subtracts damage, and changes the enemy state to pain or dead.

## Plan of Work

Create `include/doomcpp/gameplay.hpp` with `EnemyStateKind`, `EnemyState`, `ShotResult`, `build_enemies`, `fire_hitscan`, and `collide_player_move`. Implement these in `game/gameplay.cpp`.

Update `engine/input.cpp` movement support only if needed, but keep the fixed tick API intact. Integrate collision in `engine/sdl_app.cpp` by proposing the next player position through `advance_player`, then passing the old and proposed positions through `collide_player_move`. Integrate left mouse button as a fire request that calls `fire_hitscan` once per click.

Add `tests/gameplay_test.cpp`. It should load real E1M1, build enemies from THINGS, place a synthetic player facing the closest enemy for a deterministic shot, assert that the enemy transitions to pain or dead and loses health, and assert that trying to cross a known one-sided linedef is blocked.

## Concrete Steps

From `C:\Users\giorg\Documents\codex\doom-cpp`, run:

    make check

Then manually run:

    .\build\check\doom_cpp.exe

The executable should open E1M1; left click should fire the hitscan logic, and movement should be blocked by solid walls.

## Validation and Acceptance

Phase 7 is accepted only when `make check` exits with status 0, gameplay tests prove shooting and collision against real E1M1 data, and the SDL executable starts cleanly with the new gameplay loop. Placeholder enemies or placeholder walls are not acceptable.

## Idempotence and Recovery

The gameplay systems are deterministic and source-only. Tests read `assets/freedoom1.wad` but do not modify it. Generated outputs remain under ignored `build/`.

## Artifacts and Notes

Enemy thing types initially supported for hitscan targets are 9, 3004, and 3001, matching common Doom enemy definitions and the sprite table seeded in Phase 6.

## Interfaces and Dependencies

In `include/doomcpp/gameplay.hpp`, define:

    enum class EnemyStateKind : std::uint8_t { alive, pain, dead };
    struct EnemyState { float x; float y; std::int16_t type; int health; EnemyStateKind state; };
    struct ShotResult { bool hit; std::size_t enemy_index; int damage; EnemyStateKind resulting_state; };
    std::vector<EnemyState> build_enemies(const MapData& map);
    std::optional<ShotResult> fire_hitscan(std::span<EnemyState> enemies, PlayerView view);
    PlayerState collide_player_move(const MapData& map, PlayerState previous, PlayerState proposed, float radius);

This phase introduces no new external dependencies.

Revision note 2026-05-03 / Codex: Created this plan before adding Phase 7 gameplay code so shooting and collision work is recoverable from a self-contained ExecPlan.

Revision note 2026-05-03 / Codex: Updated progress after adding gameplay shooting/collision, SDL integration, tests, and the first clang-tidy fix.

Revision note 2026-05-03 / Codex: Recorded clang-tidy geometry helper findings and the `Point2` refactor.
