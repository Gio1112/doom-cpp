#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace doomcpp {

enum class WadType : std::uint8_t { iwad, pwad };

struct Lump {
    std::string name;
    std::uint32_t offset;
    std::uint32_t size;
};

class WadFile {
  public:
    /// Loads and validates a Doom WAD archive from disk.
    ///
    /// Preconditions: `path` names a readable WAD file with an IWAD or PWAD header.
    /// Postconditions: returns an owning WadFile whose lump metadata and bytes remain stable.
    /// Ownership: borrows `path`; owns all loaded archive bytes and parsed lump metadata.
    static WadFile load_from_file(const std::filesystem::path& path);

    /// Returns whether the archive is a complete IWAD or a patch PWAD.
    ///
    /// Preconditions: none.
    /// Postconditions: returns the type parsed from the WAD header.
    /// Ownership: borrows `this` and returns a value.
    [[nodiscard]] WadType type() const noexcept;

    /// Returns all parsed lump directory entries in archive order.
    ///
    /// Preconditions: none.
    /// Postconditions: returned span remains valid while this WadFile is alive and unmodified.
    /// Ownership: borrows `this`; callers must not store the span beyond this WadFile lifetime.
    [[nodiscard]] std::span<const Lump> lumps() const noexcept;

    /// Finds the first lump with a case-insensitive Doom lump name match.
    ///
    /// Preconditions: `name` is the desired lump name without path separators.
    /// Postconditions: returns a pointer to the matching lump, or nullptr when absent.
    /// Ownership: borrows `name` and `this`; returned pointer is borrowed from this WadFile.
    [[nodiscard]] const Lump* find_lump(std::string_view name) const noexcept;

    /// Returns the raw bytes for a lump.
    ///
    /// Preconditions: `lump` must be one of the entries returned by `lumps()` for this WadFile.
    /// Postconditions: returned span points at the exact byte range declared by the WAD directory.
    /// Ownership: borrows `this`; returned bytes remain valid for this WadFile lifetime.
    [[nodiscard]] std::span<const std::byte> lump_data(const Lump& lump) const;

  private:
    WadType type_{WadType::iwad};
    std::vector<std::byte> data_;
    std::vector<Lump> lumps_;
};

} // namespace doomcpp
