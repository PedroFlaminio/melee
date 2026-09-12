#include "assets/virtual_disc.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>

namespace melee::assets {
namespace {

constexpr std::array<std::byte, 8> kIndexMagic{
    std::byte{ 'M' }, std::byte{ 'D' }, std::byte{ 'V' }, std::byte{ 'D' },
    std::byte{ 'I' }, std::byte{ 'D' }, std::byte{ 'X' }, std::byte{ '1' },
};
constexpr std::uint32_t kIndexVersion = 1;
constexpr std::size_t kIndexHeaderSize = 16;
constexpr std::size_t kIndexEntryPrefixSize = 36;

std::vector<std::byte> read_all(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw VirtualDiscError("cannot open " + path.string());
    }
    const auto end = stream.tellg();
    if (end < 0) {
        throw VirtualDiscError("cannot determine the size of " + path.string());
    }
    const auto size = static_cast<std::uintmax_t>(end);
    if (size > std::numeric_limits<std::size_t>::max()) {
        throw VirtualDiscError("file is too large to map into memory");
    }

    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    if (!bytes.empty()) {
        stream.read(reinterpret_cast<char*>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size()));
    }
    if (!stream && !bytes.empty()) {
        throw VirtualDiscError("cannot read " + path.string());
    }
    return bytes;
}

std::uint32_t read_be32(std::span<const std::byte> bytes, std::size_t offset)
{
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw VirtualDiscError("truncated DVD index field");
    }
    return (std::to_integer<std::uint32_t>(bytes[offset]) << 24U) |
           (std::to_integer<std::uint32_t>(bytes[offset + 1]) << 16U) |
           (std::to_integer<std::uint32_t>(bytes[offset + 2]) << 8U) |
           std::to_integer<std::uint32_t>(bytes[offset + 3]);
}

std::uint64_t read_be64(std::span<const std::byte> bytes, std::size_t offset)
{
    const std::uint64_t upper = read_be32(bytes, offset);
    const std::uint64_t lower = read_be32(bytes, offset + 4);
    return (upper << 32U) | lower;
}

std::filesystem::path validate_relative_path(std::string_view path)
{
    if (path.empty() || path.find('\0') != std::string_view::npos) {
        throw VirtualDiscError("DVD index contains an empty or null path");
    }
    const std::filesystem::path relative{ std::string(path) };
    if (relative.is_absolute()) {
        throw VirtualDiscError("DVD index contains an absolute path");
    }
    for (const auto& component : relative) {
        if (component == ".." || component == ".") {
            throw VirtualDiscError("DVD index contains a relative traversal");
        }
    }
    return relative;
}

} // namespace

VirtualDisc::VirtualDisc(const std::filesystem::path& resource_root)
    : root_(std::filesystem::canonical(resource_root))
{
    if (!std::filesystem::is_directory(root_)) {
        throw VirtualDiscError("resource root is not a directory");
    }

    const std::vector<std::byte> bytes = read_all(root_ / "dvd-index.bin");
    const std::span<const std::byte> input{ bytes };
    if (input.size() < kIndexHeaderSize ||
        !std::equal(kIndexMagic.begin(), kIndexMagic.end(), input.begin())) {
        throw VirtualDiscError("DVD index has an invalid magic");
    }
    if (read_be32(input, 8) != kIndexVersion) {
        throw VirtualDiscError("DVD index version is unsupported");
    }

    const std::uint32_t count = read_be32(input, 12);
    std::size_t cursor = kIndexHeaderSize;
    entries_by_number_.reserve(count);
    entry_numbers_by_path_.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        if (cursor > input.size() || input.size() - cursor < kIndexEntryPrefixSize) {
            throw VirtualDiscError("DVD index entry is truncated");
        }
        const std::uint32_t entry_number = read_be32(input, cursor);
        const std::uint64_t size = read_be64(input, cursor + 4);
        const std::uint32_t path_size = read_be32(input, cursor + 12);
        cursor += kIndexEntryPrefixSize;
        if (path_size > input.size() - cursor) {
            throw VirtualDiscError("DVD index path exceeds the input");
        }

        const auto path_bytes = input.subspan(cursor, path_size);
        const auto* begin = reinterpret_cast<const char*>(path_bytes.data());
        const std::string path{ begin, path_bytes.size() };
        const auto relative_path = validate_relative_path(path);
        cursor += path_size;

        const auto [entry_it, entry_inserted] = entries_by_number_.emplace(
            entry_number, VirtualDiscEntry{ entry_number, relative_path, size });
        if (!entry_inserted) {
            throw VirtualDiscError("DVD index has duplicate entry numbers");
        }
        const auto [path_it, path_inserted] = entry_numbers_by_path_.emplace(
            relative_path.generic_string(), entry_number);
        if (!path_inserted) {
            throw VirtualDiscError("DVD index has duplicate paths");
        }
    }
    if (cursor != input.size()) {
        throw VirtualDiscError("DVD index has trailing bytes");
    }
}

