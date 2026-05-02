#include "doomcpp/wad.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace doomcpp {
namespace {

constexpr std::size_t wad_header_size = 12;
constexpr std::size_t wad_directory_entry_size = 16;
constexpr std::size_t wad_lump_name_size = 8;

struct ByteRange {
    std::uint32_t offset;
    std::uint32_t size;
};

[[nodiscard]] char uppercase_ascii(const char value) noexcept {
    if (value >= 'a' && value <= 'z') {
        return static_cast<char>(value - ('a' - 'A'));
    }
    return value;
}

[[nodiscard]] std::string normalize_lump_name(const std::string_view name) {
    std::string normalized;
    normalized.reserve(std::min(name.size(), wad_lump_name_size));

    for (const char value : name.substr(0, wad_lump_name_size)) {
        if (value == '\0') {
            break;
        }
        normalized.push_back(uppercase_ascii(value));
    }

    return normalized;
}

[[nodiscard]] std::uint32_t read_u32_le(const std::span<const std::byte> bytes,
                                        const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint32_t)) {
        throw std::runtime_error{"WAD field extends beyond file bounds"};
    }

    const auto byte_at = [bytes, offset](const std::size_t index) {
        return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + index]));
    };

    return byte_at(0) | (byte_at(1) << 8U) | (byte_at(2) << 16U) | (byte_at(3) << 24U);
}

[[nodiscard]] std::uint32_t read_non_negative_i32_le(const std::span<const std::byte> bytes,
                                                     const std::size_t offset,
                                                     const std::string_view field_name) {
    const std::uint32_t value = read_u32_le(bytes, offset);
    if (value > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
        throw std::runtime_error{std::string{"negative WAD "} + std::string{field_name}};
    }
    return value;
}

void validate_range(const std::size_t file_size, const ByteRange range,
                    const std::string_view description) {
    if (static_cast<std::size_t>(range.offset) > file_size) {
        throw std::runtime_error{std::string{description} + " starts beyond file bounds"};
    }

    const std::size_t start = range.offset;
    const std::size_t length = range.size;
    if (length > file_size - start) {
        throw std::runtime_error{std::string{description} + " extends beyond file bounds"};
    }
}

[[nodiscard]] std::vector<std::byte> read_file_bytes(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    if (!input) {
        throw std::runtime_error{"failed to open WAD file: " + path.string()};
    }

    std::vector<char> chars{std::istreambuf_iterator<char>{input},
                            std::istreambuf_iterator<char>{}};
    if (input.bad()) {
        throw std::runtime_error{"failed while reading WAD file: " + path.string()};
    }

    std::vector<std::byte> bytes;
    bytes.reserve(chars.size());
    std::ranges::transform(chars, std::back_inserter(bytes), [](const char value) {
        return std::byte{static_cast<unsigned char>(value)};
    });
    return bytes;
}

[[nodiscard]] WadType parse_type(const std::span<const std::byte> bytes) {
    const auto byte_as_char = [bytes](const std::size_t index) {
        return static_cast<char>(std::to_integer<unsigned char>(bytes[index]));
    };

    const std::array<char, 4> identifier{
        byte_as_char(0),
        byte_as_char(1),
        byte_as_char(2),
        byte_as_char(3),
    };

    if (identifier == std::array<char, 4>{'I', 'W', 'A', 'D'}) {
        return WadType::iwad;
    }
    if (identifier == std::array<char, 4>{'P', 'W', 'A', 'D'}) {
        return WadType::pwad;
    }

    throw std::runtime_error{"unknown WAD identifier"};
}

[[nodiscard]] std::string read_lump_name(const std::span<const std::byte> bytes,
                                         const std::size_t offset) {
    std::string name;
    name.reserve(wad_lump_name_size);

    for (std::size_t index = 0; index < wad_lump_name_size; ++index) {
        const char value = static_cast<char>(std::to_integer<unsigned char>(bytes[offset + index]));
        if (value == '\0') {
            break;
        }
        name.push_back(uppercase_ascii(value));
    }

    return name;
}

} // namespace

WadFile WadFile::load_from_file(const std::filesystem::path& path) {
    WadFile wad;
    wad.data_ = read_file_bytes(path);
    const std::span<const std::byte> bytes{wad.data_};

    if (bytes.size() < wad_header_size) {
        throw std::runtime_error{"WAD file is shorter than its header"};
    }

    wad.type_ = parse_type(bytes);
    const std::uint32_t lump_count = read_non_negative_i32_le(bytes, 4, "lump count");
    const std::uint32_t directory_offset = read_non_negative_i32_le(bytes, 8, "directory offset");

    const std::size_t directory_size =
        static_cast<std::size_t>(lump_count) * wad_directory_entry_size;
    if (static_cast<std::size_t>(lump_count) >
        std::numeric_limits<std::size_t>::max() / wad_directory_entry_size) {
        throw std::runtime_error{"WAD directory size overflows platform size"};
    }
    if (static_cast<std::size_t>(directory_offset) > bytes.size() ||
        directory_size > bytes.size() - static_cast<std::size_t>(directory_offset)) {
        throw std::runtime_error{"WAD directory extends beyond file bounds"};
    }

    wad.lumps_.reserve(lump_count);
    for (std::uint32_t index = 0; index < lump_count; ++index) {
        const std::size_t entry_offset =
            static_cast<std::size_t>(directory_offset) +
            (static_cast<std::size_t>(index) * wad_directory_entry_size);
        const std::uint32_t lump_offset =
            read_non_negative_i32_le(bytes, entry_offset, "lump offset");
        const std::uint32_t lump_size =
            read_non_negative_i32_le(bytes, entry_offset + 4, "lump size");
        validate_range(bytes.size(), ByteRange{.offset = lump_offset, .size = lump_size},
                       "WAD lump");

        wad.lumps_.push_back(Lump{
            .name = read_lump_name(bytes, entry_offset + 8),
            .offset = lump_offset,
            .size = lump_size,
        });
    }

    return wad;
}

WadType WadFile::type() const noexcept {
    return type_;
}

std::span<const Lump> WadFile::lumps() const noexcept {
    return lumps_;
}

const Lump* WadFile::find_lump(const std::string_view name) const noexcept {
    const std::string normalized = normalize_lump_name(name);
    const auto found = std::ranges::find_if(
        lumps_, [&normalized](const Lump& lump) { return lump.name == normalized; });
    if (found == lumps_.end()) {
        return nullptr;
    }
    return std::addressof(*found);
}

std::span<const std::byte> WadFile::lump_data(const Lump& lump) const {
    validate_range(data_.size(), ByteRange{.offset = lump.offset, .size = lump.size}, "WAD lump");
    const std::span<const std::byte> bytes{data_};
    return bytes.subspan(lump.offset, lump.size);
}

} // namespace doomcpp
