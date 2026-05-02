#include "doomcpp/map.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <stdexcept>

namespace doomcpp {
namespace {

constexpr std::size_t thing_record_size = 10;
constexpr std::size_t vertex_record_size = 4;
constexpr std::size_t linedef_record_size = 14;
constexpr std::size_t sidedef_record_size = 30;
constexpr std::size_t sector_record_size = 26;
constexpr std::size_t seg_record_size = 12;
constexpr std::size_t subsector_record_size = 4;
constexpr std::size_t node_record_size = 28;
constexpr std::size_t texture_name_size = 8;
constexpr std::uint16_t subsector_child_flag = 0x8000U;
constexpr std::uint16_t child_index_mask = 0x7FFFU;

[[nodiscard]] char uppercase_ascii(const char value) noexcept {
    if (value >= 'a' && value <= 'z') {
        return static_cast<char>(value - ('a' - 'A'));
    }
    return value;
}

[[nodiscard]] std::string normalize_name(const std::string_view name) {
    std::string normalized;
    normalized.reserve(name.size());
    for (const char value : name) {
        normalized.push_back(uppercase_ascii(value));
    }
    return normalized;
}

[[nodiscard]] bool is_digit(const char value) noexcept {
    return value >= '0' && value <= '9';
}

[[nodiscard]] bool is_map_marker_name(const std::string_view name) noexcept {
    if (name.size() == 4U && uppercase_ascii(name[0]) == 'E' && is_digit(name[1]) &&
        uppercase_ascii(name[2]) == 'M' && is_digit(name[3])) {
        return true;
    }

    return name.size() == 5U && uppercase_ascii(name[0]) == 'M' &&
           uppercase_ascii(name[1]) == 'A' && uppercase_ascii(name[2]) == 'P' &&
           is_digit(name[3]) && is_digit(name[4]);
}

[[nodiscard]] std::uint16_t read_u16_le(const std::span<const std::byte> bytes,
                                        const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint16_t)) {
        throw std::runtime_error{"map field extends beyond lump bounds"};
    }

    const auto byte_at = [bytes, offset](const std::size_t index) {
        return static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[offset + index]));
    };

    return static_cast<std::uint16_t>(byte_at(0) | static_cast<std::uint16_t>(byte_at(1) << 8U));
}

[[nodiscard]] std::int16_t read_i16_le(const std::span<const std::byte> bytes,
                                       const std::size_t offset) {
    return std::bit_cast<std::int16_t>(read_u16_le(bytes, offset));
}

void require_record_multiple(const Lump& lump, const std::size_t record_size) {
    if (lump.size % record_size != 0U) {
        throw std::runtime_error{"map lump " + lump.name + " has a partial trailing record"};
    }
}

[[nodiscard]] std::string read_name(const std::span<const std::byte> bytes,
                                    const std::size_t offset) {
    const std::span<const std::byte> name_bytes = bytes.subspan(offset, texture_name_size);
    std::string name;
    name.reserve(texture_name_size);

    for (const std::byte value : name_bytes) {
        const char character = static_cast<char>(std::to_integer<unsigned char>(value));
        if (character == '\0') {
            break;
        }
        name.push_back(uppercase_ascii(character));
    }

    return name;
}

[[nodiscard]] const Lump& required_lump(const std::span<const Lump> map_lumps,
                                        const std::string_view name) {
    const auto found =
        std::ranges::find_if(map_lumps, [name](const Lump& lump) { return lump.name == name; });
    if (found == map_lumps.end()) {
        throw std::runtime_error{"map is missing required lump: " + std::string{name}};
    }
    return *found;
}

[[nodiscard]] std::span<const Lump> find_map_lumps(const WadFile& wad,
                                                   const std::string_view map_name) {
    const std::string normalized_map_name = normalize_name(map_name);
    const std::span<const Lump> lumps = wad.lumps();
    const auto marker = std::ranges::find_if(lumps, [&normalized_map_name](const Lump& lump) {
        return lump.name == normalized_map_name;
    });
    if (marker == lumps.end()) {
        throw std::runtime_error{"map marker not found: " + normalized_map_name};
    }

    const auto marker_index = static_cast<std::size_t>(std::distance(lumps.begin(), marker));
    std::size_t end_index = lumps.size();
    for (std::size_t index = marker_index + 1U; index < lumps.size(); ++index) {
        if (is_map_marker_name(lumps[index].name)) {
            end_index = index;
            break;
        }
    }

    return lumps.subspan(marker_index + 1U, end_index - marker_index - 1U);
}

