#ifndef MELEE_HOST_HSD_ARCHIVE_HPP
#define MELEE_HOST_HSD_ARCHIVE_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace melee::assets {

class HsdArchiveError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct HsdArchiveHeader {
    std::uint32_t file_size;
    std::uint32_t data_size;
    std::uint32_t relocation_count;
    std::uint32_t public_count;
    std::uint32_t extern_count;
    std::uint32_t version;
};

struct HsdPublicSymbol {
    std::string_view name;
    std::uint32_t data_offset;
};

class HsdArchiveView final {
public:
    explicit HsdArchiveView(std::span<const std::byte> bytes);

    [[nodiscard]] const HsdArchiveHeader& header() const noexcept;
    [[nodiscard]] std::span<const std::byte> data() const noexcept;
    [[nodiscard]] std::uint32_t relocated_target(
        std::uint32_t field_offset) const;
    [[nodiscard]] std::uint32_t public_data_offset(
        std::string_view symbol) const;
    [[nodiscard]] std::vector<HsdPublicSymbol> public_symbols() const;
    [[nodiscard]] std::vector<std::uint32_t> relocation_fields() const;

private:
    struct NamedEntry {
        std::uint32_t offset;
        std::uint32_t symbol_offset;
    };

    [[nodiscard]] std::string_view symbol_at(std::uint32_t offset) const;

    std::span<const std::byte> bytes_;
    std::span<const std::byte> data_;
    std::span<const std::byte> symbols_;
    HsdArchiveHeader header_{};
    std::vector<std::uint32_t> relocations_;
    std::vector<NamedEntry> public_entries_;
    std::vector<NamedEntry> extern_entries_;
};

} // namespace melee::assets

#endif
