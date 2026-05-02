#include "doomcpp/app.hpp"

#include <iostream>
#include <stdexcept>

int main() {
    try {
        return doomcpp::run_game("assets/freedoom1.wad");
    } catch (const std::exception& exception) {
        std::cerr << "doom-cpp failed: " << exception.what() << '\n';
        return 1;
    }
}
