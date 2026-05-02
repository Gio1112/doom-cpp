#pragma once

#include <string_view>

namespace doomcpp {

/// Project name used in startup output.
///
/// Preconditions: none.
/// Postconditions: returns a static string view that remains valid for the entire process.
/// Ownership: borrows storage with static lifetime; callers must not attempt to free it.
inline constexpr std::string_view project_name = "doom-cpp";

/// Boot message printed by the Phase 0 executable.
///
/// Preconditions: none.
/// Postconditions: returns a static string view that remains valid for the entire process.
/// Ownership: borrows storage with static lifetime; callers must not attempt to free it.
inline constexpr std::string_view boot_message = "doom-cpp bootstrap: engine entry point online";

} // namespace doomcpp
