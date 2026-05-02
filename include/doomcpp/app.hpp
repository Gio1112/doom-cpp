#pragma once

#include <filesystem>

namespace doomcpp {

/// Runs the SDL game application using the supplied IWAD path.
///
/// Preconditions: `wad_path` names a readable IWAD containing E1M1.
/// Postconditions: opens a window, presents a rendered E1M1 frame, and returns a process status.
/// Ownership: borrows `wad_path`; SDL resources are owned locally and released before return.
int run_game(const std::filesystem::path& wad_path);

} // namespace doomcpp
