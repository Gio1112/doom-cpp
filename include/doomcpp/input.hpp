#pragma once

#include "doomcpp/render.hpp"

namespace doomcpp {

inline constexpr float simulation_tick_rate_hz = 35.0F;
inline constexpr float fixed_tick_seconds = 1.0F / simulation_tick_rate_hz;

struct InputState {
    float forward_axis;
    float strafe_axis;
    float mouse_delta_x;
};

struct PlayerState {
    float x;
    float y;
    float angle_degrees;
};

struct MovementConfig {
    float move_units_per_second;
    float mouse_degrees_per_count;
};

/// Advances player movement by one fixed 35 Hz simulation tick.
///
/// Preconditions: movement speeds are finite values and input axes are in the conventional
/// keyboard range [-1, 1].
/// Postconditions: returns the next player state after applying movement and yaw input.
/// Ownership: takes and returns values only.
[[nodiscard]] PlayerState advance_player(PlayerState player, InputState input,
                                         MovementConfig config) noexcept;

/// Converts simulation player state into a renderer view.
///
/// Preconditions: none.
/// Postconditions: returns a renderer-facing view with matching position and yaw.
/// Ownership: takes and returns values only.
[[nodiscard]] PlayerView to_player_view(PlayerState player) noexcept;

} // namespace doomcpp
