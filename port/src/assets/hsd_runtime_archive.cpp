#include "assets/hsd_runtime_archive.hpp"

#include <algorithm>
#include <limits>

namespace melee::assets {

HsdRuntimeArchive::HsdRuntimeArchive(std::span<const std::byte> input)
    : storage_(input.begin(), input.end())
    , view_(std::make_unique<HsdArchiveView>(std::span<const std::byte>(storage_)))
{
}

const HsdArchiveView& HsdRuntimeArchive::disk_view() const noexcept
{
    return *view_;
}

HsdRuntimeNode HsdRuntimeArchive::public_root(std::string_view symbol) const
{
    return { view_->public_data_offset(symbol) };
}

std::uint32_t HsdRuntimeArchive::read_u32(HsdRuntimeNode node,
                                          std::uint32_t relative_offset) const
{
    const std::uint64_t offset = static_cast<std::uint64_t>(node.data_offset) +
                                 relative_offset;
    const auto data = view_->data();
    const std::uint64_t data_size = data.size();
    if (offset > data_size || data_size - offset < 4) {
        throw HsdArchiveError("HSD runtime field exceeds the data section");
    }
    const std::size_t index = static_cast<std::size_t>(offset);
    return (std::to_integer<std::uint32_t>(data[index]) << 24U) |
           (std::to_integer<std::uint32_t>(data[index + 1]) << 16U) |
           (std::to_integer<std::uint32_t>(data[index + 2]) << 8U) |
           std::to_integer<std::uint32_t>(data[index + 3]);
}

std::string_view HsdRuntimeArchive::read_c_string(HsdRuntimeNode node) const
{
    const auto data = view_->data();
    if (node.data_offset >= data.size()) {
        throw HsdArchiveError("HSD runtime string exceeds the data section");
    }
    const char* begin = reinterpret_cast<const char*>(
        data.data() + static_cast<std::size_t>(node.data_offset));
    const std::size_t remaining = data.size() - node.data_offset;
    const char* end = std::find(begin, begin + remaining, '\0');
    if (end == begin + remaining) {
        throw HsdArchiveError("HSD runtime string is not null-terminated");
    }
    return { begin, static_cast<std::size_t>(end - begin) };
}

HsdRuntimeNode HsdRuntimeArchive::reference_at(
    HsdRuntimeNode node, std::uint32_t relative_offset) const
{
    const std::uint64_t field = static_cast<std::uint64_t>(node.data_offset) +
                                relative_offset;
    if (field > std::numeric_limits<std::uint32_t>::max()) {
        throw HsdArchiveError("HSD runtime reference offset overflows u32");
    }
    return { view_->relocated_target(static_cast<std::uint32_t>(field)) };
}

std::vector<HsdRuntimeReference>
HsdRuntimeArchive::internal_references() const
{
    const std::vector<std::uint32_t> fields = view_->relocation_fields();
    std::vector<HsdRuntimeReference> references;
    references.reserve(fields.size());
    for (const std::uint32_t field : fields) {
        references.push_back({ field, { view_->relocated_target(field) } });
    }
    return references;
}

} // namespace melee::assets
