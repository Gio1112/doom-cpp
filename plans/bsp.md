# Traverse the Doom BSP Tree

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `PLANS.md` at the repository root. It describes Phase 3: parsing Doom's BSP lumps and using the node tree to find the subsector that contains the player's position.

## Purpose / Big Picture

After this phase, the engine can load E1M1's binary space partition data and answer a concrete spatial query: "which subsector is the player standing in?" A binary space partition tree, or BSP tree, recursively divides the 2D map with partition lines. Doom uses it to order walls for rendering and to accelerate spatial queries. The observable result is a real integration test that loads E1M1 from `assets/freedoom1.wad`, places the player at the real player 1 start `(-416, 256)`, traverses the BSP from the root node, and asserts that the result is a valid subsector index.

## Progress

- [x] (2026-05-02 22:02+01:00) Inspected E1M1 BSP lump counts in `assets/freedoom1.wad`.
- [x] (2026-05-02 22:03+01:00) Inspected the E1M1 player 1 start at `(-416, 256)` with angle `0`.
- [x] (2026-05-02 22:10+01:00) Extended public map data with SEGS, SSECTORS, and NODES typed structs.
- [x] (2026-05-02 22:10+01:00) Parsed BSP lumps in `game/map.cpp`.
- [x] (2026-05-02 22:10+01:00) Implemented BSP point traversal that returns a subsector index.
- [x] (2026-05-02 22:10+01:00) Added a real integration test for the E1M1 player start.
- [x] (2026-05-02 22:11+01:00) Ran `make check`; clang-format reported a style violation in `game/map.cpp`.
- [x] (2026-05-02 22:12+01:00) Applied clang-format to the BSP-touched map header, implementation, and test files.
- [x] (2026-05-02 22:13+01:00) Reran `make check`; clang-tidy reported dynamic array indexing and `modernize-use-auto` issues in BSP traversal.
- [x] (2026-05-02 22:14+01:00) Replaced dynamic `std::array` subscript with `.at()` and used `auto` for the masked subsector index.
- [x] (2026-05-02 22:16+01:00) Reran `make check`; clang-format, clang-tidy, Ninja build, and CTest all passed with `4/4` tests green.
- [x] (2026-05-02 22:17+01:00) Reran `make check` after the plan update; it passed with `4/4` tests green.
- [ ] Commit Phase 3 with a present-tense milestone message.

## Surprises & Discoveries

- Observation: Freedoom 0.13.0 E1M1 has one fewer node than subsector, which matches the normal full binary-tree shape used by Doom BSP data.
  Evidence: The `SSECTORS` lump has 682 records, and the `NODES` lump has 681 records.

- Observation: The real player 1 start for E1M1 is at map coordinates `(-416, 256)`.
  Evidence: Scanning THINGS records for type 1 produced `X=-416`, `Y=256`, `Angle=0`, and `Flags=7`.

- Observation: The first BSP implementation draft needed clang-format normalization before static analysis and build could run.
  Evidence: `make check` stopped at `clang-format check failed` with a diagnostic in `game/map.cpp`.

- Observation: The strict clang-tidy profile rejects dynamic `std::array` subscripting even when the index is constrained to 0 or 1.
  Evidence: `make check` reported `cppcoreguidelines-pro-bounds-constant-array-index` for `node.children[child_side_for_point(node, point)]`.

## Decision Log

- Decision: Add BSP structures to `MapData` rather than creating a separate loaded-BSP object in Phase 3.
  Rationale: Doom stores BSP data as part of the map lump sequence, and later renderer/collision code needs geometry and BSP records together. Keeping them in one owning `MapData` keeps startup parsing simple and avoids premature lifetime complexity.
  Date/Author: 2026-05-02 / Codex

- Decision: Return an optional subsector index from traversal.
  Rationale: A valid loaded Doom map should always produce a subsector, but `std::optional<std::uint16_t>` gives tests and callers a clean way to detect malformed or missing BSP data without sentinel values.
  Date/Author: 2026-05-02 / Codex

## Outcomes & Retrospective

Phase 3 now parses E1M1 SEGS, SSECTORS, and NODES into typed records, keeps BSP data together with the rest of `MapData`, and traverses from the root node to a subsector for a supplied map point. The BSP integration test proves the real Freedoom E1M1 counts of 2,057 segs, 682 subsectors, and 681 nodes, then verifies that the real player 1 start `(-416, 256)` resolves to an in-range subsector. The main lesson from this phase is that even a narrow spatial query benefits from representing complete node records now, because renderer ordering will need the same decoded tree and bounding boxes in the next phase.

## Context and Orientation

Phase 2 parses core map geometry from E1M1. Phase 3 adds the BSP-specific lumps that immediately follow VERTEXES in a Doom map: SEGS, SSECTORS, and NODES. A seg is a directed wall segment produced by splitting linedefs along BSP partition boundaries. A subsector is a leaf region containing a consecutive range of segs. A node is an internal BSP tree branch with a partition line and two children. Child 0 is the front side and child 1 is the back side. Doom marks a child as a subsector by setting bit 15 on the child value; the lower 15 bits are then the subsector index. If bit 15 is clear, the child value is another node index.

The relevant Doom BSP record layouts are:

