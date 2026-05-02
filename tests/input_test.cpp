#include "doomcpp/input.hpp"

#include <cmath>
#include <iostream>

namespace {

constexpr doomcpp::MovementConfig test_config{
    .move_units_per_second = 128.0F,
    .mouse_degrees_per_count = 0.5F,
};

[[nodiscard]] bool near_equal(const float left, const float right) noexcept {
    return std::abs(left - right) < 0.01F;
}

} // namespace

int main() {
    doomcpp::PlayerState player{
        .x = 0.0F,
        .y = 0.0F,
        .angle_degrees = 0.0F,
    };
    const doomcpp::InputState forward_input{
        .forward_axis = 1.0F,
        .strafe_axis = 0.0F,
        .mouse_delta_x = 0.0F,
    };

    for (int tick = 0; tick < 35; ++tick) {
        player = doomcpp::advance_player(player, forward_input, test_config);
    }

    if (!near_equal(player.x, 128.0F) || !near_equal(player.y, 0.0F)) {
        std::cerr << "expected one second of forward movement to reach x=128 y=0, got x="
                  << player.x << " y=" << player.y << '\n';
        return 1;
    }

    const doomcpp::InputState mouse_input{
        .forward_axis = 0.0F,
        .strafe_axis = 0.0F,
        .mouse_delta_x = 10.0F,
    };
    player = doomcpp::advance_player(player, mouse_input, test_config);
    if (!near_equal(player.angle_degrees, 5.0F)) {
        std::cerr << "expected mouse yaw to reach 5 degrees, got " << player.angle_degrees << '\n';
        return 1;
    }

    return 0;
}
