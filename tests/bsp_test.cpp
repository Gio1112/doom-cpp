#include "doomcpp/map.hpp"
#include "doomcpp/wad.hpp"

#include <exception>
#include <iostream>
#include <string_view>

namespace {

constexpr std::size_t expected_e1m1_seg_count = 2057;
constexpr std::size_t expected_e1m1_subsector_count = 682;
constexpr std::size_t expected_e1m1_node_count = 681;

/// Reports a failed test expectation with a concrete message.
///
/// Preconditions: `message` describes the failed observable condition.
/// Postconditions: writes the message to stderr and returns a failing test status.
/// Ownership: borrows `message`.
int fail(const std::string_view message) {
    std::cerr << message << '\n';
    return 1;
}

} // namespace

int main() {
    try {
        const doomcpp::WadFile wad = doomcpp::WadFile::load_from_file(DOOMCPP_FREEDOOM1_WAD);
        const doomcpp::MapData map = doomcpp::load_map(wad, "E1M1");

        if (map.segs.size() != expected_e1m1_seg_count) {
            std::cerr << "expected E1M1 seg count " << expected_e1m1_seg_count << ", got "
                      << map.segs.size() << '\n';
            return 1;
        }
        if (map.subsectors.size() != expected_e1m1_subsector_count) {
            std::cerr << "expected E1M1 subsector count " << expected_e1m1_subsector_count
                      << ", got " << map.subsectors.size() << '\n';
            return 1;
        }
        if (map.nodes.size() != expected_e1m1_node_count) {
            std::cerr << "expected E1M1 node count " << expected_e1m1_node_count << ", got "
                      << map.nodes.size() << '\n';
            return 1;
        }

        const std::optional<std::uint16_t> subsector =
            doomcpp::find_subsector_containing_point(map, doomcpp::BspPoint{.x = -416, .y = 256});
        if (!subsector.has_value()) {
            return fail("expected player 1 start to resolve to a subsector");
        }
        if (*subsector >= map.subsectors.size()) {
            return fail("expected player 1 start subsector to be in range");
        }

        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "bsp test failed: " << exception.what() << '\n';
        return 1;
    }
}
