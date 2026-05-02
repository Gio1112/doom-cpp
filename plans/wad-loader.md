# Load the IWAD Directory

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `PLANS.md` at the repository root. It describes Phase 1: loading a real Doom-family IWAD file from `assets/`, parsing its directory, exposing lump names and byte ranges, and proving the behavior with an integration test against the real WAD.

## Purpose / Big Picture

After this phase, the engine can open a real `freedoom1.wad` or `DOOM1.WAD` file and enumerate the lumps inside it. A lump is a named byte range inside a WAD archive; later systems use lumps to find maps, textures, sprites, palettes, sounds, and other game data. The observable result is that `make check` runs a test against the real file in `assets/` and confirms the first lump name from the WAD directory exactly matches the known asset.

## Progress

- [x] (2026-05-02 21:24+01:00) Confirmed `assets/` did not contain any `.wad` file before Phase 1.
- [x] (2026-05-02 21:25+01:00) Verified the official Freedoom GitHub releases page marks `Freedoom 0.13.0` as the latest release.
- [x] (2026-05-02 21:28+01:00) Acquired the official Freedoom 0.13.0 archive and extracted `freedoom1.wad` into `assets/`.
- [x] (2026-05-02 21:29+01:00) Inspected `assets/freedoom1.wad`; it is an IWAD with 3,163 lumps and first lump `E1M1`.
- [x] (2026-05-02 21:34+01:00) Implemented the public WAD reader interface under `include/doomcpp/wad.hpp`.
- [x] (2026-05-02 21:34+01:00) Implemented binary WAD parsing under `game/wad.cpp`.
- [x] (2026-05-02 21:34+01:00) Added a real integration test that opens `assets/freedoom1.wad` and asserts the first lump name.
- [x] (2026-05-02 21:34+01:00) Wired the new library and test into `CMakeLists.txt`.
- [x] (2026-05-02 21:35+01:00) Ran `make check`; clang-format reported style violations in `game/wad.cpp`.
- [x] (2026-05-02 21:36+01:00) Applied clang-format to the new WAD header, implementation, and test files.
- [x] (2026-05-02 21:37+01:00) Reran `make check`; clang-tidy reported enum-size, easily-swappable parameter, and pointer-arithmetic issues.
- [x] (2026-05-02 21:38+01:00) Changed `WadType` to use `std::uint8_t`, introduced a named `ByteRange`, and replaced pointer arithmetic with `std::span::subspan`.
- [x] (2026-05-02 21:40+01:00) Reran `make check`; clang-format, clang-tidy, Ninja build, and CTest all passed with `2/2` tests green.
- [x] (2026-05-02 21:41+01:00) Reran `make check` after the plan update; it passed with `2/2` tests green.
- [ ] Commit Phase 1 with a present-tense milestone message.

## Surprises & Discoveries

- Observation: The repository did not yet have an `assets/` directory or WAD file.
  Evidence: recursive file searches for `assets` and `*.wad` returned no files.

- Observation: Freedoom 0.13.0 Phase 1 starts with an `E1M1` marker lump.
  Evidence: Direct header inspection of `assets/freedoom1.wad` reported `Id: IWAD`, `LumpCount: 3163`, `DirectoryOffset: 28744468`, `FirstOffset: 12`, `FirstSize: 0`, and `FirstName: E1M1`.

- Observation: The first WAD parser draft needed clang-format normalization before static analysis and build could run.
  Evidence: `make check` stopped at `clang-format check failed` with diagnostics in `game/wad.cpp`.

- Observation: The strict clang-tidy profile catches API shapes that could become maintenance hazards even in private helpers.
  Evidence: `make check` rejected adjacent `std::uint32_t offset, size` helper parameters and raw pointer arithmetic in `WadFile::lump_data`.

## Decision Log

- Decision: Use `freedoom1.wad` from Freedoom 0.13.0 as the Phase 1 IWAD.
  Rationale: The user allowed either `DOOM1.WAD` or `freedoom1.wad`; Freedoom is freely redistributable and avoids requiring the user to supply proprietary game data. The official GitHub release page identifies 0.13.0 as latest as of 2026-05-02.
  Date/Author: 2026-05-02 / Codex

- Decision: Put the parser implementation in `game/wad.cpp` while exposing the interface in `include/doomcpp/wad.hpp`.
  Rationale: WAD files are Doom game data archives, and the public API belongs under `include/` so engine, game, and tests can share it without reaching into implementation internals.
  Date/Author: 2026-05-02 / Codex

## Outcomes & Retrospective

Phase 1 now loads a real Doom-family IWAD from `assets/freedoom1.wad`, validates the header and directory bounds, exposes normalized lump metadata, supports case-insensitive lump lookup, and returns raw lump byte spans. The integration test uses the real Freedoom 0.13.0 Phase 1 file and proves the first lump is the expected `E1M1` marker. The main lesson from this phase is that strict static analysis is useful even this early: it pushed the parser away from ambiguous offset/size parameter pairs and raw pointer arithmetic before those habits could spread.

## Context and Orientation

Phase 0 created a CMake project with `doom_cpp` as the runnable executable, `bootstrap_tests` as an integration test, and `make check` as the required quality gate. The gate runs clang-format, configures a Ninja build, runs clang-tidy with warnings as errors, builds all targets, and runs CTest.

