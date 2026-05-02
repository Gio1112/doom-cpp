#include "doomcpp/wad.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <string_view>

namespace {

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
        if (wad.type() != doomcpp::WadType::iwad) {
            return fail("expected Freedoom Phase 1 to be an IWAD");
        }
        if (wad.lumps().empty()) {
            return fail("expected Freedoom Phase 1 to contain at least one lump");
        }
        if (wad.lumps().front().name != "E1M1") {
            std::cerr << "expected first lump name E1M1, got " << wad.lumps().front().name << '\n';
            return 1;
        }
        if (wad.lumps().front().offset != 12U || wad.lumps().front().size != 0U) {
            return fail("expected first E1M1 marker lump at offset 12 with zero size");
        }

        const doomcpp::Lump* e1m1 = wad.find_lump("e1m1");
        if (e1m1 == nullptr) {
            return fail("expected case-insensitive lookup to find E1M1");
        }
        if (!wad.lump_data(*e1m1).empty()) {
            return fail("expected E1M1 marker lump to have no payload bytes");
        }

        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "wad test failed: " << exception.what() << '\n';
        return 1;
    }
}
