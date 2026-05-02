#pragma once

#include "doomcpp/wad.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace doomcpp {

struct Thing {
    std::int16_t x;
    std::int16_t y;
    std::int16_t angle;
    std::int16_t type;
    std::int16_t flags;
};

struct Vertex {
    std::int16_t x;
    std::int16_t y;
};

struct Linedef {
    std::uint16_t start_vertex;
    std::uint16_t end_vertex;
    std::uint16_t flags;
    std::uint16_t special_type;
    std::uint16_t sector_tag;
    std::int16_t right_sidedef;
    std::int16_t left_sidedef;
};

struct Sidedef {
    std::int16_t x_offset;
    std::int16_t y_offset;
    std::string upper_texture;
    std::string lower_texture;
    std::string middle_texture;
    std::int16_t sector;
};

struct Sector {
    std::int16_t floor_height;
    std::int16_t ceiling_height;
    std::string floor_texture;
    std::string ceiling_texture;
    std::int16_t light_level;
    std::int16_t special_type;
    std::int16_t tag;
};

struct Seg {
    std::uint16_t start_vertex;
    std::uint16_t end_vertex;
    std::int16_t angle;
    std::uint16_t linedef;
    std::int16_t direction;
    std::int16_t offset;
};

struct Subsector {
    std::uint16_t seg_count;
    std::uint16_t first_seg;
};

struct BoundingBox {
    std::int16_t top;
    std::int16_t bottom;
    std::int16_t left;
    std::int16_t right;
};

struct Node {
    std::int16_t x;
    std::int16_t y;
    std::int16_t dx;
    std::int16_t dy;
    std::array<BoundingBox, 2> bounding_boxes;
    std::array<std::uint16_t, 2> children;
};

struct BspPoint {
    std::int32_t x;
    std::int32_t y;
};

struct MapData {
    std::vector<Thing> things;
    std::vector<Linedef> linedefs;
    std::vector<Sidedef> sidedefs;
    std::vector<Vertex> vertices;
    std::vector<Sector> sectors;
    std::vector<Seg> segs;
    std::vector<Subsector> subsectors;
    std::vector<Node> nodes;
};

/// Loads core Doom map geometry from an already-open WAD archive.
///
/// Preconditions: `wad` contains a map marker named `map_name` followed by THINGS, LINEDEFS,
/// SIDEDEFS, VERTEXES, and SECTORS lumps before the next map marker.
/// Postconditions: returns owning typed map vectors decoded from the requested map's raw lumps.
/// Ownership: borrows `wad` and `map_name`; owns all returned map records and texture strings.
[[nodiscard]] MapData load_map(const WadFile& wad, std::string_view map_name);

/// Traverses the map BSP tree to find the subsector containing a point.
///
/// Preconditions: `map` contains parsed Doom NODES and SSECTORS data for one map.
/// Postconditions: returns a valid subsector index, or std::nullopt if BSP data is malformed.
/// Ownership: borrows `map`; returns a value.
[[nodiscard]] std::optional<std::uint16_t> find_subsector_containing_point(const MapData& map,
                                                                           BspPoint point);

} // namespace doomcpp
