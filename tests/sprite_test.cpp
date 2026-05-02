#include "doomcpp/map.hpp"
#include "doomcpp/render.hpp"
#include "doomcpp/sprites.hpp"
#include "doomcpp/wad.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        const doomcpp::WadFile wad = doomcpp::WadFile::load_from_file(DOOMCPP_FREEDOOM1_WAD);
        const doomcpp::SpriteCatalog catalog = doomcpp::load_sprite_catalog(wad);
        if (catalog.patches().empty()) {
            std::cerr << "expected real IWAD sprite patches to load\n";
            return 1;
        }
        if (catalog.find_by_prefix("BON1") == nullptr) {
            std::cerr << "expected health bonus sprite prefix BON1 to exist\n";
            return 1;
        }

        const doomcpp::MapData map = doomcpp::load_map(wad, "E1M1");
        const std::vector<doomcpp::ThingSprite> sprites =
            doomcpp::build_thing_sprites(map, catalog);
        if (sprites.empty()) {
            std::cerr << "expected E1M1 to produce renderable thing sprites\n";
            return 1;
        }

        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "sprite test failed: " << exception.what() << '\n';
        return 1;
    }
}
