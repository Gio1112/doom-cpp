# Parse Doom Map Geometry

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `PLANS.md` at the repository root. It describes Phase 2: decoding the core map geometry lumps from a real IWAD into typed C++ structures that later BSP traversal, collision, rendering, and gameplay systems can use.

## Purpose / Big Picture

After this phase, the engine can load E1M1 from `assets/freedoom1.wad` and turn the raw WAD lumps into typed map data: player and object starts, line segments, wall sides, vertices, and sectors. The observable result is a real integration test that opens the Freedoom IWAD, parses E1M1, and asserts the map has the known real vertex count of 1,196. This is the first step from "archive reader" toward a playable level because renderer and collision code cannot operate on untyped bytes.

## Progress

- [x] (2026-05-02 21:43+01:00) Inspected the real E1M1 lump sequence in `assets/freedoom1.wad`.
- [x] (2026-05-02 21:50+01:00) Implemented public typed map structs under `include/doomcpp/map.hpp`.
- [x] (2026-05-02 21:50+01:00) Implemented E1M1 map loading and lump decoding under `game/map.cpp`.
- [x] (2026-05-02 21:50+01:00) Added an integration test that parses E1M1 and asserts the expected vertex count of 1,196.
- [x] (2026-05-02 21:50+01:00) Wired the map parser into `CMakeLists.txt`.
- [x] (2026-05-02 21:51+01:00) Ran `make check`; clang-format reported style violations in `game/map.cpp`.
- [x] (2026-05-02 21:52+01:00) Applied clang-format to the new map header, implementation, and test files.
- [x] (2026-05-02 21:53+01:00) Reran `make check`; clang-tidy reported one `modernize-use-auto` issue in `game/map.cpp`.
- [x] (2026-05-02 21:54+01:00) Updated the marker index initialization to use `auto` with the explicit cast.
- [x] (2026-05-02 21:56+01:00) Reran `make check`; clang-format, clang-tidy, Ninja build, and CTest all passed with `3/3` tests green.
- [x] (2026-05-02 21:58+01:00) Reran `make check` after the plan update; it passed with `3/3` tests green.
- [ ] Commit Phase 2 with a present-tense milestone message.

## Surprises & Discoveries

- Observation: Freedoom 0.13.0 E1M1 is the first map in the IWAD and starts at lump index 0.
  Evidence: Direct directory inspection showed `E1M1` followed by `THINGS`, `LINEDEFS`, `SIDEDEFS`, `VERTEXES`, `SEGS`, `SSECTORS`, `NODES`, `SECTORS`, `REJECT`, and `BLOCKMAP`.

- Observation: The real E1M1 vertex count for Freedoom 0.13.0 is 1,196.
  Evidence: The `VERTEXES` lump has 4,784 bytes and each vertex record is 4 bytes, so `4784 / 4 = 1196`.

- Observation: The first map parser draft needed clang-format normalization before static analysis and build could run.
  Evidence: `make check` stopped at `clang-format check failed` with diagnostics in `game/map.cpp`.

- Observation: The strict clang-tidy profile applies modernize checks to internal parser helpers.
  Evidence: `make check` rejected an explicit `std::size_t` variable initialized from `static_cast<std::size_t>(...)` and required `auto`.

## Decision Log

- Decision: Parse only THINGS, LINEDEFS, SIDEDEFS, VERTEXES, and SECTORS in Phase 2.
  Rationale: The user explicitly scoped Phase 2 to those lumps. BSP-specific lumps `SEGS`, `SSECTORS`, and `NODES` are intentionally deferred to Phase 3 so the geometry milestone remains independently verifiable.
  Date/Author: 2026-05-02 / Codex

- Decision: Use `std::int16_t` and `std::uint16_t` in public structs to reflect Doom's on-disk field widths.
  Rationale: Doom map records are compact little-endian 16-bit fields. Preserving these widths keeps the parser honest, makes tests map directly to the file format, and avoids accidental widening hiding sign or sentinel behavior.
  Date/Author: 2026-05-02 / Codex

## Outcomes & Retrospective

Phase 2 now parses the real E1M1 THINGS, LINEDEFS, SIDEDEFS, VERTEXES, and SECTORS lumps into typed C++ structures. The map geometry integration test opens `assets/freedoom1.wad`, loads E1M1 through the real WAD reader, and proves the expected 1,196 vertices are present while also exercising the other core geometry lumps. The main lesson from this phase is that keeping map decoding narrowly scoped made it easy to validate against real binary data without pulling BSP or renderer concerns into the same milestone.

## Context and Orientation

Phase 1 added `doomcpp::WadFile`, which owns an IWAD's bytes and parsed lump directory. A Doom map is stored as a marker lump such as `E1M1` followed by a fixed sequence of map data lumps. For Doom and Ultimate Doom maps the marker name is `E#M#`, while Doom II style maps are named `MAP##`. This phase targets `E1M1` in Freedoom Phase 1.

The relevant Doom map record layouts are:

THINGS stores map objects and starts. Each record is 10 bytes: signed 16-bit x position, signed 16-bit y position, signed 16-bit angle in degrees, signed 16-bit type number, and signed 16-bit flags. Type 1 is the player 1 start in classic Doom data.

LINEDEFS stores wall and trigger lines. Each record is 14 bytes: unsigned 16-bit start vertex index, unsigned 16-bit end vertex index, unsigned 16-bit flags, unsigned 16-bit special type, unsigned 16-bit sector tag, signed 16-bit right sidedef index, and signed 16-bit left sidedef index. A sidedef index of -1 means that side is absent, which is common for one-sided walls.