const std::filesystem::path& VirtualDisc::root() const noexcept
{
    return root_;
}

std::size_t VirtualDisc::entry_count() const noexcept
{
    return entries_by_number_.size();
}

const VirtualDiscEntry& VirtualDisc::entry(std::uint32_t entry_number) const
{
    const auto found = entries_by_number_.find(entry_number);
    if (found == entries_by_number_.end()) {
        throw VirtualDiscError("DVD entry number was not found");
    }
    return found->second;
}

const VirtualDiscEntry& VirtualDisc::entry(std::string_view path) const
{
    const auto found = entry_numbers_by_path_.find(std::string(path));
    if (found == entry_numbers_by_path_.end()) {
        throw VirtualDiscError("DVD path was not found");
    }
    return entry(found->second);
}

std::vector<std::byte> VirtualDisc::read(std::uint32_t entry_number) const
{
    const VirtualDiscEntry& selected = entry(entry_number);
    if (selected.size > std::numeric_limits<std::size_t>::max()) {
        throw VirtualDiscError("DVD entry is too large to read into memory");
    }
    return read_range(entry_number, 0, static_cast<std::size_t>(selected.size));
}

std::vector<std::byte> VirtualDisc::read_range(std::uint32_t entry_number,
                                               std::uint64_t offset,
                                               std::size_t length) const
{
    const VirtualDiscEntry& selected = entry(entry_number);
    if (offset > selected.size || length > selected.size - offset) {
        throw VirtualDiscError("DVD read range exceeds the entry");
    }
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        throw VirtualDiscError("DVD read offset exceeds stream limits");
    }

    std::ifstream stream(resolve_file(selected), std::ios::binary);
    if (!stream) {
        throw VirtualDiscError("cannot open DVD entry for ranged read");
    }
    stream.seekg(static_cast<std::streamoff>(offset));
    if (!stream) {
        throw VirtualDiscError("cannot seek DVD entry");
    }
    std::vector<std::byte> bytes(length);
    if (!bytes.empty()) {
        stream.read(reinterpret_cast<char*>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size()));
    }
    if (!stream && !bytes.empty()) {
        throw VirtualDiscError("cannot read DVD entry range");
    }
    return bytes;
}

std::filesystem::path
VirtualDisc::resolve_file(const VirtualDiscEntry& selected) const
{
    const auto candidate = std::filesystem::canonical(root_ / selected.relative_path);
    const auto relative = candidate.lexically_relative(root_);
    if (relative.empty() || relative.is_absolute() ||
        relative.begin() == relative.end() || *relative.begin() == "..") {
        throw VirtualDiscError("DVD entry resolves outside the resource root");
    }
    if (!std::filesystem::is_regular_file(candidate)) {
        throw VirtualDiscError("DVD entry is not a regular file");
    }
    return candidate;
}

} // namespace melee::assets
