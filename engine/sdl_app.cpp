#include "doomcpp/app.hpp"

#include "doomcpp/map.hpp"
#include "doomcpp/render.hpp"
#include "doomcpp/wad.hpp"

#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
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

[[nodiscard]] PlayerView player_one_start_view(const MapData& map) {
    const auto start =
        std::ranges::find_if(map.things, [](const Thing& thing) { return thing.type == 1; });
    if (start == map.things.end()) {
        throw std::runtime_error{"E1M1 has no player 1 start"};
    }

    return PlayerView{
        .x = static_cast<float>(start->x),
        .y = static_cast<float>(start->y),
        .angle_degrees = static_cast<float>(start->angle),
    };
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

} // namespace

int run_game(const std::filesystem::path& wad_path) {
    const WadFile wad = WadFile::load_from_file(resolve_wad_path(wad_path));
    const MapData map = load_map(wad, "E1M1");
    const PlayerView view = player_one_start_view(map);
    const SoftwareFrame frame = render_map_frame(map, view, default_render_config);

    const SdlRuntime sdl;
    SdlWindow window{default_render_config};

    const auto start_time = std::chrono::steady_clock::now();
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN) {
                running = false;
            }
        }

        window.present(frame);
        if (std::chrono::steady_clock::now() - start_time >= smoke_loop_duration) {
            running = false;
        }
        SDL_Delay(16);
    }

    return 0;
}

} // namespace doomcpp
