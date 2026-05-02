#include "doomcpp/config.hpp"

#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

/// Runs the built game executable and captures stdout for inspection.
///
/// Preconditions: `command` names an executable command that can be launched by the host shell.
/// Postconditions: returns all bytes written to stdout before the process exits normally.
/// Ownership: owns the pipe handle while reading; returns an owning string to the caller.
std::string run_and_capture(const std::string& command) {
#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe{_popen(command.c_str(), "r"), _pclose};
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe{popen(command.c_str(), "r"), pclose};
#endif
    if (pipe == nullptr) {
        throw std::runtime_error{"failed to launch command: " + command};
    }

    std::array<char, 256> buffer{};
    std::string output;
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        output.append(buffer.data());
    }
    return output;
}

/// Adds shell quoting around a path so whitespace in the build directory is safe.
///
/// Preconditions: `path` does not contain embedded quote characters.
/// Postconditions: returns a string suitable for passing to the platform shell.
/// Ownership: borrows `path` and returns an owning string.
std::string quote_for_shell(std::string_view path) {
#ifdef _WIN32
    return '"' + std::string{path} + '"';
#else
    return '\'' + std::string{path} + '\'';
#endif
}

} // namespace

int main() {
    try {
        const std::string output = run_and_capture(quote_for_shell(DOOMCPP_BINARY_PATH));
        const std::string expected{doomcpp::boot_message};

        if (output.find(expected) == std::string::npos) {
            std::cerr << "expected boot output containing: " << expected << '\n'
                      << "actual output: " << output << '\n';
            return 1;
        }

        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "bootstrap test failed: " << exception.what() << '\n';
        return 1;
    }
}