SIDEDEFS stores wall side texture placement and sector reference. Each record is 30 bytes: signed 16-bit x texture offset, signed 16-bit y texture offset, three 8-byte texture names for upper, lower, and middle textures, and signed 16-bit sector index. Texture names are ASCII padded with zero bytes and normalized to uppercase strings in this project.

VERTEXES stores two-dimensional points. Each record is 4 bytes: signed 16-bit x and signed 16-bit y.

SECTORS stores floor and ceiling properties for convex or non-convex areas referenced by sidedefs. Each record is 26 bytes: signed 16-bit floor height, signed 16-bit ceiling height, 8-byte floor flat name, 8-byte ceiling flat name, signed 16-bit light level, signed 16-bit special type, and signed 16-bit tag.

All multi-byte fields are little-endian, meaning the least significant byte appears first in the file.

## Plan of Work

Create `include/doomcpp/map.hpp` with typed structs `Thing`, `Vertex`, `Linedef`, `Sidedef`, `Sector`, and `MapData`. `MapData` should own vectors for each parsed lump and expose them as public fields for now, because this milestone is a load-time data representation and later phases will decide higher-level query APIs.

Create `game/map.cpp` with `MapData load_map(const WadFile& wad, std::string_view map_name)`. The function must find the map marker lump, locate the required lumps before the next map marker, validate that each lump size is an exact multiple of its record size, and decode each field. It must throw `std::runtime_error` with a concrete message when required lumps are missing or malformed.

Extend `CMakeLists.txt` so `doom_cpp_game` includes `game/map.cpp`. Add `tests/map_geometry_test.cpp`, which loads `assets/freedoom1.wad`, calls `load_map(wad, "E1M1")`, and asserts `map.vertices.size() == 1196`. It should also make lightweight sanity assertions that things, linedefs, sidedefs, and sectors are non-empty so every Phase 2 lump is exercised by the test.

## Concrete Steps

From `C:\Users\giorg\Documents\codex\doom-cpp`, run:

    make check

After implementation, CTest should include the bootstrap test, WAD directory test, and map geometry test. The new test must read the real Freedoom IWAD from `assets/freedoom1.wad` and parse real E1M1 data.

## Validation and Acceptance

Phase 2 is accepted only when `make check` exits with status 0 and CTest reports the map geometry test passing. The primary acceptance assertion is that parsing E1M1 from the real Freedoom 0.13.0 IWAD yields exactly 1,196 vertices. This proves the parser found the right map, read the right lump, applied the correct record size, and decoded real binary map data instead of placeholder geometry.

## Idempotence and Recovery

The parser is read-only with respect to assets, so tests can be repeated safely. If CMake state becomes stale, `make check` recreates `build/check`. If the WAD is missing, restore `assets/freedoom1.wad` from the official Freedoom 0.13.0 archive used in Phase 1.

## Artifacts and Notes

The real E1M1 directory excerpt from `assets/freedoom1.wad` is:

    Index Name     Offset  Size
        0 E1M1         12     0
        1 THINGS       12  2920
        2 LINEDEFS   2932 16450
        3 SIDEDEFS  19384 54870
        4 VERTEXES  74256  4784
        5 SEGS      79040 24684
        6 SSECTORS 103724  2728
        7 NODES    106452 19068
        8 SECTORS  125520  4732
        9 REJECT   130252  4141
       10 BLOCKMAP 134396  7528

The record counts derived from those sizes are:

    THINGS: 292 records
    LINEDEFS: 1175 records
    SIDEDEFS: 1829 records
    VERTEXES: 1196 records
    SECTORS: 182 records

## Interfaces and Dependencies

In `include/doomcpp/map.hpp`, define:

    struct Thing { std::int16_t x; std::int16_t y; std::int16_t angle; std::int16_t type; std::int16_t flags; };
    struct Vertex { std::int16_t x; std::int16_t y; };
    struct Linedef { std::uint16_t start_vertex; std::uint16_t end_vertex; std::uint16_t flags; std::uint16_t special_type; std::uint16_t sector_tag; std::int16_t right_sidedef; std::int16_t left_sidedef; };
    struct Sidedef { std::int16_t x_offset; std::int16_t y_offset; std::string upper_texture; std::string lower_texture; std::string middle_texture; std::int16_t sector; };
    struct Sector { std::int16_t floor_height; std::int16_t ceiling_height; std::string floor_texture; std::string ceiling_texture; std::int16_t light_level; std::int16_t special_type; std::int16_t tag; };
    struct MapData { std::vector<Thing> things; std::vector<Linedef> linedefs; std::vector<Sidedef> sidedefs; std::vector<Vertex> vertices; std::vector<Sector> sectors; };
    MapData load_map(const WadFile& wad, std::string_view map_name);

No new external dependency is introduced. This phase depends on `doomcpp::WadFile` from Phase 1 and the existing `assets/freedoom1.wad` asset.

Revision note 2026-05-02 / Codex: Created this plan before adding Phase 2 parser code so map geometry loading is recoverable from a self-contained ExecPlan.

Revision note 2026-05-02 / Codex: Updated progress after adding typed map structs, map lump decoding, CMake wiring, and the real E1M1 geometry test.

Revision note 2026-05-02 / Codex: Recorded the clang-format failure and fix for the Phase 2 parser files.

Revision note 2026-05-02 / Codex: Recorded the clang-tidy modernization finding and fix for the map marker index.

Revision note 2026-05-02 / Codex: Recorded the green Phase 2 `make check` result and outcomes after the real E1M1 geometry test passed.

Revision note 2026-05-02 / Codex: Recorded the green `make check` result after updating this ExecPlan.