[[nodiscard]] std::vector<Thing> parse_things(const WadFile& wad, const Lump& lump) {
    require_record_multiple(lump, thing_record_size);
    const std::span<const std::byte> bytes = wad.lump_data(lump);
    std::vector<Thing> things;
    things.reserve(bytes.size() / thing_record_size);

    for (std::size_t offset = 0; offset < bytes.size(); offset += thing_record_size) {
        things.push_back(Thing{
            .x = read_i16_le(bytes, offset),
            .y = read_i16_le(bytes, offset + 2U),
            .angle = read_i16_le(bytes, offset + 4U),
            .type = read_i16_le(bytes, offset + 6U),
            .flags = read_i16_le(bytes, offset + 8U),
        });
    }

    return things;
}

[[nodiscard]] std::vector<Linedef> parse_linedefs(const WadFile& wad, const Lump& lump) {
    require_record_multiple(lump, linedef_record_size);
    const std::span<const std::byte> bytes = wad.lump_data(lump);
    std::vector<Linedef> linedefs;
    linedefs.reserve(bytes.size() / linedef_record_size);

    for (std::size_t offset = 0; offset < bytes.size(); offset += linedef_record_size) {
        linedefs.push_back(Linedef{
            .start_vertex = read_u16_le(bytes, offset),
            .end_vertex = read_u16_le(bytes, offset + 2U),
            .flags = read_u16_le(bytes, offset + 4U),
            .special_type = read_u16_le(bytes, offset + 6U),
            .sector_tag = read_u16_le(bytes, offset + 8U),
            .right_sidedef = read_i16_le(bytes, offset + 10U),
            .left_sidedef = read_i16_le(bytes, offset + 12U),
        });
    }

    return linedefs;
}

[[nodiscard]] std::vector<Sidedef> parse_sidedefs(const WadFile& wad, const Lump& lump) {
    require_record_multiple(lump, sidedef_record_size);
    const std::span<const std::byte> bytes = wad.lump_data(lump);
    std::vector<Sidedef> sidedefs;
    sidedefs.reserve(bytes.size() / sidedef_record_size);

    for (std::size_t offset = 0; offset < bytes.size(); offset += sidedef_record_size) {
        sidedefs.push_back(Sidedef{
            .x_offset = read_i16_le(bytes, offset),
            .y_offset = read_i16_le(bytes, offset + 2U),
            .upper_texture = read_name(bytes, offset + 4U),
            .lower_texture = read_name(bytes, offset + 12U),
            .middle_texture = read_name(bytes, offset + 20U),
            .sector = read_i16_le(bytes, offset + 28U),
        });
    }

    return sidedefs;
}

[[nodiscard]] std::vector<Vertex> parse_vertices(const WadFile& wad, const Lump& lump) {
    require_record_multiple(lump, vertex_record_size);
    const std::span<const std::byte> bytes = wad.lump_data(lump);
    std::vector<Vertex> vertices;
    vertices.reserve(bytes.size() / vertex_record_size);

    for (std::size_t offset = 0; offset < bytes.size(); offset += vertex_record_size) {
        vertices.push_back(Vertex{
            .x = read_i16_le(bytes, offset),
            .y = read_i16_le(bytes, offset + 2U),
        });
    }

    return vertices;
}

[[nodiscard]] std::vector<Sector> parse_sectors(const WadFile& wad, const Lump& lump) {
    require_record_multiple(lump, sector_record_size);
    const std::span<const std::byte> bytes = wad.lump_data(lump);
    std::vector<Sector> sectors;
    sectors.reserve(bytes.size() / sector_record_size);

    for (std::size_t offset = 0; offset < bytes.size(); offset += sector_record_size) {
        sectors.push_back(Sector{
            .floor_height = read_i16_le(bytes, offset),
            .ceiling_height = read_i16_le(bytes, offset + 2U),
            .floor_texture = read_name(bytes, offset + 4U),
            .ceiling_texture = read_name(bytes, offset + 12U),
            .light_level = read_i16_le(bytes, offset + 20U),
            .special_type = read_i16_le(bytes, offset + 22U),
            .tag = read_i16_le(bytes, offset + 24U),
        });
    }

    return sectors;
}

[[nodiscard]] std::vector<Seg> parse_segs(const WadFile& wad, const Lump& lump) {
    require_record_multiple(lump, seg_record_size);
    const std::span<const std::byte> bytes = wad.lump_data(lump);
    std::vector<Seg> segs;
    segs.reserve(bytes.size() / seg_record_size);

    for (std::size_t offset = 0; offset < bytes.size(); offset += seg_record_size) {
        segs.push_back(Seg{
            .start_vertex = read_u16_le(bytes, offset),
            .end_vertex = read_u16_le(bytes, offset + 2U),
            .angle = read_i16_le(bytes, offset + 4U),
            .linedef = read_u16_le(bytes, offset + 6U),
            .direction = read_i16_le(bytes, offset + 8U),
            .offset = read_i16_le(bytes, offset + 10U),
        });
    }

    return segs;
}

