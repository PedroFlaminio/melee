#ifndef MELEE_HOST_VIRTUAL_DISC_HPP
#define MELEE_HOST_VIRTUAL_DISC_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace melee::assets {

class VirtualDiscError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct VirtualDiscEntry {
    std::uint32_t entry_number;
    std::filesystem::path relative_path;
    std::uint64_t size;
};

class VirtualDisc final {
public:
    explicit VirtualDisc(const std::filesystem::path& resource_root);

    [[nodiscard]] const std::filesystem::path& root() const noexcept;
    [[nodiscard]] std::size_t entry_count() const noexcept;
    [[nodiscard]] const VirtualDiscEntry& entry(std::uint32_t entry_number) const;
    [[nodiscard]] const VirtualDiscEntry& entry(std::string_view path) const;
    [[nodiscard]] std::vector<std::byte> read(std::uint32_t entry_number) const;
    [[nodiscard]] std::vector<std::byte> read_range(
        std::uint32_t entry_number, std::uint64_t offset,
        std::size_t length) const;

private:
    [[nodiscard]] std::filesystem::path resolve_file(
        const VirtualDiscEntry& entry) const;

    std::filesystem::path root_;
    std::unordered_map<std::uint32_t, VirtualDiscEntry> entries_by_number_;
    std::unordered_map<std::string, std::uint32_t> entry_numbers_by_path_;
};

} // namespace melee::assets

#endif
