#include "doomcpp/input.hpp"

#include <cmath>
#include <numbers>

namespace doomcpp {
namespace {

constexpr float degrees_to_radians = std::numbers::pi_v<float> / 180.0F;
constexpr float full_turn_degrees = 360.0F;

[[nodiscard]] float wrap_degrees(float angle) noexcept {
    while (angle < 0.0F) {
        angle += full_turn_degrees;
    }
    while (angle >= full_turn_degrees) {
        angle -= full_turn_degrees;
    }
    return angle;
}

} // namespace

PlayerState advance_player(PlayerState player, const InputState input,
                           const MovementConfig config) noexcept {
    player.angle_degrees =
        wrap_degrees(player.angle_degrees + (input.mouse_delta_x * config.mouse_degrees_per_count));

    const float angle = player.angle_degrees * degrees_to_radians;
    const float forward_x = std::cos(angle);
    const float forward_y = std::sin(angle);
    const float strafe_x = -forward_y;
    const float strafe_y = forward_x;
    const float movement = config.move_units_per_second * fixed_tick_seconds;

    player.x += ((forward_x * input.forward_axis) + (strafe_x * input.strafe_axis)) * movement;
    player.y += ((forward_y * input.forward_axis) + (strafe_y * input.strafe_axis)) * movement;
    return player;
}

PlayerView to_player_view(const PlayerState player) noexcept {
    return PlayerView{
        .x = player.x,
        .y = player.y,
        .angle_degrees = player.angle_degrees,
    };
}

} // namespace doomcpp
