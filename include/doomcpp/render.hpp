#pragma once

#include "doomcpp/map.hpp"
#include "doomcpp/sprites.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace doomcpp {

inline constexpr std::uint32_t ceiling_color = 0xFF383850U;
inline constexpr std::uint32_t floor_color = 0xFF303020U;
inline constexpr std::uint32_t minimum_wall_color = 0xFF707070U;

struct RenderConfig {
    std::uint16_t width;
    std::uint16_t height;
    float horizontal_fov_degrees;
};

struct PlayerView {
    float x;
    float y;
    float angle_degrees;
};

class SoftwareFrame {
  public:
    /// Creates an owning framebuffer with one 32-bit RGBA pixel per screen pixel.
    ///
    /// Preconditions: `width` and `height` are non-zero.
    /// Postconditions: allocates `width * height` pixels initialized to zero.
    /// Ownership: owns the pixel storage.
    SoftwareFrame(std::uint16_t width, std::uint16_t height);

    /// Returns the framebuffer width in pixels.
    ///
    /// Preconditions: none.
    /// Postconditions: returns the width passed at construction.
    /// Ownership: borrows `this` and returns a value.
    [[nodiscard]] std::uint16_t width() const noexcept;

    /// Returns the framebuffer height in pixels.
    ///
    /// Preconditions: none.
    /// Postconditions: returns the height passed at construction.
    /// Ownership: borrows `this` and returns a value.
    [[nodiscard]] std::uint16_t height() const noexcept;

    /// Returns mutable pixel storage in row-major order.
    ///
    /// Preconditions: none.
    /// Postconditions: returned span remains valid while this frame is alive and unmodified.
    /// Ownership: borrows `this`; callers must not outlive this SoftwareFrame.
    [[nodiscard]] std::span<std::uint32_t> pixels() noexcept;

    /// Returns immutable pixel storage in row-major order.
    ///
    /// Preconditions: none.
    /// Postconditions: returned span remains valid while this frame is alive and unmodified.
    /// Ownership: borrows `this`; callers must not outlive this SoftwareFrame.
    [[nodiscard]] std::span<const std::uint32_t> pixels() const noexcept;

  private:
    std::uint16_t width_;
    std::uint16_t height_;
    std::vector<std::uint32_t> pixels_;
};

/// Renders a software frame for a map from the supplied player viewpoint.
///
/// Preconditions: `map` contains vertices and linedefs decoded from a real Doom map, and config
/// dimensions and FOV are positive.
/// Postconditions: returns an owning framebuffer containing ceiling, floor, and projected walls.
/// Ownership: borrows `map`; owns the returned frame pixels.
[[nodiscard]] SoftwareFrame render_map_frame(const MapData& map, PlayerView view,
                                             RenderConfig config,
                                             std::span<const ThingSprite> sprites = {});

} // namespace doomcpp
