#include "assets/hsd_archive.hpp"

#include <algorithm>
#include <limits>
#include <string>

namespace melee::assets {
namespace {

constexpr std::size_t kHeaderSize = 0x20;
constexpr std::size_t kNamedEntrySize = 8;
constexpr std::size_t kRelocationEntrySize = 4;

std::uint32_t read_be32(std::span<const std::byte> bytes, std::size_t offset)
{
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw HsdArchiveError("truncated 32-bit big-endian value");
    }

    const auto b0 = std::to_integer<std::uint32_t>(bytes[offset]);
    const auto b1 = std::to_integer<std::uint32_t>(bytes[offset + 1]);
    const auto b2 = std::to_integer<std::uint32_t>(bytes[offset + 2]);
    const auto b3 = std::to_integer<std::uint32_t>(bytes[offset + 3]);
    return (b0 << 24U) | (b1 << 16U) | (b2 << 8U) | b3;
}

std::size_t checked_add(std::size_t left, std::size_t right,
                        std::string_view description)
{
    if (right > std::numeric_limits<std::size_t>::max() - left) {
        throw HsdArchiveError(std::string(description) + " overflows size_t");
    }
    return left + right;
}

std::size_t checked_table_end(std::size_t offset, std::uint32_t count,
                              std::size_t stride,
                              std::string_view description)
{
    if (count > std::numeric_limits<std::size_t>::max() / stride) {
        throw HsdArchiveError(std::string(description) + " is too large");
    }
    return checked_add(offset, static_cast<std::size_t>(count) * stride,
                       description);
}

} // namespace

std::vector<HsdArchiveMember> enumerate_hsd_archives(
    std::span<const std::byte> bytes)
{
    constexpr std::size_t kAlignment = 32;
    std::vector<HsdArchiveMember> members;
    std::size_t offset = 0;

    while (offset + kHeaderSize <= bytes.size()) {
        const std::size_t declared = read_be32(bytes, offset);
        if (declared < kHeaderSize || declared > bytes.size() - offset) {
            break;
        }
        const std::span<const std::byte> member =
            bytes.subspan(offset, declared);
        std::string_view symbol;
        try {
            const HsdArchiveView view(member);
            const std::vector<HsdPublicSymbol> symbols = view.public_symbols();
            if (!symbols.empty()) {
                symbol = symbols.front().name;
            }
        } catch (const HsdArchiveError&) {
            break;
        }
        members.push_back({ offset, declared, symbol });
        /* Each member starts on a 32-byte boundary, so the next one begins at
         * the rounded-up end of this one. */
        const std::size_t advance =
            (declared + kAlignment - 1) / kAlignment * kAlignment;
        if (advance == 0 || advance > bytes.size() - offset) {
            break;
        }
        offset += advance;
    }
    return members;
}

