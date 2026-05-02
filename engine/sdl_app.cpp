#include "doomcpp/app.hpp"

#include "doomcpp/gameplay.hpp"
#include "doomcpp/input.hpp"
#include "doomcpp/map.hpp"
#include "doomcpp/render.hpp"
#include "doomcpp/sprites.hpp"
#include "doomcpp/wad.hpp"

#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>

namespace doomcpp {
namespace {

constexpr RenderConfig default_render_config{
    .width = 640,
    .height = 400,
    .horizontal_fov_degrees = 90.0F,
};
constexpr std::chrono::milliseconds smoke_loop_duration{3000};
constexpr MovementConfig default_movement_config{
    .move_units_per_second = 128.0F,
    .mouse_degrees_per_count = 0.12F,
};

struct EventResult {
    bool running;
    bool fire_requested;
};

class SdlRuntime {
  public:
    SdlRuntime() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            throw std::runtime_error{"SDL_Init failed: " + std::string{SDL_GetError()}};
        }
    }

    SdlRuntime(const SdlRuntime&) = delete;
    SdlRuntime& operator=(const SdlRuntime&) = delete;
    SdlRuntime(SdlRuntime&&) = delete;
    SdlRuntime& operator=(SdlRuntime&&) = delete;

    ~SdlRuntime() {
        SDL_Quit();
    }
};

class SdlWindow {
  public:
    SdlWindow(const RenderConfig config)
        : window_{SDL_CreateWindow("doom-cpp", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   config.width, config.height, SDL_WINDOW_SHOWN)} {
        if (window_ == nullptr) {
            throw std::runtime_error{"SDL_CreateWindow failed: " + std::string{SDL_GetError()}};
        }

        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
        if (renderer_ == nullptr) {
            throw std::runtime_error{"SDL_CreateRenderer failed: " + std::string{SDL_GetError()}};
        }

        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, config.width, config.height);
        if (texture_ == nullptr) {
            throw std::runtime_error{"SDL_CreateTexture failed: " + std::string{SDL_GetError()}};
        }
    }

    SdlWindow(const SdlWindow&) = delete;
    SdlWindow& operator=(const SdlWindow&) = delete;
    SdlWindow(SdlWindow&&) = delete;
    SdlWindow& operator=(SdlWindow&&) = delete;

    ~SdlWindow() {
        if (texture_ != nullptr) {
            SDL_DestroyTexture(texture_);
        }
        if (renderer_ != nullptr) {
            SDL_DestroyRenderer(renderer_);
        }
        if (window_ != nullptr) {
            SDL_DestroyWindow(window_);
        }
    }

    void present(const SoftwareFrame& frame) {
        const int pitch = static_cast<int>(frame.width() * sizeof(std::uint32_t));
        if (SDL_UpdateTexture(texture_, nullptr, frame.pixels().data(), pitch) != 0) {
            throw std::runtime_error{"SDL_UpdateTexture failed: " + std::string{SDL_GetError()}};
        }
        if (SDL_RenderClear(renderer_) != 0) {
            throw std::runtime_error{"SDL_RenderClear failed: " + std::string{SDL_GetError()}};
        }
        if (SDL_RenderCopy(renderer_, texture_, nullptr, nullptr) != 0) {
            throw std::runtime_error{"SDL_RenderCopy failed: " + std::string{SDL_GetError()}};
        }
        SDL_RenderPresent(renderer_);
    }

  private:
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    SDL_Texture* texture_{nullptr};
};

[[nodiscard]] PlayerState player_one_start_state(const MapData& map) {
    const auto start =
        std::ranges::find_if(map.things, [](const Thing& thing) { return thing.type == 1; });
    if (start == map.things.end()) {
        throw std::runtime_error{"E1M1 has no player 1 start"};
    }

    return PlayerState{
        .x = static_cast<float>(start->x),
        .y = static_cast<float>(start->y),
        .angle_degrees = static_cast<float>(start->angle),
    };
}

[[nodiscard]] bool smoke_test_requested() noexcept {
    return SDL_getenv("DOOMCPP_SMOKE_TEST") != nullptr;
}