SEGS stores wall fragments. Each record is 12 bytes: unsigned 16-bit start vertex index, unsigned 16-bit end vertex index, signed 16-bit angle, unsigned 16-bit linedef index, signed 16-bit direction, and signed 16-bit offset.

SSECTORS stores subsector ranges. Each record is 4 bytes: unsigned 16-bit seg count and unsigned 16-bit first seg index.

NODES stores BSP branches. Each record is 28 bytes: signed 16-bit partition x, signed 16-bit partition y, signed 16-bit dx, signed 16-bit dy, two child bounding boxes with four signed 16-bit values each, and two unsigned 16-bit child references. The bounding boxes are ordered as top, bottom, left, right for each child. Traversal only needs the partition line and child references in this phase.

To classify a point against a node partition, compute:

    side = (point_x - node.x) * node.dy - (point_y - node.y) * node.dx

If `side <= 0`, use child 0; otherwise use child 1. This convention follows Doom's front/back child ordering for point-in-subsector traversal.

## Plan of Work

Extend `include/doomcpp/map.hpp` with `Seg`, `Subsector`, `BoundingBox`, `Node`, `BspPoint`, and `find_subsector_containing_point(const MapData&, BspPoint)`. Add `std::vector<Seg> segs`, `std::vector<Subsector> subsectors`, and `std::vector<Node> nodes` to `MapData`.

Extend `game/map.cpp` to parse SEGS, SSECTORS, and NODES. Validate exact record-size multiples as in Phase 2. For NODES, decode both child bounding boxes and child references even though traversal only needs the references; this keeps the typed representation complete for later renderer work.

Implement `find_subsector_containing_point` in `game/map.cpp`. It should start at the last node in `map.nodes`, because Doom stores the root node last. It should follow node child references until it reaches a subsector child. It should return `std::nullopt` if there are no nodes, if a node child references an out-of-range node, or if the final subsector index is out of range.

Add `tests/bsp_test.cpp`. The test loads `assets/freedoom1.wad`, parses E1M1, calls `find_subsector_containing_point(map, {.x = -416, .y = 256})`, and asserts the optional has a value less than `map.subsectors.size()`. It should also assert the real BSP counts: 2,057 segs, 682 subsectors, and 681 nodes.

## Concrete Steps

From `C:\Users\giorg\Documents\codex\doom-cpp`, run:

    make check

After implementation, CTest should include the bootstrap, WAD directory, map geometry, and BSP traversal tests. The BSP test must use the real Freedoom E1M1 player start from the THINGS lump.

## Validation and Acceptance

Phase 3 is accepted only when `make check` exits with status 0 and the BSP traversal test passes. The test must prove that parsing E1M1 yields exactly 2,057 segs, 682 subsectors, and 681 nodes, and that traversing from player start `(-416, 256)` returns a valid subsector index. Returning any placeholder value without walking real nodes is not acceptable.

## Idempotence and Recovery

All Phase 3 parsing is read-only with respect to assets. `make check` recreates `build/check`, so stale generated files are safe. If the WAD asset is missing, restore `assets/freedoom1.wad` from the official Freedoom 0.13.0 archive used in Phase 1.

## Artifacts and Notes

The real E1M1 BSP counts from `assets/freedoom1.wad` are:

    SEGS: 24684 bytes / 12 bytes per record = 2057 records
    SSECTORS: 2728 bytes / 4 bytes per record = 682 records
    NODES: 19068 bytes / 28 bytes per record = 681 records

The real player starts from E1M1 THINGS include:

    X     Y    Angle  Type  Flags
    -416  256  0      1     7
    -416  304  0      2     7
    -464  258  0      3     7
    -416  208  0      4     7

## Interfaces and Dependencies

In `include/doomcpp/map.hpp`, add:

    struct Seg { std::uint16_t start_vertex; std::uint16_t end_vertex; std::int16_t angle; std::uint16_t linedef; std::int16_t direction; std::int16_t offset; };
    struct Subsector { std::uint16_t seg_count; std::uint16_t first_seg; };
    struct BoundingBox { std::int16_t top; std::int16_t bottom; std::int16_t left; std::int16_t right; };
    struct Node { std::int16_t x; std::int16_t y; std::int16_t dx; std::int16_t dy; std::array<BoundingBox, 2> bounding_boxes; std::array<std::uint16_t, 2> children; };
    struct BspPoint { std::int32_t x; std::int32_t y; };
    std::optional<std::uint16_t> find_subsector_containing_point(const MapData& map, BspPoint point);

This phase introduces no new external dependency. It depends on the existing WAD reader, map geometry parser, and real `assets/freedoom1.wad` asset.

Revision note 2026-05-02 / Codex: Created this plan before adding Phase 3 BSP code so traversal work is recoverable from a self-contained ExecPlan.

Revision note 2026-05-02 / Codex: Updated progress after adding BSP structs, BSP lump decoding, point traversal, and the real player-start test.

Revision note 2026-05-02 / Codex: Recorded the clang-format failure and fix for the Phase 3 BSP files.

Revision note 2026-05-02 / Codex: Recorded clang-tidy traversal findings and the fixes made in response.

Revision note 2026-05-02 / Codex: Recorded the green Phase 3 `make check` result and outcomes after the real BSP traversal test passed.

Revision note 2026-05-02 / Codex: Recorded the green `make check` result after updating this ExecPlan.
