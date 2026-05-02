#include "doomcpp/gameplay.hpp"
#include "doomcpp/map.hpp"
#include "doomcpp/wad.hpp"

#include <cmath>
#include <exception>
#include <iostream>

namespace {

[[nodiscard]] float angle_to(const doomcpp::PlayerState player, const doomcpp::EnemyState enemy) {
    float degrees =
        std::atan2(enemy.y - player.y, enemy.x - player.x) * 180.0F / std::numbers::pi_v<float>;
    if (degrees < 0.0F) {
        degrees += 360.0F;
    }
    return degrees;
}

} // namespace

int main() {
    try {
        const doomcpp::WadFile wad = doomcpp::WadFile::load_from_file(DOOMCPP_FREEDOOM1_WAD);
        const doomcpp::MapData map = doomcpp::load_map(wad, "E1M1");
        std::vector<doomcpp::EnemyState> enemies = doomcpp::build_enemies(map);
        if (enemies.empty()) {
            std::cerr << "expected E1M1 to build enemy states\n";
            return 1;
        }

        doomcpp::PlayerState player{
            .x = enemies.front().x - 128.0F,
            .y = enemies.front().y,
            .angle_degrees = 0.0F,
        };
        player.angle_degrees = angle_to(player, enemies.front());
        const int old_health = enemies.front().health;
        const std::optional<doomcpp::ShotResult> shot =
            doomcpp::fire_hitscan(enemies, doomcpp::to_player_view(player));
        if (!shot.has_value() || !shot->hit || shot->enemy_index != 0U) {
            std::cerr << "expected hitscan to hit the first aimed enemy\n";
            return 1;
        }
        if (enemies.front().health >= old_health ||
            enemies.front().state == doomcpp::EnemyStateKind::alive) {
            std::cerr << "expected enemy to lose health and enter pain/death state\n";
            return 1;
        }

        const doomcpp::Linedef* wall = nullptr;
        for (const doomcpp::Linedef& linedef : map.linedefs) {
            if (linedef.left_sidedef == -1 && linedef.start_vertex < map.vertices.size() &&
                linedef.end_vertex < map.vertices.size()) {
                wall = std::addressof(linedef);
                break;
            }
        }
        if (wall == nullptr) {
            std::cerr << "expected E1M1 to contain a one-sided blocking linedef\n";
            return 1;
        }
        const doomcpp::Vertex start = map.vertices[wall->start_vertex];
        const doomcpp::Vertex end = map.vertices[wall->end_vertex];
        const doomcpp::PlayerState previous{
            .x = static_cast<float>(start.x),
            .y = static_cast<float>(start.y),
            .angle_degrees = 0.0F,
        };
        const doomcpp::PlayerState proposed{
            .x = static_cast<float>(end.x),
            .y = static_cast<float>(end.y),
            .angle_degrees = 0.0F,
        };
        const doomcpp::PlayerState collided =
            doomcpp::collide_player_move(map, previous, proposed, 16.0F);
        if (collided.x != previous.x || collided.y != previous.y) {
            std::cerr << "expected movement crossing a one-sided linedef to be blocked\n";
            return 1;
        }

        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "gameplay test failed: " << exception.what() << '\n';
        return 1;
    }
}