A WAD file is the archive format used by Doom. The name means "Where's All the Data." A WAD starts with a 12-byte header. Bytes 0 through 3 are an ASCII identification string such as `IWAD` for a complete game data file or `PWAD` for a patch/mod file. Bytes 4 through 7 are a little-endian signed 32-bit integer giving the number of lumps. Bytes 8 through 11 are a little-endian signed 32-bit integer giving the byte offset of the directory. The directory has one 16-byte entry per lump. Each entry stores a little-endian signed 32-bit file position, a little-endian signed 32-bit size, and an 8-byte ASCII name padded with zero bytes. Lump names are case-insensitive in the original engine, but this project will normalize names to uppercase ASCII strings with trailing zero padding removed.

The Phase 1 reader does not decode lump contents beyond returning raw bytes on request. It only validates archive boundaries, parses the directory, exposes lump metadata, and supports finding lumps by normalized name. Map-specific interpretation begins in Phase 2.

## Plan of Work

Create `assets/` and place `freedoom1.wad` there by downloading the official Freedoom 0.13.0 release archive and extracting the Phase 1 WAD. The asset is needed for plug-and-play tests and future rendering milestones.

Create `include/doomcpp/wad.hpp` with `WadType`, `Lump`, and `WadFile`. `WadFile` should own the WAD bytes and lump metadata after loading. Public methods should include `type()`, `lumps()`, `find_lump(std::string_view)`, and `lump_data(const Lump&)`. Contracts must state preconditions, postconditions, and ownership.

Create `game/wad.cpp` to implement little-endian parsing and input validation. The parser must reject files shorter than the header, unknown identifiers, negative lump counts, negative directory offsets, directory ranges outside the file, negative lump offsets or sizes, and lump byte ranges outside the file. It must use `std::filesystem::path` for file paths and standard containers for load-time storage. Per-frame allocation is not relevant in Phase 1 because WAD parsing occurs at startup/load time.

Extend `CMakeLists.txt` with a `doom_cpp_game` library containing `game/wad.cpp`, linked by tests and later by the executable. Add a new test executable `wad_tests` that opens `assets/freedoom1.wad`, checks that it is an IWAD, checks it has at least one lump, and asserts the first lump name exactly matches the real file.

## Concrete Steps

From `C:\Users\giorg\Documents\codex\doom-cpp`, run:

    make check

Before the parser exists this command still runs the Phase 0 gate. After implementation it must build the WAD library and run both the bootstrap integration test and the WAD integration test.

To smoke-test the asset manually after acquisition, inspect that this file exists:

    assets/freedoom1.wad

The WAD test itself is the authoritative check because it parses the real binary directory rather than trusting the filename.

## Validation and Acceptance

Phase 1 is accepted only when `make check` exits with status 0 and CTest reports both the existing bootstrap test and the new WAD test passing. The WAD test must read the real `assets/freedoom1.wad` file from disk, not a mock or a synthetic in-memory archive. The assertion for the first lump name must be based on the actual Freedoom 0.13.0 file placed in `assets/`.

## Idempotence and Recovery

Downloading and extracting the Freedoom archive is safe to repeat if it writes the same `assets/freedoom1.wad` file. The generated `build/` directory remains ignored and may be deleted. If the downloaded archive is corrupt, delete the incomplete archive or extracted file and repeat the acquisition from the official release source.

## Artifacts and Notes

The official Freedoom GitHub releases page showed:

    Freedoom 0.13.0
    Latest

The initial local asset scan found no WAD files:

    Get-ChildItem -Force -Recurse -File assets -ErrorAction SilentlyContinue
    Get-ChildItem -Recurse -Filter '*.wad' -ErrorAction SilentlyContinue

Both commands produced no file output.

## Interfaces and Dependencies

In `include/doomcpp/wad.hpp`, define:

    enum class WadType { iwad, pwad };

    struct Lump {
        std::string name;
        std::uint32_t offset;
        std::uint32_t size;
    };

    class WadFile {
    public:
        static WadFile load_from_file(const std::filesystem::path& path);
        [[nodiscard]] WadType type() const noexcept;
        [[nodiscard]] std::span<const Lump> lumps() const noexcept;
        [[nodiscard]] const Lump* find_lump(std::string_view name) const noexcept;
        [[nodiscard]] std::span<const std::byte> lump_data(const Lump& lump) const;
    };

No new third-party C++ dependency is introduced in Phase 1. The only asset dependency is the freely redistributable Freedoom 0.13.0 `freedoom1.wad` file placed under `assets/`.

Revision note 2026-05-02 / Codex: Created this plan before adding Phase 1 parser code so WAD loading is recoverable from a self-contained ExecPlan.

Revision note 2026-05-02 / Codex: Updated progress and discoveries after acquiring the Freedoom IWAD and implementing the first WAD directory reader.

Revision note 2026-05-02 / Codex: Recorded the clang-format failure and fix for the Phase 1 parser files.

Revision note 2026-05-02 / Codex: Recorded clang-tidy findings and the parser API refinements made in response.

Revision note 2026-05-02 / Codex: Recorded the green Phase 1 `make check` result and outcomes after the real WAD integration test passed.

Revision note 2026-05-02 / Codex: Recorded the green `make check` result after updating this ExecPlan.
