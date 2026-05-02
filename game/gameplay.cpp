#include "doomcpp/gameplay.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace doomcpp {
namespace {

constexpr float degrees_to_radians = std::numbers::pi_v<float> / 180.0F;
constexpr float aim_cone_cosine = 0.9659258F;
constexpr float hit_radius = 32.0F;
constexpr int hitscan_damage = 20;
constexpr float collision_epsilon = 0.001F;

struct Point2 {
    float x;
    float y;
};

[[nodiscard]] bool is_enemy_type(const std::int16_t type) noexcept {
    return type == 9 || type == 3001 || type == 3004;
}

[[nodiscard]] int starting_health_for_type(const std::int16_t type) noexcept {
    return type == 3001 ? 60 : 20;
}

[[nodiscard]] float cross(const Point2 first, const Point2 second) noexcept {
    return (first.x * second.y) - (first.y * second.x);
}

[[nodiscard]] bool segments_intersect(const Point2 first, const Point2 second, const Vertex& start,
                                      const Vertex& end) noexcept {
    const Point2 wall_start{
        .x = static_cast<float>(start.x),
        .y = static_cast<float>(start.y),
    };
    const Point2 wall_delta{
        .x = static_cast<float>(end.x - start.x),
        .y = static_cast<float>(end.y - start.y),
    };
    const Point2 move_delta{
        .x = second.x - first.x,
        .y = second.y - first.y,
    };
    const float denominator = cross(move_delta, wall_delta);
    if (std::abs(denominator) < collision_epsilon) {
        return false;
    }

    const Point2 relative{
        .x = wall_start.x - first.x,
        .y = wall_start.y - first.y,
    };
    const float move_fraction = cross(relative, wall_delta) / denominator;
    const float wall_fraction = cross(relative, move_delta) / denominator;
    return move_fraction >= 0.0F && move_fraction <= 1.0F && wall_fraction >= 0.0F &&
           wall_fraction <= 1.0F;
}

} // namespace

std::vector<EnemyState> build_enemies(const MapData& map) {
    std::vector<EnemyState> enemies;
    enemies.reserve(map.things.size());
    for (const Thing& thing : map.things) {
        if (!is_enemy_type(thing.type)) {
            continue;
        }
        enemies.push_back(EnemyState{
            .x = static_cast<float>(thing.x),
            .y = static_cast<float>(thing.y),
            .type = thing.type,
            .health = starting_health_for_type(thing.type),
            .state = EnemyStateKind::alive,
        });
    }
    return enemies;
}

std::optional<ShotResult> fire_hitscan(const std::span<EnemyState> enemies, const PlayerView view) {
    const float angle = view.angle_degrees * degrees_to_radians;
    const float forward_x = std::cos(angle);
    const float forward_y = std::sin(angle);

    std::size_t best_index = enemies.size();
    float best_distance = std::numeric_limits<float>::max();
    for (std::size_t index = 0; index < enemies.size(); ++index) {
        const EnemyState& enemy = enemies[index];
        if (enemy.state == EnemyStateKind::dead) {
            continue;
        }
        const float dx = enemy.x - view.x;
        const float dy = enemy.y - view.y;
        const float distance = std::hypot(dx, dy);
        if (distance < collision_epsilon) {
            continue;
        }
        const float aim = ((dx / distance) * forward_x) + ((dy / distance) * forward_y);
        const float lateral = std::abs((dx * forward_y) - (dy * forward_x));
        if (aim >= aim_cone_cosine && lateral <= hit_radius && distance < best_distance) {
            best_index = index;
            best_distance = distance;
        }
    }

    if (best_index == enemies.size()) {
        return std::nullopt;
    }

    EnemyState& enemy = enemies[best_index];
    enemy.health -= hitscan_damage;
    enemy.state = enemy.health <= 0 ? EnemyStateKind::dead : EnemyStateKind::pain;
    return ShotResult{
        .hit = true,
        .enemy_index = best_index,
        .damage = hitscan_damage,
        .resulting_state = enemy.state,
    };
}

PlayerState collide_player_move(const MapData& map, const PlayerState previous,
                                const PlayerState proposed, const float radius) noexcept {
    (void)radius;
    for (const Linedef& linedef : map.linedefs) {
        if (linedef.left_sidedef != -1 || linedef.start_vertex >= map.vertices.size() ||
            linedef.end_vertex >= map.vertices.size()) {
            continue;
        }
        if (segments_intersect(
                Point2{.x = previous.x, .y = previous.y}, Point2{.x = proposed.x, .y = proposed.y},
                map.vertices[linedef.start_vertex], map.vertices[linedef.end_vertex])) {
            return previous;
        }
    }
    return proposed;
}

} // namespace doomcpp
