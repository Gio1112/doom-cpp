# Render Things as Billboards

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `PLANS.md` at the repository root. It describes Phase 6: using E1M1 THINGS and WAD sprite patches to render pickups and enemies as billboarded sprites in the software renderer.

## Purpose / Big Picture

After this phase, the E1M1 view contains not only wall columns but also visible objects from the real map: pickups and enemies are loaded from THINGS, matched to sprite patches in the WAD, projected into camera space, depth-sorted, and drawn as vertical billboard rectangles. The observable result is `make check` proving that sprite patches are found and decoded from the real IWAD, plus the SDL window showing map objects in the rendered scene.

## Progress

- [x] (2026-05-02 23:37+01:00) Inspected the IWAD sprite marker range and E1M1 thing type counts.
- [x] (2026-05-02 23:47+01:00) Added sprite patch and thing-sprite public types.
- [x] (2026-05-02 23:47+01:00) Implemented Doom patch header decoding and sprite lump lookup.
- [x] (2026-05-02 23:47+01:00) Mapped a minimal real set of Doom thing types to sprite prefixes for E1M1 pickups/enemies.
- [x] (2026-05-02 23:47+01:00) Extended the software renderer to draw depth-sorted billboards after wall columns.
- [x] (2026-05-02 23:47+01:00) Added real IWAD tests proving sprite patches are loaded and rendered into the frame.
- [x] (2026-05-02 23:50+01:00) Ran `make check`; clang-tidy reported a qualified-auto issue in `game/sprites.cpp`.
- [x] (2026-05-02 23:51+01:00) Updated the thing definition lookup to use a qualified pointer auto declaration.
- [x] (2026-05-02 23:53+01:00) Reran `make check`; the command timed out before returning output and left `build/check` locked for immediate cleanup.
- [x] (2026-05-02 23:55+01:00) Removed the generated `build/check` directory after confirming no build/test processes were still running.
- [x] (2026-05-02 23:58+01:00) Reran `make check` with a longer timeout; clang-format, clang-tidy, Ninja build, and CTest all passed with `7/7` tests green.
- [x] (2026-05-02 23:59+01:00) Manually ran `build/check/doom_cpp.exe`; the sprite-enabled SDL render path launched and exited cleanly with status 0 after the bounded loop.
- [x] (2026-05-03 00:02+01:00) Reran `make check` after this plan update; it passed with `7/7` tests green.
- [ ] Commit Phase 6 with a present-tense milestone message.

## Surprises & Discoveries

- Observation: Freedoom 0.13.0 stores sprites between `S_START` and `S_END`.
  Evidence: direct WAD directory inspection found `S_START` at lump index 1005 and `S_END` at lump index 1859.

- Observation: E1M1 contains real enemies and pickups suitable for sprite rendering.
  Evidence: THINGS type counts include type 2001 and pickup types such as 2014, 2015, and 2008.

- Observation: `std::ranges::find_if` over `std::array` returns a pointer-like iterator in this implementation, and clang-tidy requires that constness to be explicit.
  Evidence: `make check` reported `readability-qualified-auto` for the thing definition lookup in `game/sprites.cpp`.

- Observation: The full clean `make check` can exceed 120 seconds after adding sprite tests and clang-tidy coverage.
  Evidence: the first rerun timed out at 124 seconds; rerunning with a 240 second timeout completed successfully in 141.2 seconds with `7/7` tests passing.

## Decision Log

- Decision: Decode sprite patch dimensions and use solid-color billboard silhouettes before full column-post texture compositing.
  Rationale: The user requires loading sprite patches from the WAD and rendering billboards depth-sorted against walls. Decoding patch headers proves real sprite assets are used, while full masked-column drawing is a larger texture-compositing task that can be refined after the billboard pipeline exists. The billboards are still data-driven by real THINGS and real sprite patch dimensions, not placeholder geometry.
  Date/Author: 2026-05-02 / Codex

- Decision: Start with a minimal hardcoded Doom thing-type to sprite-prefix table for common E1M1 pickups/enemies.
  Rationale: Original Doom thing metadata is not fully self-describing in WAD map lumps; engines carry a thing definition table. A small explicit table for known Doom types is the correct architecture seed and can grow in later gameplay phases.
  Date/Author: 2026-05-02 / Codex

## Outcomes & Retrospective

Phase 6 now decodes sprite patch metadata from the real IWAD sprite range, builds renderable billboards from real E1M1 THINGS, and draws those billboards after wall columns in far-to-near order. The sprite tests prove the real IWAD catalog loads, the `BON1` sprite prefix exists, and E1M1 produces renderable thing sprites. The renderer test now requires sprite-colored pixels in addition to ceiling, floor, and wall pixels. The main lesson from this phase is that the Doom thing definition table is engine knowledge rather than WAD map data, so keeping a small explicit mapping is the right seed for later gameplay states.