HsdArchiveView::HsdArchiveView(std::span<const std::byte> bytes) : bytes_(bytes)
{
    if (bytes.size() < kHeaderSize) {
        throw HsdArchiveError("HSD archive is smaller than its header");
    }

    header_.file_size = read_be32(bytes, 0x00);
    header_.data_size = read_be32(bytes, 0x04);
    header_.relocation_count = read_be32(bytes, 0x08);
    header_.public_count = read_be32(bytes, 0x0C);
    header_.extern_count = read_be32(bytes, 0x10);
    header_.version = read_be32(bytes, 0x14);

    if (header_.file_size != bytes.size()) {
        throw HsdArchiveError("HSD file_size does not match the input size");
    }

    std::size_t cursor = kHeaderSize;
    const std::size_t data_end =
        checked_add(cursor, header_.data_size, "HSD data section");
    if (data_end > bytes.size()) {
        throw HsdArchiveError("HSD data section exceeds the input");
    }
    data_ = bytes.subspan(cursor, header_.data_size);
    cursor = data_end;

    const std::size_t relocation_end = checked_table_end(
        cursor, header_.relocation_count, kRelocationEntrySize,
        "HSD relocation table");
    if (relocation_end > bytes.size()) {
        throw HsdArchiveError("HSD relocation table exceeds the input");
    }
    relocations_.reserve(header_.relocation_count);
    for (std::uint32_t i = 0; i < header_.relocation_count; ++i) {
        const std::size_t entry =
            cursor + static_cast<std::size_t>(i) * kRelocationEntrySize;
        const std::uint32_t field_offset = read_be32(bytes, entry);
        if (field_offset > data_.size() || data_.size() - field_offset < 4) {
            throw HsdArchiveError("HSD relocation field exceeds data section");
        }
        relocations_.push_back(field_offset);
    }
    cursor = relocation_end;

    const auto read_named_table = [&](std::uint32_t count,
                                      std::vector<NamedEntry>& output,
                                      std::string_view description) {
        const std::size_t end =
            checked_table_end(cursor, count, kNamedEntrySize, description);
        if (end > bytes.size()) {
            throw HsdArchiveError(std::string(description) +
                                  " exceeds the input");
        }
        output.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::size_t entry =
                cursor + static_cast<std::size_t>(i) * kNamedEntrySize;
            const std::uint32_t offset = read_be32(bytes, entry);
            const std::uint32_t symbol_offset = read_be32(bytes, entry + 4);
            if (offset > data_.size()) {
                throw HsdArchiveError(std::string(description) +
                                      " contains an invalid data offset");
            }
            output.push_back({ offset, symbol_offset });
        }
        cursor = end;
    };

    read_named_table(header_.public_count, public_entries_,
                     "HSD public table");
    read_named_table(header_.extern_count, extern_entries_,
                     "HSD extern table");
    symbols_ = bytes.subspan(cursor);

    for (const auto& entry : public_entries_) {
        static_cast<void>(symbol_at(entry.symbol_offset));
    }
    for (const auto& entry : extern_entries_) {
        static_cast<void>(symbol_at(entry.symbol_offset));
    }
}

const HsdArchiveHeader& HsdArchiveView::header() const noexcept
{
    return header_;
}

std::span<const std::byte> HsdArchiveView::data() const noexcept
{
    return data_;
}

std::uint32_t
HsdArchiveView::relocated_target(std::uint32_t field_offset) const
{
    if (std::find(relocations_.begin(), relocations_.end(), field_offset) ==
        relocations_.end()) {
        throw HsdArchiveError("requested field is not in the relocation table");
    }

    const std::uint32_t target = read_be32(data_, field_offset);
    if (target > data_.size()) {
        throw HsdArchiveError("relocated target exceeds the HSD data section");
    }
    return target;
}

bool HsdArchiveView::has_relocation(std::uint32_t field_offset) const noexcept
{
    return std::find(relocations_.begin(), relocations_.end(), field_offset) !=
           relocations_.end();
}

std::uint32_t
HsdArchiveView::public_data_offset(std::string_view symbol) const
{
    const auto entry = std::find_if(
        public_entries_.begin(), public_entries_.end(),
        [&](const NamedEntry& candidate) {
            return symbol_at(candidate.symbol_offset) == symbol;
        });
    if (entry == public_entries_.end()) {
        throw HsdArchiveError("public HSD symbol was not found");
    }
    return entry->offset;
}

std::vector<HsdPublicSymbol> HsdArchiveView::public_symbols() const
{
    std::vector<HsdPublicSymbol> symbols;
    symbols.reserve(public_entries_.size());
    for (const NamedEntry& entry : public_entries_) {
        symbols.push_back({ symbol_at(entry.symbol_offset), entry.offset });
    }
    return symbols;
}

std::vector<HsdPublicSymbol> HsdArchiveView::extern_symbols() const
{
    std::vector<HsdPublicSymbol> symbols;
    symbols.reserve(extern_entries_.size());
    for (const NamedEntry& entry : extern_entries_) {
        symbols.push_back({ symbol_at(entry.symbol_offset), entry.offset });
    }
    return symbols;
}

std::vector<std::uint32_t> HsdArchiveView::relocation_fields() const
{
    return relocations_;
}

std::string_view HsdArchiveView::symbol_at(std::uint32_t offset) const
{
    if (offset >= symbols_.size()) {
        throw HsdArchiveError("HSD symbol offset exceeds the symbol table");
    }

    const char* begin = reinterpret_cast<const char*>(symbols_.data() + offset);
    const std::size_t remaining = symbols_.size() - offset;
    const auto terminator = std::find(begin, begin + remaining, '\0');
    if (terminator == begin + remaining) {
        throw HsdArchiveError("HSD symbol is not null-terminated");
    }
    return { begin, static_cast<std::size_t>(terminator - begin) };
}

} // namespace melee::assets