[[nodiscard]] std::filesystem::path resolve_wad_path(const std::filesystem::path& requested_path) {
    if (std::filesystem::exists(requested_path)) {
        return requested_path;
    }

    std::filesystem::path executable_path = SDL_GetBasePath();
    std::filesystem::path repository_relative =
        executable_path / ".." / ".." / "assets" / requested_path.filename();
    if (std::filesystem::exists(repository_relative)) {
        return repository_relative;
    }

    return requested_path;
}

[[nodiscard]] EventResult poll_events(InputState& input, const EventResult previous) {
    EventResult result = previous;
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        const bool requested_quit = event.type == SDL_QUIT || (event.type == SDL_KEYDOWN &&
                                                               event.key.keysym.sym == SDLK_ESCAPE);
        if (requested_quit) {
            result.running = false;
        } else if (event.type == SDL_MOUSEMOTION) {
            input.mouse_delta_x += static_cast<float>(event.motion.xrel);
        } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            result.fire_requested = true;
        }
    }
    return result;
}

[[nodiscard]] bool key_is_down(const std::span<const Uint8> keys, const SDL_Scancode scancode) {
    const auto index = static_cast<std::size_t>(scancode);
    return index < keys.size() && keys[index] != 0U;
}

void sample_keyboard(InputState& input) {
    int key_count = 0;
    const Uint8* keyboard = SDL_GetKeyboardState(&key_count);
    const std::span<const Uint8> keys{keyboard, static_cast<std::size_t>(key_count)};

    input.forward_axis = 0.0F;
    input.strafe_axis = 0.0F;
    if (key_is_down(keys, SDL_SCANCODE_W)) {
        input.forward_axis += 1.0F;
    }
    if (key_is_down(keys, SDL_SCANCODE_S)) {
        input.forward_axis -= 1.0F;
    }
    if (key_is_down(keys, SDL_SCANCODE_D)) {
        input.strafe_axis += 1.0F;
    }
    if (key_is_down(keys, SDL_SCANCODE_A)) {
        input.strafe_axis -= 1.0F;
    }
}

void step_fixed_ticks(const MapData& map, PlayerState& player, InputState& input,
                      float& accumulator_seconds) {
    while (accumulator_seconds >= fixed_tick_seconds) {
        const PlayerState proposed = advance_player(player, input, default_movement_config);
        player = collide_player_move(map, player, proposed, 16.0F);
        input.mouse_delta_x = 0.0F;
        accumulator_seconds -= fixed_tick_seconds;
    }
}

} // namespace

int run_game(const std::filesystem::path& wad_path) {
    const WadFile wad = WadFile::load_from_file(resolve_wad_path(wad_path));
    const MapData map = load_map(wad, "E1M1");
    const SpriteCatalog sprite_catalog = load_sprite_catalog(wad);
    const std::vector<ThingSprite> sprites = build_thing_sprites(map, sprite_catalog);
    std::vector<EnemyState> enemies = build_enemies(map);
    PlayerState player = player_one_start_state(map);

    const SdlRuntime sdl;
    SdlWindow window{default_render_config};
    if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0) {
        throw std::runtime_error{"SDL_SetRelativeMouseMode failed: " + std::string{SDL_GetError()}};
    }

    const auto start_time = std::chrono::steady_clock::now();
    const bool bounded_smoke_loop = smoke_test_requested();
    auto previous_time = start_time;
    float accumulator_seconds = 0.0F;
    InputState input{
        .forward_axis = 0.0F,
        .strafe_axis = 0.0F,
        .mouse_delta_x = 0.0F,
    };
    EventResult events{
        .running = true,
        .fire_requested = false,
    };
    while (events.running) {
        input.mouse_delta_x = 0.0F;
        events.fire_requested = false;
        events = poll_events(input, events);
        sample_keyboard(input);

        const auto current_time = std::chrono::steady_clock::now();
        accumulator_seconds += std::chrono::duration<float>(current_time - previous_time).count();
        previous_time = current_time;
        step_fixed_ticks(map, player, input, accumulator_seconds);
        if (events.fire_requested) {
            (void)fire_hitscan(enemies, to_player_view(player));
        }

        const SoftwareFrame frame =
            render_map_frame(map, to_player_view(player), default_render_config, sprites);
        window.present(frame);
        if (bounded_smoke_loop &&
            std::chrono::steady_clock::now() - start_time >= smoke_loop_duration) {
            events.running = false;
        }
        SDL_Delay(16);
    }

    return 0;
}

} // namespace doomcpp