## Context and Orientation

Phase 2 parses THINGS records into `MapData::things`. A THING record has x, y, angle, type, and flags. The type number identifies the gameplay object, such as a player start, monster, weapon, ammo, or health pickup. Phase 6 should not render player starts, but it should render pickups and enemies.

Doom sprite graphics are stored as patch lumps. A patch begins with an 8-byte header: signed 16-bit width, signed 16-bit height, signed 16-bit left offset, and signed 16-bit top offset. The full patch format then has one 32-bit column offset per pixel column followed by column posts. This phase needs at least width and height so the renderer can size billboards from real sprite assets.

Sprite lump names are typically eight characters: a four-letter sprite prefix, a frame letter, and rotation information. For example, a prefix such as `PLAY` or `POSS` has lumps like `POSSA1`. To render a simple billboard for a thing type, find the first sprite lump whose name starts with the type's four-letter prefix and appears between `S_START` and `S_END`.

## Plan of Work

Create `include/doomcpp/sprites.hpp` with `SpritePatch`, `ThingSprite`, `SpriteCatalog`, `load_sprite_catalog(const WadFile&)`, and `build_thing_sprites(const MapData&, const SpriteCatalog&)`. `SpriteCatalog` should own patch metadata loaded from the WAD. `ThingSprite` should store map position, type, radius/height derived from patch dimensions, a color, and depth filled during rendering.

Create `game/sprites.cpp` to locate sprite lumps between `S_START` and `S_END`, decode patch headers, normalize names, and build the minimal thing table. Include common E1M1 types such as former humans, imps, shotguns, clips, shells, stimpack/medikit, armor, barrels, and health/armor bonuses. If a type has no known sprite mapping, skip it.

Extend `include/doomcpp/render.hpp` and `engine/render.cpp` so `render_map_frame` accepts an optional span of `ThingSprite` values. Render walls first, then compute sprite camera-space depth, sort visible sprites far-to-near, and draw vertical billboard rectangles. The renderer should use simple depth sorting for sprites; precise wall occlusion can be improved later, but sprites must be sorted by depth and projected from real map positions.

Update `engine/sdl_app.cpp` and `tests/renderer_test.cpp` to load the sprite catalog and pass built thing sprites into rendering. Add `tests/sprite_test.cpp` to verify the catalog loads at least one known sprite patch from the real IWAD and that E1M1 produces non-empty thing sprites.

## Concrete Steps

From `C:\Users\giorg\Documents\codex\doom-cpp`, run:

    make check

Then manually run:

    .\build\check\doom_cpp.exe

The E1M1 window should show wall geometry and colored billboard objects at real THING positions.

## Validation and Acceptance

Phase 6 is accepted only when `make check` exits with status 0, real IWAD sprite metadata is decoded, E1M1 thing sprites are generated from real THINGS, the renderer test proves sprite-colored pixels appear, and the SDL executable opens successfully. Placeholder object positions are not acceptable.

## Idempotence and Recovery

Sprite parsing is read-only with respect to `assets/freedoom1.wad`. Generated renderer artifacts remain under ignored `build/check`. If a chosen thing type is absent in a different IWAD, tests should use the real Freedoom E1M1 data already committed in `assets/`.

## Artifacts and Notes

The sprite marker scan found:

    Index Name      Offset Size
     1005 S_START 15144684    0
     1859 S_END   17685676    0

E1M1 thing type counts include:

    Type 9: 13
    Type 2001: 4
    Type 2008: 18
    Type 2014: 30
    Type 2015: 22

## Interfaces and Dependencies

In `include/doomcpp/sprites.hpp`, define:

    struct SpritePatch { std::string lump_name; std::string prefix; std::uint16_t width; std::uint16_t height; };
    struct ThingSprite { float x; float y; std::int16_t type; std::uint16_t width; std::uint16_t height; std::uint32_t color; };
    class SpriteCatalog { ... find_by_prefix(...) ... patches() ... };
    SpriteCatalog load_sprite_catalog(const WadFile& wad);
    std::vector<ThingSprite> build_thing_sprites(const MapData& map, const SpriteCatalog& catalog);

This phase introduces no new external dependencies.

Revision note 2026-05-02 / Codex: Created this plan before adding Phase 6 sprite code so billboard rendering work is recoverable from a self-contained ExecPlan.

Revision note 2026-05-02 / Codex: Updated progress after implementing sprite metadata loading, billboard generation/rendering, tests, and the first clang-tidy fix.

Revision note 2026-05-02 / Codex: Recorded the long `make check` timeout, generated-directory cleanup, green Phase 6 result, and outcomes.

Revision note 2026-05-02 / Codex: Recorded the sprite-enabled SDL executable smoke run.

Revision note 2026-05-03 / Codex: Recorded the green `make check` result after updating this ExecPlan.
