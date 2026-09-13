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

struct HsdArchiveMember {
    std::size_t offset;
    std::size_t size;
    /* The member's first public symbol, which is how the game names it.  A
     * character's animations use one archive per action, named after it. */
    std::string_view symbol;
};

/* Walks a file that holds several HSD archives laid end to end, each padded to
 * a 32-byte boundary.  There is no index: every header states its own length,
 * which is what makes the walk possible.  Stops at the first header that does
 * not parse, so a truncated file yields what was readable rather than
 * throwing. */
[[nodiscard]] std::vector<HsdArchiveMember> enumerate_hsd_archives(
    std::span<const std::byte> bytes);

class HsdArchiveView final {
public:
    explicit HsdArchiveView(std::span<const std::byte> bytes);

    [[nodiscard]] const HsdArchiveHeader& header() const noexcept;
    [[nodiscard]] std::span<const std::byte> data() const noexcept;
    [[nodiscard]] std::uint32_t relocated_target(
        std::uint32_t field_offset) const;
    [[nodiscard]] bool has_relocation(
        std::uint32_t field_offset) const noexcept;
    [[nodiscard]] std::uint32_t public_data_offset(
        std::string_view symbol) const;
    [[nodiscard]] std::vector<HsdPublicSymbol> public_symbols() const;
    /* Symbols the archive expects another archive to provide.  The data
     * offset is the head of a chain threaded through the pointer fields that
     * refer to the symbol, which is how HSD_ArchiveLocateExtern patches them
     * all. */
    [[nodiscard]] std::vector<HsdPublicSymbol> extern_symbols() const;
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
