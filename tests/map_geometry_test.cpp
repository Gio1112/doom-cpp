#include "doomcpp/map.hpp"
#include "doomcpp/wad.hpp"

#include <exception>
#include <iostream>
#include <string_view>

namespace {

constexpr std::size_t expected_e1m1_vertex_count = 1196;

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

        if (map.vertices.size() != expected_e1m1_vertex_count) {
            std::cerr << "expected E1M1 vertex count " << expected_e1m1_vertex_count << ", got "
                      << map.vertices.size() << '\n';
            return 1;
        }
        if (map.things.empty()) {
            return fail("expected E1M1 THINGS to decode at least one record");
        }
        if (map.linedefs.empty()) {
            return fail("expected E1M1 LINEDEFS to decode at least one record");
        }
        if (map.sidedefs.empty()) {
            return fail("expected E1M1 SIDEDEFS to decode at least one record");
        }
        if (map.sectors.empty()) {
            return fail("expected E1M1 SECTORS to decode at least one record");
        }

        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "map geometry test failed: " << exception.what() << '\n';
        return 1;
    }
}
