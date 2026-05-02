#include "doomcpp/render.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace doomcpp {
namespace {

constexpr float degrees_to_radians = std::numbers::pi_v<float> / 180.0F;
constexpr float near_plane = 1.0F;
constexpr float wall_height_world_units = 128.0F;

struct CameraPoint {
    float x;
    float y;
};

struct ColumnSpan {
    std::int32_t screen_x;
    std::int32_t top;
    std::int32_t bottom;
};

struct WallDrawStyle {
    float focal_length;
    std::uint32_t color;
};

[[nodiscard]] std::uint32_t wall_color_for_linedef(const std::size_t index) noexcept {
    const auto shade = static_cast<std::uint32_t>(112U + ((index * 37U) % 96U));
    return 0xFF000000U | (shade << 16U) | (shade << 8U) | shade;
}

[[nodiscard]] CameraPoint transform_to_camera(const Vertex& vertex, const PlayerView view) {
    const float angle = view.angle_degrees * degrees_to_radians;
    const float cos_angle = std::cos(angle);
    const float sin_angle = std::sin(angle);
    const float translated_x = static_cast<float>(vertex.x) - view.x;
    const float translated_y = static_cast<float>(vertex.y) - view.y;

    return CameraPoint{
        .x = (translated_x * cos_angle) + (translated_y * sin_angle),
        .y = (-translated_x * sin_angle) + (translated_y * cos_angle),
    };
}

void clip_to_near_plane(CameraPoint& first, CameraPoint& second) {
    const float denominator = second.x - first.x;
    if (std::abs(denominator) < 0.0001F) {
        first.x = std::max(first.x, near_plane);
        second.x = std::max(second.x, near_plane);
        return;
    }

    if (first.x < near_plane) {
        const float lerp_amount = (near_plane - first.x) / denominator;
        first.y += (second.y - first.y) * lerp_amount;
        first.x = near_plane;
    }
    if (second.x < near_plane) {
        const float lerp_amount = (near_plane - second.x) / (first.x - second.x);
        second.y += (first.y - second.y) * lerp_amount;
        second.x = near_plane;
    }
}

[[nodiscard]] float project_x(const CameraPoint point, const RenderConfig config,
                              const float focal_length) {
    return (static_cast<float>(config.width) * 0.5F) + ((point.y / point.x) * focal_length);
}

void draw_column(SoftwareFrame& frame, const ColumnSpan column, const std::uint32_t color) {
    if (column.screen_x < 0 || std::cmp_greater_equal(column.screen_x, frame.width())) {
        return;
    }

    const std::int32_t clamped_top = std::max(0, column.top);
    const std::int32_t clamped_bottom =
        std::min(static_cast<std::int32_t>(frame.height()) - 1, column.bottom);
    if (clamped_top > clamped_bottom) {
        return;
    }

    std::span<std::uint32_t> pixels = frame.pixels();
    for (std::int32_t screen_y = clamped_top; screen_y <= clamped_bottom; ++screen_y) {
        pixels[(static_cast<std::size_t>(screen_y) * frame.width()) +
               static_cast<std::size_t>(column.screen_x)] = color;
    }
}

void draw_wall_segment(SoftwareFrame& frame, const CameraPoint first, const CameraPoint second,
                       const RenderConfig config, const WallDrawStyle style) {
    const float projected_first = project_x(first, config, style.focal_length);
    const float projected_second = project_x(second, config, style.focal_length);
    const float left = std::min(projected_first, projected_second);
    const float right = std::max(projected_first, projected_second);
    if (right < 0.0F || left >= static_cast<float>(config.width) || std::abs(right - left) < 0.5F) {
        return;
    }

    const float max_screen_x = static_cast<float>(config.width) - 1.0F;
    const auto start_x =
        static_cast<std::int32_t>(std::floor(std::clamp(left, 0.0F, max_screen_x)));
    const auto end_x = static_cast<std::int32_t>(std::ceil(std::clamp(right, 0.0F, max_screen_x)));

    for (std::int32_t screen_x = start_x; screen_x <= end_x; ++screen_x) {
        const float lerp_amount = (right - left) < 0.0001F
                                      ? 0.0F
                                      : (static_cast<float>(screen_x) - left) / (right - left);
        const float depth = first.x + ((second.x - first.x) * std::clamp(lerp_amount, 0.0F, 1.0F));
        const float projected_height =
            (wall_height_world_units / std::max(depth, near_plane)) * style.focal_length * 0.5F;
        const auto center_y = static_cast<std::int32_t>(config.height / 2U);
        const auto half_height = static_cast<std::int32_t>(std::max(1.0F, projected_height));
        draw_column(frame,
                    ColumnSpan{
                        .screen_x = screen_x,
                        .top = center_y - half_height,
                        .bottom = center_y + half_height,
                    },
                    style.color);
    }
}

} // namespace

SoftwareFrame::SoftwareFrame(const std::uint16_t width, const std::uint16_t height)
    : width_{width}, height_{height}, pixels_(static_cast<std::size_t>(width) * height) {
    if (width == 0U || height == 0U) {
        throw std::runtime_error{"software frame dimensions must be non-zero"};
    }
}

std::uint16_t SoftwareFrame::width() const noexcept {
    return width_;
}

std::uint16_t SoftwareFrame::height() const noexcept {
    return height_;
}

std::span<std::uint32_t> SoftwareFrame::pixels() noexcept {
    return pixels_;
}

std::span<const std::uint32_t> SoftwareFrame::pixels() const noexcept {
    return pixels_;
}

SoftwareFrame render_map_frame(const MapData& map, const PlayerView view,
                               const RenderConfig config) {
    if (config.width == 0U || config.height == 0U || config.horizontal_fov_degrees <= 0.0F) {
        throw std::runtime_error{"invalid render configuration"};
    }

    SoftwareFrame frame{config.width, config.height};
    std::span<std::uint32_t> pixels = frame.pixels();
    for (std::uint16_t screen_y = 0; screen_y < config.height; ++screen_y) {
        const std::uint32_t color = screen_y < (config.height / 2U) ? ceiling_color : floor_color;
        const std::size_t row_start = static_cast<std::size_t>(screen_y) * config.width;
        std::fill(pixels.begin() + static_cast<std::ptrdiff_t>(row_start),
                  pixels.begin() + static_cast<std::ptrdiff_t>(row_start + config.width), color);
    }

    const float half_fov = (config.horizontal_fov_degrees * degrees_to_radians) * 0.5F;
    const float focal_length = (static_cast<float>(config.width) * 0.5F) / std::tan(half_fov);

    for (std::size_t index = 0; index < map.linedefs.size(); ++index) {
        const Linedef& linedef = map.linedefs[index];
        if (linedef.start_vertex >= map.vertices.size() ||
            linedef.end_vertex >= map.vertices.size()) {
            continue;
        }

        CameraPoint first = transform_to_camera(map.vertices[linedef.start_vertex], view);
        CameraPoint second = transform_to_camera(map.vertices[linedef.end_vertex], view);
        if (first.x < near_plane && second.x < near_plane) {
            continue;
        }

        clip_to_near_plane(first, second);
        draw_wall_segment(frame, first, second, config,
                          WallDrawStyle{
                              .focal_length = focal_length,
                              .color = wall_color_for_linedef(index),
                          });
    }

    return frame;
}

} // namespace doomcpp
