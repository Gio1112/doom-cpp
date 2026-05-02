#pragma once

#include "doomcpp/input.hpp"
#include "doomcpp/map.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace doomcpp {

enum class EnemyStateKind : std::uint8_t { alive, pain, dead };

struct EnemyState {
    float x;
    float y;
    std::int16_t type;
    int health;
    EnemyStateKind state;
};

struct ShotResult {
    bool hit;
    std::size_t enemy_index;
    int damage;
    EnemyStateKind resulting_state;
};

/// Builds live enemy state from Doom THINGS records.
///
/// Preconditions: `map` was decoded from a real Doom map.
/// Postconditions: returns enemies for supported enemy THING types.
/// Ownership: borrows `map`; owns returned enemy states.
[[nodiscard]] std::vector<EnemyState> build_enemies(const MapData& map);

/// Fires one deterministic hitscan shot from the player view.
///
/// Preconditions: `enemies` points to mutable enemy state and `view` is the player's current view.
/// Postconditions: damages the closest living enemy in the aim cone, if any.
/// Ownership: mutably borrows `enemies`; returns a value describing the shot result.
[[nodiscard]] std::optional<ShotResult> fire_hitscan(std::span<EnemyState> enemies,
                                                     PlayerView view);

/// Resolves a proposed player movement against solid one-sided linedefs.
///
/// Preconditions: `map` contains decoded vertices and linedefs from a real Doom map.
/// Postconditions: returns `proposed` when no solid wall blocks the move, otherwise `previous`.
/// Ownership: borrows `map`; takes and returns player state values.
[[nodiscard]] PlayerState collide_player_move(const MapData& map, PlayerState previous,
                                              PlayerState proposed, float radius) noexcept;

} // namespace doomcpp
