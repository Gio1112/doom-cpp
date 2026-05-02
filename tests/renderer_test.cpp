#include "doomcpp/map.hpp"
#include "doomcpp/render.hpp"
#include "doomcpp/sprites.hpp"
#include "doomcpp/wad.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>

namespace {

constexpr doomcpp::RenderConfig test_render_config{
    .width = 320,
    .height = 200,
    .horizontal_fov_degrees = 90.0F,
};

constexpr std::string_view renderer_artifact_path = "e1m1-render.ppm";

[[nodiscard]] doomcpp::PlayerView player_one_start_view(const doomcpp::MapData& map) {
    const auto start = std::ranges::find_if(
        map.things, [](const doomcpp::Thing& thing) { return thing.type == 1; });
    if (start == map.things.end()) {
        throw std::runtime_error{"missing player 1 start"};
    }

    return doomcpp::PlayerView{
        .x = static_cast<float>(start->x),
        .y = static_cast<float>(start->y),
        .angle_degrees = static_cast<float>(start->angle),
    };
}

} // namespace

void write_ppm_artifact(const doomcpp::SoftwareFrame& frame, const std::string_view path) {
    std::ofstream output{std::string{path}, std::ios::binary};
    if (!output) {
        throw std::runtime_error{"failed to open renderer artifact for writing"};
    }

    output << "P6\n" << frame.width() << ' ' << frame.height() << "\n255\n";
    for (const std::uint32_t pixel : frame.pixels()) {
        const auto red = static_cast<char>((pixel >> 16U) & 0xFFU);
        const auto green = static_cast<char>((pixel >> 8U) & 0xFFU);
        const auto blue = static_cast<char>(pixel & 0xFFU);
        output.write(&red, 1);
        output.write(&green, 1);
        output.write(&blue, 1);
    }
}

int main() {
    try {
        const doomcpp::WadFile wad = doomcpp::WadFile::load_from_file(DOOMCPP_FREEDOOM1_WAD);
        const doomcpp::MapData map = doomcpp::load_map(wad, "E1M1");
        const doomcpp::SpriteCatalog sprite_catalog = doomcpp::load_sprite_catalog(wad);
        const std::vector<doomcpp::ThingSprite> sprites =
            doomcpp::build_thing_sprites(map, sprite_catalog);
        const doomcpp::SoftwareFrame frame =
            doomcpp::render_map_frame(map, player_one_start_view(map), test_render_config, sprites);

        const auto pixels = frame.pixels();
        const bool has_ceiling = std::ranges::find(pixels, doomcpp::ceiling_color) != pixels.end();
        const bool has_floor = std::ranges::find(pixels, doomcpp::floor_color) != pixels.end();
        const bool has_wall = std::ranges::any_of(pixels, [](const std::uint32_t color) {
            return color != doomcpp::ceiling_color && color != doomcpp::floor_color &&
                   color >= doomcpp::minimum_wall_color;
        });
        const bool has_sprite = std::ranges::any_of(pixels, [](const std::uint32_t color) {
            return color == doomcpp::sprite_pixel_color || color == 0xFFC04040U ||
                   color == 0xFFD8C078U;
        });

        if (!has_ceiling || !has_floor || !has_wall || !has_sprite) {
            std::cerr
                << "expected rendered frame to contain ceiling, floor, wall, and sprite pixels\n";
            return 1;
        }

        write_ppm_artifact(frame, renderer_artifact_path);
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "renderer test failed: " << exception.what() << '\n';
        return 1;
    }
}