[[nodiscard]] std::vector<Subsector> parse_subsectors(const WadFile& wad, const Lump& lump) {
    require_record_multiple(lump, subsector_record_size);
    const std::span<const std::byte> bytes = wad.lump_data(lump);
    std::vector<Subsector> subsectors;
    subsectors.reserve(bytes.size() / subsector_record_size);

    for (std::size_t offset = 0; offset < bytes.size(); offset += subsector_record_size) {
        subsectors.push_back(Subsector{
            .seg_count = read_u16_le(bytes, offset),
            .first_seg = read_u16_le(bytes, offset + 2U),
        });
    }

    return subsectors;
}

[[nodiscard]] BoundingBox parse_bounding_box(const std::span<const std::byte> bytes,
                                             const std::size_t offset) {
    return BoundingBox{
        .top = read_i16_le(bytes, offset),
        .bottom = read_i16_le(bytes, offset + 2U),
        .left = read_i16_le(bytes, offset + 4U),
        .right = read_i16_le(bytes, offset + 6U),
    };
}

[[nodiscard]] std::vector<Node> parse_nodes(const WadFile& wad, const Lump& lump) {
    require_record_multiple(lump, node_record_size);
    const std::span<const std::byte> bytes = wad.lump_data(lump);
    std::vector<Node> nodes;
    nodes.reserve(bytes.size() / node_record_size);

    for (std::size_t offset = 0; offset < bytes.size(); offset += node_record_size) {
        nodes.push_back(Node{
            .x = read_i16_le(bytes, offset),
            .y = read_i16_le(bytes, offset + 2U),
            .dx = read_i16_le(bytes, offset + 4U),
            .dy = read_i16_le(bytes, offset + 6U),
            .bounding_boxes =
                {
                    parse_bounding_box(bytes, offset + 8U),
                    parse_bounding_box(bytes, offset + 16U),
                },
            .children =
                {
                    read_u16_le(bytes, offset + 24U),
                    read_u16_le(bytes, offset + 26U),
                },
        });
    }

    return nodes;
}

[[nodiscard]] std::size_t child_side_for_point(const Node& node, const BspPoint point) {
    const std::int64_t dx = static_cast<std::int64_t>(point.x) - node.x;
    const std::int64_t dy = static_cast<std::int64_t>(point.y) - node.y;
    const std::int64_t side =
        (dx * static_cast<std::int64_t>(node.dy)) - (dy * static_cast<std::int64_t>(node.dx));
    return side <= 0 ? 0U : 1U;
}

} // namespace

MapData load_map(const WadFile& wad, const std::string_view map_name) {
    const std::span<const Lump> map_lumps = find_map_lumps(wad, map_name);

    return MapData{
        .things = parse_things(wad, required_lump(map_lumps, "THINGS")),
        .linedefs = parse_linedefs(wad, required_lump(map_lumps, "LINEDEFS")),
        .sidedefs = parse_sidedefs(wad, required_lump(map_lumps, "SIDEDEFS")),
        .vertices = parse_vertices(wad, required_lump(map_lumps, "VERTEXES")),
        .sectors = parse_sectors(wad, required_lump(map_lumps, "SECTORS")),
        .segs = parse_segs(wad, required_lump(map_lumps, "SEGS")),
        .subsectors = parse_subsectors(wad, required_lump(map_lumps, "SSECTORS")),
        .nodes = parse_nodes(wad, required_lump(map_lumps, "NODES")),
    };
}

std::optional<std::uint16_t> find_subsector_containing_point(const MapData& map,
                                                             const BspPoint point) {
    if (map.nodes.empty()) {
        return std::nullopt;
    }

    auto node_index = static_cast<std::uint16_t>(map.nodes.size() - 1U);
    for (;;) {
        if (node_index >= map.nodes.size()) {
            return std::nullopt;
        }

        const Node& node = map.nodes[node_index];
        const std::uint16_t child = node.children.at(child_side_for_point(node, point));
        if ((child & subsector_child_flag) != 0U) {
            const auto subsector_index = static_cast<std::uint16_t>(child & child_index_mask);
            if (subsector_index >= map.subsectors.size()) {
                return std::nullopt;
            }
            return subsector_index;
        }

        node_index = child;
    }
}

} // namespace doomcpp
