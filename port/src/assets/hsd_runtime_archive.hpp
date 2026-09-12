#ifndef MELEE_HOST_HSD_RUNTIME_ARCHIVE_HPP
#define MELEE_HOST_HSD_RUNTIME_ARCHIVE_HPP

#include "assets/hsd_archive.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace melee::assets {

/* A host-safe address into an HSD data section; never a serialized pointer. */
struct HsdRuntimeNode {
    std::uint32_t data_offset;
};

struct HsdRuntimeReference {
    std::uint32_t field_offset;
    HsdRuntimeNode target;
};

/*
 * Owns the source blob and exposes its internal graph as validated 32-bit
 * offsets. Typed schema loaders can consume these nodes without applying the
 * GameCube in-place pointer relocation used by the original archive.c.
 */
class HsdRuntimeArchive final {
public:
    explicit HsdRuntimeArchive(std::span<const std::byte> input);

    [[nodiscard]] const HsdArchiveView& disk_view() const noexcept;
    [[nodiscard]] HsdRuntimeNode public_root(std::string_view symbol) const;
    [[nodiscard]] std::uint32_t read_u32(HsdRuntimeNode node,
                                         std::uint32_t relative_offset) const;
    [[nodiscard]] std::uint16_t read_u16(HsdRuntimeNode node,
                                         std::uint32_t relative_offset) const;
    [[nodiscard]] float read_f32(HsdRuntimeNode node,
                                 std::uint32_t relative_offset) const;
    [[nodiscard]] std::span<const std::byte> bytes_at(
        HsdRuntimeNode node, std::size_t length) const;
    [[nodiscard]] std::string_view read_c_string(HsdRuntimeNode node) const;
    [[nodiscard]] HsdRuntimeNode reference_at(
        HsdRuntimeNode node, std::uint32_t relative_offset) const;
    [[nodiscard]] bool has_reference_at(
        HsdRuntimeNode node, std::uint32_t relative_offset) const;
    [[nodiscard]] std::vector<HsdRuntimeReference> internal_references() const;

private:
    std::vector<std::byte> storage_;
    std::unique_ptr<HsdArchiveView> view_;
};

} // namespace melee::assets

#endif
