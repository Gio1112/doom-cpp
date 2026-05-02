#include "doomcpp/sprites.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <optional>

namespace doomcpp {
namespace {

constexpr std::size_t patch_header_size = 8;
constexpr std::size_t sprite_prefix_size = 4;

struct ThingDefinition {
    std::int16_t type;
    std::string_view prefix;
    std::uint32_t color;
};

constexpr std::array thing_definitions{
    ThingDefinition{.type = 9, .prefix = "SPOS", .color = 0xFFC04040U},
    ThingDefinition{.type = 3004, .prefix = "POSS", .color = 0xFFC04040U},
    ThingDefinition{.type = 3001, .prefix = "TROO", .color = 0xFFD06040U},
    ThingDefinition{.type = 2001, .prefix = "SHOT", .color = 0xFFE0E0A0U},
    ThingDefinition{.type = 2002, .prefix = "MGUN", .color = 0xFFE0E0A0U},
    ThingDefinition{.type = 2003, .prefix = "LAUN", .color = 0xFFE0E0A0U},
    ThingDefinition{.type = 2004, .prefix = "PLAS", .color = 0xFFE0E0A0U},
    ThingDefinition{.type = 2005, .prefix = "CSAW", .color = 0xFFE0E0A0U},
    ThingDefinition{.type = 2007, .prefix = "CLIP", .color = 0xFFD8C078U},
    ThingDefinition{.type = 2008, .prefix = "SHEL", .color = 0xFFD8C078U},
    ThingDefinition{.type = 2010, .prefix = "ROCK", .color = 0xFFD8C078U},
    ThingDefinition{.type = 2011, .prefix = "STIM", .color = 0xFFE8E8E8U},
    ThingDefinition{.type = 2012, .prefix = "MEDI", .color = 0xFFE8E8E8U},
    ThingDefinition{.type = 2014, .prefix = "BON1", .color = 0xFF60A0FFU},
    ThingDefinition{.type = 2015, .prefix = "BON2", .color = 0xFF60FF90U},
    ThingDefinition{.type = 2018, .prefix = "ARM1", .color = 0xFF60FF90U},
    ThingDefinition{.type = 2019, .prefix = "ARM2", .color = 0xFF60A0FFU},
    ThingDefinition{.type = 2035, .prefix = "BAR1", .color = 0xFFC08040U},
};

[[nodiscard]] std::uint16_t read_u16_le(const std::span<const std::byte> bytes,
                                        const std::size_t offset) {
    const auto byte_at = [bytes, offset](const std::size_t index) {
        return static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[offset + index]));
    };
    return static_cast<std::uint16_t>(byte_at(0) | static_cast<std::uint16_t>(byte_at(1) << 8U));
}

[[nodiscard]] std::int16_t read_i16_le(const std::span<const std::byte> bytes,
                                       const std::size_t offset) {
    return std::bit_cast<std::int16_t>(read_u16_le(bytes, offset));
}

[[nodiscard]] std::string prefix_from_lump_name(const std::string_view lump_name) {
    return std::string{lump_name.substr(0, std::min(lump_name.size(), sprite_prefix_size))};
}

[[nodiscard]] std::optional<SpritePatch> decode_patch(const WadFile& wad, const Lump& lump) {
    if (lump.size < patch_header_size || lump.name.size() < sprite_prefix_size) {
        return std::nullopt;
    }

    const std::span<const std::byte> bytes = wad.lump_data(lump);
    const std::int16_t width = read_i16_le(bytes, 0);
    const std::int16_t height = read_i16_le(bytes, 2);
    if (width <= 0 || height <= 0) {
        return std::nullopt;
    }

    return SpritePatch{
        .lump_name = lump.name,
        .prefix = prefix_from_lump_name(lump.name),
        .width = static_cast<std::uint16_t>(width),
        .height = static_cast<std::uint16_t>(height),
    };
}

[[nodiscard]] const ThingDefinition* find_definition(const std::int16_t type) noexcept {
    const auto* const found = std::ranges::find_if(
        thing_definitions, [type](const ThingDefinition& def) { return def.type == type; });
    return found == thing_definitions.end() ? nullptr : std::addressof(*found);
}

} // namespace

void SpriteCatalog::add_patch(SpritePatch patch) {
    patches_.push_back(std::move(patch));
}

std::span<const SpritePatch> SpriteCatalog::patches() const noexcept {
    return patches_;
}

const SpritePatch* SpriteCatalog::find_by_prefix(const std::string_view prefix) const noexcept {
    const auto found = std::ranges::find_if(
        patches_, [prefix](const SpritePatch& patch) { return patch.prefix == prefix; });
    return found == patches_.end() ? nullptr : std::addressof(*found);
}

SpriteCatalog load_sprite_catalog(const WadFile& wad) {
    SpriteCatalog catalog;
    const std::span<const Lump> lumps = wad.lumps();
    const auto start =
        std::ranges::find_if(lumps, [](const Lump& lump) { return lump.name == "S_START"; });
    const auto end =
        std::ranges::find_if(lumps, [](const Lump& lump) { return lump.name == "S_END"; });
    if (start == lumps.end() || end == lumps.end() || start >= end) {
        return catalog;
    }

    for (auto current = std::next(start); current != end; ++current) {
        if (std::optional<SpritePatch> patch = decode_patch(wad, *current); patch.has_value()) {
            catalog.add_patch(std::move(*patch));
        }
    }

    return catalog;
}

std::vector<ThingSprite> build_thing_sprites(const MapData& map, const SpriteCatalog& catalog) {
    std::vector<ThingSprite> sprites;
    sprites.reserve(map.things.size());
    for (const Thing& thing : map.things) {
        const ThingDefinition* definition = find_definition(thing.type);
        if (definition == nullptr) {
            continue;
        }
        const SpritePatch* patch = catalog.find_by_prefix(definition->prefix);
        if (patch == nullptr) {
            continue;
        }
        sprites.push_back(ThingSprite{
            .x = static_cast<float>(thing.x),
            .y = static_cast<float>(thing.y),
            .type = thing.type,
            .width = patch->width,
            .height = patch->height,
            .color = definition->color,
        });
    }
    return sprites;
}

} // namespace doomcpp
