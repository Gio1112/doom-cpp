#pragma once

#include "doomcpp/map.hpp"
#include "doomcpp/wad.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace doomcpp {

inline constexpr std::uint32_t sprite_pixel_color = 0xFFC85858U;

struct SpritePatch {
    std::string lump_name;
    std::string prefix;
    std::uint16_t width;
    std::uint16_t height;
};

struct ThingSprite {
    float x;
    float y;
    std::int16_t type;
    std::uint16_t width;
    std::uint16_t height;
    std::uint32_t color;
};

class SpriteCatalog {
  public:
    /// Stores one decoded WAD sprite patch metadata record.
    ///
    /// Preconditions: patch dimensions are decoded from a real WAD patch lump.
    /// Postconditions: the patch can be found by prefix or listed through `patches()`.
    /// Ownership: copies and owns the patch strings and metadata.
    void add_patch(SpritePatch patch);

    /// Returns all decoded sprite patches.
    ///
    /// Preconditions: none.
    /// Postconditions: returned span is valid while this catalog is alive and unmodified.
    /// Ownership: borrows `this`.
    [[nodiscard]] std::span<const SpritePatch> patches() const noexcept;

    /// Finds the first decoded patch with a matching four-character sprite prefix.
    ///
    /// Preconditions: `prefix` is a Doom sprite prefix such as POSS or BON1.
    /// Postconditions: returns a borrowed patch pointer, or nullptr when absent.
    /// Ownership: borrows `this` and `prefix`.
    [[nodiscard]] const SpritePatch* find_by_prefix(std::string_view prefix) const noexcept;

  private:
    std::vector<SpritePatch> patches_;
};

/// Loads sprite patch metadata from the WAD's sprite marker range.
///
/// Preconditions: `wad` contains S_START and S_END marker lumps.
/// Postconditions: returns decoded patch dimensions for sprite lumps with valid patch headers.
/// Ownership: borrows `wad`; owns returned metadata.
[[nodiscard]] SpriteCatalog load_sprite_catalog(const WadFile& wad);

/// Builds renderable thing billboards from real map THINGS and decoded sprite metadata.
///
/// Preconditions: `map` was decoded from a Doom map and `catalog` was loaded from the same IWAD.
/// Postconditions: returns sprite billboards for known renderable thing types.
/// Ownership: borrows inputs; owns returned billboard records.
[[nodiscard]] std::vector<ThingSprite> build_thing_sprites(const MapData& map,
                                                           const SpriteCatalog& catalog);

} // namespace doomcpp
